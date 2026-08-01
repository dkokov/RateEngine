#!/usr/bin/env bash
#
# bench_rating_replay.sh - offline-rating throughput baseline on a CLONE of the
#                          real DB, replaying real CDRs. NEVER mutates live data.
#
# What it does:
#   1. Clones the live DB (READ-ONLY via pg_dump) into a THROWAWAY bench DB.
#   2. On the CLONE ONLY: takes the newest N already-rated CDRs, resets them to
#      leg_a=0 and deletes their rating rows, so rt.so re-rates them - exercising
#      the real rating + balance/save path (the sync_bt_thread lock we're about
#      to change) with the real account/rate distribution.
#   3. Runs rt.so against the clone (leg a), capturing an INFO-level log.
#   4. Feeds the log to rating_perf_report.sh (records a labelled baseline row).
#   5. Drops the clone.
#
# SAFETY: the live DB is only ever read (pg_dump). Every DROP/UPDATE/DELETE runs
# against $BENCHDB, which must differ from $SRCDB (asserted). Live balances are
# never touched.
#
# Env (creds default to the installed engine config, then the usual defaults):
#   SRCDB   live DB to clone            (default rate_engine)   [READ ONLY]
#   BENCHDB throwaway clone name        (default rate_engine_bench)
#   N       # of rated CDRs to replay   (default 20000)
#   THREADS RatingThreads for the run   (default 4)
#   LABEL   report row label            (default 0.7.6-baseline)
#   DBHOST/DBPORT/DBUSER/DBPASS, RE_PREFIX, RE_CONF
#
# Exit: 0 ok, 1 run/report failure, 2 prerequisites missing.
set -u

HERE=$(cd "$(dirname "$0")" && pwd)
REPORT="$HERE/rating_perf_report.sh"

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
BENCHDB=${BENCHDB:-rate_engine_bench}
N=${N:-20000}
THREADS=${THREADS:-4}
LABEL=${LABEL:-0.7.6-baseline}
KEEP=${KEEP:-0}
REPEAT=${REPEAT:-3}   # passes on ONE warm clone; report the MEDIAN (cuts variance)
MODULE=${MODULE:-rt.so}   # rating engine to benchmark: rt.so (online) | rt_duckdb.so (offline)

# module load list + RatingModule for the chosen engine
if [ "$MODULE" = "rt_duckdb.so" ]; then
	MODLINES='    <param name="module" value="pgsql.so" />
    <param name="module" value="cdrm.so" />
    <param name="module" value="duckdb.so" />
    <param name="module" value="rt_duckdb.so" />'
else
	MODLINES='    <param name="module" value="pgsql.so" />
    <param name="module" value="cdrm.so" />
    <param name="module" value="rt.so" />'
fi

# --- hard safety guard: never operate on the live DB -------------------------
if [ "$BENCHDB" = "$SRCDB" ]; then
	echo "bench_rating_replay: refusing to run - BENCHDB ($BENCHDB) must differ from SRCDB ($SRCDB)" >&2
	exit 2
fi

psql_c() { psql -v ON_ERROR_STOP=1 -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" "$@"; }
q_bench() { psql -tA -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$BENCHDB" -c "$1" 2>/dev/null; }

WORKDIR=""; RE_PID=""; LOGFILE=""; CLONED=""; FAILKEEP=0
cleanup() {
	[ -n "$RE_PID" ] && { kill -TERM "$RE_PID" 2>/dev/null; wait "$RE_PID" 2>/dev/null; }
	# drop ONLY the throwaway clone
	[ -n "$CLONED" ] && psql_c -d postgres -c "DROP DATABASE IF EXISTS $BENCHDB;" >/dev/null 2>&1
	# keep the workdir (logs) on failure or when KEEP=1, so it can be inspected
	if [ -n "$WORKDIR" ]; then
		if [ "$KEEP" = "1" ] || [ "$FAILKEEP" = "1" ]; then
			echo "  (workdir kept for inspection: $WORKDIR)"
		else
			rm -rf "$WORKDIR"
		fi
	fi
}
trap cleanup EXIT

die() { echo "bench_rating_replay: $*" >&2; exit 2; }

# --- prerequisites -----------------------------------------------------------
[ -x "$RE_BIN" ]     || die "engine not found: $RE_BIN (set RE_PREFIX)"
[ -x "$REPORT" ]     || die "missing $REPORT"
for m in pgsql.so cdrm.so; do [ -e "$RE_MODULES/$m" ] || die "module $m not installed"; done
[ -e "$RE_MODULES/$MODULE" ] || die "rating module $MODULE not installed in $RE_MODULES"
[ "$MODULE" = "rt_duckdb.so" ] && { [ -e "$RE_MODULES/duckdb.so" ] || die "duckdb.so not installed (needed by rt_duckdb.so)"; }
command -v psql    >/dev/null || die "psql not in PATH"
command -v pg_dump >/dev/null || die "pg_dump not in PATH"

echo "bench_rating_replay: clone $SRCDB -> $BENCHDB @ $DBHOST:$DBPORT ; replay N=$N threads=$THREADS module=$MODULE"

# --- 1. clone (READ-ONLY on source) ------------------------------------------
psql_c -d postgres -c "DROP DATABASE IF EXISTS $BENCHDB;" >/dev/null 2>&1 || true
psql_c -d postgres -c "CREATE DATABASE $BENCHDB;" >/dev/null 2>&1 \
	|| die "cannot CREATE DATABASE $BENCHDB (does $DBUSER have CREATEDB?)"
CLONED=1
echo "  dumping $SRCDB (read-only) and loading into $BENCHDB ..."
if ! pg_dump -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$SRCDB" \
	| psql -q -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$BENCHDB" >/dev/null 2>&1; then
	die "clone failed (pg_dump | psql)"
fi
# pg_dump restores data but NOT planner statistics -> the fresh clone would
# seq-scan and give unrepresentative (slow) query plans. ANALYZE so rating uses
# the same index plans production does; otherwise the baseline is inflated and a
# lock-change improvement could be masked by bad plans.
echo "  ANALYZE clone (rebuild planner statistics) ..."
psql_c -d "$BENCHDB" -c "VACUUM ANALYZE;" >/dev/null 2>&1 || true

# --- static bench config pointing at the CLONE ------------------------------
before=$(q_bench "SELECT count(*) FROM cdrs WHERE leg_a > 0;")
[ "${before:-0}" -gt 0 ] || die "clone has no already-rated CDRs (leg_a>0) to replay"

WORKDIR=$(mktemp -d "${TMPDIR:-/tmp}/re7_bench_rt.XXXXXX") || die "mktemp failed"
ln -s "$RE_MODULES" "$WORKDIR/modules"
mkdir -p "$WORKDIR/logs" "$WORKDIR/config/cdr_profiles"
LOGFILE="$WORKDIR/logs/rate_engine.log"
cat >"$WORKDIR/config/RateEngine7.xml" <<EOF
<RateEngine version="0.7.6">
 <System>
    <param name="DIR" value="$WORKDIR/" />
    <param name="PIDFile" value="logs/rate_engine.pid" />
 </System>
 <LoadModules>
$MODLINES
 </LoadModules>
 <DB>
    <param name="dbtype" value="pgsql" />
    <param name="dbhost" value="$DBHOST" />
    <param name="dbname" value="$BENCHDB" />
    <param name="dbuser" value="$DBUSER" />
    <param name="dbpass" value="$DBPASS" />
    <param name="dbport" value="$DBPORT" />
    <param name="NumberRetries" value="3" />
    <param name="IntervalRetries" value="1" />
 </DB>
 <Rating>
    <param name="active" value="yes" />
    <param name="RatingModule" value="$MODULE" />
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
    <param name="LogDebugLevel" value="1" />
 </Logs>
</RateEngine>
EOF

# reset the newest N rated CDRs on the CLONE so rt.so re-rates them
reset_cdrs() {
	q_bench "CREATE TEMP TABLE _replay AS
	           SELECT id FROM cdrs WHERE leg_a > 0 ORDER BY id DESC LIMIT $N;
	         DELETE FROM rating WHERE call_id IN (SELECT id FROM _replay);
	         UPDATE cdrs SET leg_a = 0 WHERE id IN (SELECT id FROM _replay);" >/dev/null \
		|| die "failed to reset replay CDRs on clone"
}

# one rating pass -> echoes "overall_ms_cdr cdr_s" for the pass (or empty on fail)
run_pass() {
	: >"$LOGFILE"
	(
		cd "$WORKDIR" && exec env LD_LIBRARY_PATH="$RE_LIBS:${LD_LIBRARY_PATH:-}" \
			"$RE_BIN" -c "$WORKDIR/config/RateEngine7.xml" -r a --debug 1
	) >"$WORKDIR/daemon.stdout" 2>&1 &
	RE_PID=$!
	local _ left
	for _ in $(seq 1 600); do
		kill -0 "$RE_PID" 2>/dev/null || break
		left=$(q_bench "SELECT count(*) FROM cdrs WHERE leg_a = 0;")
		[ "${left:-1}" = "0" ] && break
		sleep 0.5
	done
	kill -TERM "$RE_PID" 2>/dev/null; wait "$RE_PID" 2>/dev/null; RE_PID=""
	# with --debug 1 logs go to $LOGFILE (rt_log.c: level>=1 -> LogFile); merge both
	cat "$LOGFILE" "$WORKDIR/daemon.stdout" 2>/dev/null >"$RUNLOG"
	# Normalize per-batch/cycle timing to "total_sec cdrs" for BOTH engines:
	#   rt.so       -> "batch times: <total> total, <avg> avg/cdr, cdrs: <n>"
	#   rt_duckdb.so-> "cycle done: processed <n> (rated <r>) in <elapsed> sec ..."
	local norm
	norm=$( { grep -oE 'batch times: [0-9.]+ total, [0-9.]+ avg/cdr, cdrs: [0-9]+' "$RUNLOG" \
	            | sed -E 's/batch times: ([0-9.]+) total, [0-9.]+ avg\/cdr, cdrs: ([0-9]+)/\1 \2/';
	          grep -oE 'cycle done: processed [0-9]+ \(rated [0-9]+\) in [0-9.]+ sec' "$RUNLOG" \
	            | sed -E 's/cycle done: processed ([0-9]+) \(rated [0-9]+\) in ([0-9.]+) sec/\2 \1/'; } )
	[ -n "$norm" ] || return 1
	# overall ms/cdr and cdr/s across all batches/cycles of this pass
	printf '%s\n' "$norm" | awk '{t+=$1; c+=$2} END{ if(c>0) printf "%.3f %.1f", t*1000/c, c/t }'
}

median() { printf '%s\n' "$@" | sort -n | awk '{a[NR]=$1} END{ if(NR==0) exit; if(NR%2) printf "%.3f",a[(NR+1)/2]; else printf "%.3f",(a[NR/2]+a[NR/2+1])/2 }'; }

# --- run REPEAT passes on the SAME warm clone; VACUUM between to reset bloat --
RUNLOG="$WORKDIR/run.log"
echo "  rating on the clone with $MODULE (threads=$THREADS), $REPEAT pass(es) ..."
MS=(); CPS=()
for p in $(seq 1 "$REPEAT"); do
	reset_cdrs
	res=$(run_pass) || { FAILKEEP=1; echo "  pass $p: no 'batch times' produced"; \
		tail -n 15 "$LOGFILE" "$WORKDIR/daemon.stdout" 2>/dev/null | sed 's/^/    /'; exit 1; }
	pms=${res% *}; pcps=${res#* }
	MS+=("$pms"); CPS+=("$pcps")
	printf "    pass %d/%d: %s ms/cdr  (%s cdr/s)\n" "$p" "$REPEAT" "$pms" "$pcps"
	# reset write-bloat from this pass so the next pass starts clean
	[ "$p" -lt "$REPEAT" ] && psql_c -d "$BENCHDB" -c "VACUUM ANALYZE cdrs, rating, balance;" >/dev/null 2>&1 || true
done

med_ms=$(median "${MS[@]}")
mn=$(printf '%s\n' "${MS[@]}" | sort -n | head -1); mx=$(printf '%s\n' "${MS[@]}" | sort -n | tail -1)
med_cps=$(median "${CPS[@]}")

echo
echo "================= rating perf ($LABEL) ================="
printf "  passes=%d  threads=%d  N=%d\n" "$REPEAT" "$THREADS" "$N"
printf "  ms/cdr : median %s  (min %s  max %s)\n" "$med_ms" "$mn" "$mx"
printf "  cdr/s  : median %s\n" "$med_cps"
echo "======================================================="

# record the MEDIAN row (stable across-pass number) to the TSV
BASELINE_TSV="$HERE/baseline_results.tsv"
[ -f "$BASELINE_TSV" ] || printf 'label\tpasses\tthreads\tN\tmedian_ms\tmin_ms\tmax_ms\tmedian_cdr_s\n' >"$BASELINE_TSV"
printf '%s\t%d\t%d\t%d\t%s\t%s\t%s\t%s\n' "$LABEL" "$REPEAT" "$THREADS" "$N" "$med_ms" "$mn" "$mx" "$med_cps" >>"$BASELINE_TSV"
echo "  recorded median as \"$LABEL\" in $BASELINE_TSV"
