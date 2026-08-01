#!/usr/bin/env bash
#
# parity_real.sh - prove rt.so and rt_duckdb.so BILL THE SAME on real data.
#
# The golden regression only checks 3 synthetic CDRs. Before flipping production
# offline rating from rt.so to rt_duckdb.so, we must confirm the two engines
# produce identical per-CDR billing on the REAL feature mix (time conditions,
# leg B, pcard, free_billsec, KLimitMin, billing day, ...). This:
#   1. clones the live DB READ-ONLY (pg_dump) into a throwaway TEMPLATE, then
#      makes two identical copies from it (fast, no second dump);
#   2. on copy A: resets the newest N rated CDRs and rates them with rt.so;
#   3. on copy B: resets the SAME CDRs and rates them with rt_duckdb.so;
#   4. diffs per-CDR SUM(call_price)/SUM(call_billsec) between A and B.
# Identical -> DuckDB is safe for offline. Mismatch -> lists the CDRs + values
# that differ (pinpoints a feature DuckDB doesn't implement).
#
# SAFETY: the live DB is only read (pg_dump). All resets/rating happen on the
# throwaway copies, which must differ from SRCDB (asserted). Live data untouched.
#
# Env: SRCDB (default rate_engine), N (default 20000), THREADS (default 4),
#      TOL (price abs tolerance, default 0.005), DBHOST/DBPORT/DBUSER/DBPASS,
#      RE_PREFIX/RE_CONF, KEEP=1 to keep copies+workdir.
# Exit: 0 all CDRs match, 1 a mismatch, 2 prerequisites missing.
set -u

HERE=$(cd "$(dirname "$0")" && pwd)

RE_PREFIX=${RE_PREFIX:-/usr/local/RateEngine}
RE_BIN=${RE_BIN:-$RE_PREFIX/bin/RateEngine}
RE_LIBS=${RE_LIBS:-$RE_PREFIX/libs}
RE_MODULES=${RE_MODULES:-$RE_PREFIX/modules}
RE_CONF=${RE_CONF:-$RE_PREFIX/config/RateEngine7.xml}

db_from_conf() {
	[ -f "$RE_CONF" ] || return 0
	sed -n '/<DB>/,/<\/DB>/p' "$RE_CONF" 2>/dev/null |
		sed -nE "s/.*name=\"$1\"[^>]*value=\"([^\"]*)\".*/\1/p" | head -1
}
DBHOST=${DBHOST:-$(db_from_conf dbhost)}; DBHOST=${DBHOST:-re7-db}
DBUSER=${DBUSER:-$(db_from_conf dbuser)}; DBUSER=${DBUSER:-re_admin}
DBPASS=${DBPASS:-$(db_from_conf dbpass)}; DBPASS=${DBPASS:-_cfg.access}
DBPORT=${DBPORT:-$(db_from_conf dbport)}; DBPORT=${DBPORT:-5432}
export PGPASSWORD="$DBPASS"

SRCDB=${SRCDB:-rate_engine}
TMPL=${TMPL:-rate_engine_parity_tmpl}
DBA=${DBA:-rate_engine_parity_a}
DBB=${DBB:-rate_engine_parity_b}
N=${N:-20000}
THREADS=${THREADS:-4}
TOL=${TOL:-0.005}
KEEP=${KEEP:-0}

for d in "$TMPL" "$DBA" "$DBB"; do
	[ "$d" = "$SRCDB" ] && { echo "parity_real: refusing - '$d' must differ from SRCDB '$SRCDB'" >&2; exit 2; }
done

psql_c()  { psql -v ON_ERROR_STOP=1 -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" "$@"; }
q()       { psql -tA -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$1" -c "$2" 2>/dev/null; }

WORKDIR=""; RE_PID=""
cleanup() {
	[ -n "$RE_PID" ] && { kill -TERM "$RE_PID" 2>/dev/null; wait "$RE_PID" 2>/dev/null; }
	if [ "$KEEP" != "1" ]; then
		for d in "$DBA" "$DBB" "$TMPL"; do psql_c -d postgres -c "DROP DATABASE IF EXISTS $d;" >/dev/null 2>&1; done
		[ -n "$WORKDIR" ] && rm -rf "$WORKDIR"
	else
		echo "  (kept: $DBA $DBB $TMPL  workdir $WORKDIR)"
	fi
}
trap cleanup EXIT
die() { echo "parity_real: $*" >&2; exit 2; }

[ -x "$RE_BIN" ] || die "engine not found: $RE_BIN"
for m in pgsql.so cdrm.so rt.so rt_duckdb.so duckdb.so; do
	[ -e "$RE_MODULES/$m" ] || die "module $m not installed (need both engines)"
done
command -v psql >/dev/null && command -v pg_dump >/dev/null || die "psql/pg_dump not in PATH"

echo "parity_real: clone $SRCDB, rate N=$N with rt.so vs rt_duckdb.so, diff billing"

# 1. one read-only dump into a template, then two fast copies from it
psql_c -d postgres -c "DROP DATABASE IF EXISTS $DBA;" >/dev/null 2>&1
psql_c -d postgres -c "DROP DATABASE IF EXISTS $DBB;" >/dev/null 2>&1
psql_c -d postgres -c "DROP DATABASE IF EXISTS $TMPL;" >/dev/null 2>&1
psql_c -d postgres -c "CREATE DATABASE $TMPL;" >/dev/null 2>&1 || die "cannot CREATE DATABASE $TMPL (CREATEDB?)"
echo "  dumping $SRCDB (read-only) -> $TMPL ..."
pg_dump -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$SRCDB" \
	| psql -q -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$TMPL" >/dev/null 2>&1 || die "clone failed"
# copies (no connection to the template -> TEMPLATE copy is allowed)
psql_c -d postgres -c "CREATE DATABASE $DBA TEMPLATE $TMPL;" >/dev/null 2>&1 || die "copy A failed"
psql_c -d postgres -c "CREATE DATABASE $DBB TEMPLATE $TMPL;" >/dev/null 2>&1 || die "copy B failed"

WORKDIR=$(mktemp -d "${TMPDIR:-/tmp}/re7_parity.XXXXXX") || die "mktemp failed"
ln -s "$RE_MODULES" "$WORKDIR/modules"
mkdir -p "$WORKDIR/logs" "$WORKDIR/config/cdr_profiles"

# gen_cfg DBNAME MODULE -> writes config; picks module load list per engine
gen_cfg() {
	local db=$1 module=$2 modlines
	if [ "$module" = "rt_duckdb.so" ]; then
		modlines='    <param name="module" value="pgsql.so" />
    <param name="module" value="cdrm.so" />
    <param name="module" value="duckdb.so" />
    <param name="module" value="rt_duckdb.so" />'
	else
		modlines='    <param name="module" value="pgsql.so" />
    <param name="module" value="cdrm.so" />
    <param name="module" value="rt.so" />'
	fi
	cat >"$WORKDIR/config/RateEngine7.xml" <<EOF
<RateEngine version="0.7.6">
 <System>
    <param name="DIR" value="$WORKDIR/" />
    <param name="PIDFile" value="logs/rate_engine.pid" />
 </System>
 <LoadModules>
$modlines
 </LoadModules>
 <DB>
    <param name="dbtype" value="pgsql" />
    <param name="dbhost" value="$DBHOST" />
    <param name="dbname" value="$db" />
    <param name="dbuser" value="$DBUSER" />
    <param name="dbpass" value="$DBPASS" />
    <param name="dbport" value="$DBPORT" />
    <param name="NumberRetries" value="3" />
    <param name="IntervalRetries" value="1" />
 </DB>
 <Rating>
    <param name="active" value="yes" />
    <param name="RatingModule" value="$module" />
    <param name="leg" value="a" />
    <param name="RatingThreads" value="$THREADS" />
    <param name="BatchLimit" value="50000" />
    <param name="RatingInterval" value="2" />
    <param name="WaitRatingInterval" value="500" />
    <param name="UsePCard" value="no" />
    <param name="BillingDay" value="01" />
 </Rating>
 <CDRMediator>
    <param name="CDRProfilesDIR" value="$WORKDIR/config/cdr_profiles/" />
 </CDRMediator>
 <Logs>
    <param name="LogFile" value="logs/rate_engine.log" />
    <param name="LogMaxFileSize" value="409600000" />
    <param name="LogSeparator" value="|" />
    <param name="LogDebugLevel" value="0" />
 </Logs>
</RateEngine>
EOF
}

# rate_and_capture DBNAME MODULE OUTFILE - reset the replay set, rate, capture
# per-CDR "call_uid|sum_price|sum_billsec" for exactly that set.
rate_and_capture() {
	local db=$1 module=$2 out=$3
	# fix the replay set (same rows in both copies: identical clones) and reset it
	q "$db" "DROP TABLE IF EXISTS _replay_set;
	         CREATE TABLE _replay_set AS SELECT id FROM cdrs WHERE leg_a > 0 ORDER BY id DESC LIMIT $N;
	         DELETE FROM rating WHERE call_id IN (SELECT id FROM _replay_set);
	         UPDATE cdrs SET leg_a = 0 WHERE id IN (SELECT id FROM _replay_set);" >/dev/null \
		|| die "reset failed on $db"
	gen_cfg "$db" "$module"
	(
		cd "$WORKDIR" && exec env LD_LIBRARY_PATH="$RE_LIBS:${LD_LIBRARY_PATH:-}" \
			"$RE_BIN" -c "$WORKDIR/config/RateEngine7.xml" -r a
	) >"$WORKDIR/daemon.stdout" 2>&1 &
	RE_PID=$!
	local _ left
	for _ in $(seq 1 600); do
		kill -0 "$RE_PID" 2>/dev/null || break
		left=$(q "$db" "SELECT count(*) FROM cdrs WHERE leg_a = 0;")
		[ "${left:-1}" = "0" ] && break
		sleep 0.5
	done
	kill -TERM "$RE_PID" 2>/dev/null; wait "$RE_PID" 2>/dev/null; RE_PID=""
	q "$db" "SELECT c.call_uid||'|'||COALESCE(SUM(r.call_price),0)||'|'||COALESCE(SUM(r.call_billsec),0)
	           FROM cdrs c JOIN _replay_set rs ON rs.id = c.id
	           LEFT JOIN rating r ON r.call_id = c.id
	          GROUP BY c.call_uid ORDER BY c.call_uid;" >"$out"
}

echo "  rating copy A with rt.so ..."
rate_and_capture "$DBA" "rt.so"        "$WORKDIR/a.tsv"
echo "  rating copy B with rt_duckdb.so ..."
rate_and_capture "$DBB" "rt_duckdb.so" "$WORKDIR/b.tsv"

# 4. diff per-CDR billing
echo
echo "==================== rt.so vs rt_duckdb.so parity (real data) ===================="
join -t'|' -a1 -a2 -e MISSING -o '0,1.2,1.3,2.2,2.3' \
	<(sort "$WORKDIR/a.tsv") <(sort "$WORKDIR/b.tsv") \
	| awk -F'|' -v tol="$TOL" '
	{
		uid=$1; ap=$2; ab=$3; bp=$4; bb=$5; total++;
		if(ap=="MISSING" || bp=="MISSING"){ miss++; if(shown++<20) printf "  MISSING: %s  rt=%s/%s duckdb=%s/%s\n",uid,ap,ab,bp,bb; next }
		dp=ap-bp; if(dp<0)dp=-dp;
		if(dp>=tol || ab!=bb){ bad++; if(shown++<20) printf "  DIFF: %s  price rt=%s duckdb=%s  billsec rt=%s duckdb=%s\n",uid,ap,bp,ab,bb }
		else ok++;
	}
	END{
		printf "\n  CDRs compared: %d\n", total;
		printf "  match: %d   price/billsec diff: %d   missing: %d\n", ok+0, bad+0, miss+0;
		if((bad+0)+(miss+0)==0) print "  RESULT: PARITY OK - rt.so and rt_duckdb.so bill identically";
		else print "  RESULT: MISMATCH - DuckDB is NOT a drop-in for these CDRs (see above)";
	}'
echo "=================================================================================="

# exit non-zero on any mismatch
if join -t'|' -a1 -a2 -e MISSING -o '0,1.2,1.3,2.2,2.3' <(sort "$WORKDIR/a.tsv") <(sort "$WORKDIR/b.tsv") \
	| awk -F'|' -v tol="$TOL" '{ap=$2;ab=$3;bp=$4;bb=$5; if(ap=="MISSING"||bp=="MISSING")exit 1; d=ap-bp;if(d<0)d=-d; if(d>=tol||ab!=bb)exit 1} END{exit 0}'; then
	exit 0
else
	exit 1
fi
