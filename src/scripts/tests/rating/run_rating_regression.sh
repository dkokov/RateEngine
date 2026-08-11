#!/usr/bin/env bash
#
# run_rating_regression.sh - offline-rating regression for rt.so and rt_duckdb.so.
#
# Loads a SYNTHETIC schema+fixture into a THROWAWAY database, rates a set of
# known CDRs with the offline Rating engine, and asserts the produced per-CDR
# price/billed-seconds against hand-computed golden values. Runs each available
# engine on its OWN fresh load of the fixture, then:
#   * asserts rt.so        == golden
#   * asserts rt_duckdb.so == golden        (skipped if duckdb not installed)
#   * asserts rt.so        == rt_duckdb.so   (parity)
#
#   * Uses a dedicated test database (created + dropped here) - it NEVER touches
#     the engine's real database.
#   * Schema: src/scripts/sql/rt_pgsql_v2.sql (structure + generic lookups only).
#   * Data:   fixture.sql + cdrs_seed.sql in this directory (all invented).
#   * Golden: golden.tsv in this directory.
#
# Prerequisites:
#   * RateEngine installed under $RE_PREFIX (default /usr/local/RateEngine):
#       bin/RateEngine, libs/libre7core.so, modules/{pgsql,cdrm,rt}.so
#       (and modules/{duckdb,rt_duckdb}.so for the DuckDB parity pass).
#   * A reachable PostgreSQL the DB user can CREATE/DROP a database on.
#   * psql in PATH.
#
# Env (optional; DB params default to the installed engine config, then to the
# CI Postgres): RE_PREFIX, RE_CONF, DBHOST, DBNAME, DBUSER, DBPASS, DBPORT,
#               TESTDB (throwaway db name, default re7_rating_test).
#
# Exit: 0 all checks pass, 1 a mismatch/failure, 2 prerequisites missing.

set -u

HERE=$(cd "$(dirname "$0")" && pwd)
REPO_SRC=$(cd "$HERE/../../.." && pwd)           # rating -> tests -> scripts -> src
SCHEMA_SQL="$REPO_SRC/scripts/sql/rt_pgsql_v2.sql"
FIXTURE_SQL="$HERE/fixture.sql"
CDRS_SQL="$HERE/cdrs_seed.sql"
GOLDEN_TSV="$HERE/golden.tsv"
INVARIANTS="$HERE/check_rating_invariants.sh"

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
DBHOST=${DBHOST:-$(db_from_conf dbhost)}; DBHOST=${DBHOST:-127.0.0.1}
DBNAME=${DBNAME:-$(db_from_conf dbname)}; DBNAME=${DBNAME:-rate_engine}
DBUSER=${DBUSER:-$(db_from_conf dbuser)}; DBUSER=${DBUSER:-re_admin}
DBPASS=${DBPASS:-$(db_from_conf dbpass)}; DBPASS=${DBPASS:-change_me}
DBPORT=${DBPORT:-$(db_from_conf dbport)}; DBPORT=${DBPORT:-5432}
TESTDB=${TESTDB:-re7_rating_test}
export PGPASSWORD="$DBPASS"

WORKDIR=""
RE_PID=""
LOGFILE=""
DB_CREATED=""
HAVE_DUCKDB=""
PASS=0
FAIL=0

pass() { PASS=$((PASS + 1)); echo "  PASS: $1"; }
fail() { FAIL=$((FAIL + 1)); echo "  FAIL: $1"; }
note() { echo "  INFO: $*"; }

psql_adm() { psql -v ON_ERROR_STOP=1 -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$DBNAME" "$@"; }
psql_test() { psql -v ON_ERROR_STOP=1 -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$TESTDB" "$@"; }
q() { psql -tA -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$TESTDB" -c "$1" 2>/dev/null; }

die() {
	echo "run_rating: $*" >&2
	dump_logs
	exit 2
}

dump_logs() {
	if [ -n "$LOGFILE" ] && [ -f "$LOGFILE" ]; then
		echo "----- rate_engine.log (tail) -----" >&2
		tail -n 60 "$LOGFILE" >&2 || true
	fi
	if [ -n "$WORKDIR" ] && [ -f "$WORKDIR/daemon.stdout" ]; then
		echo "----- daemon.stdout (tail) -----" >&2
		tail -n 30 "$WORKDIR/daemon.stdout" >&2 || true
	fi
}

re_stop() {
	[ -n "$RE_PID" ] || return 0
	kill -TERM "$RE_PID" 2>/dev/null || true
	wait "$RE_PID" 2>/dev/null || true
	RE_PID=""
}

cleanup() {
	re_stop
	if [ -n "$DB_CREATED" ]; then
		psql_adm -c "DROP DATABASE IF EXISTS $TESTDB;" >/dev/null 2>&1 || true
	fi
	[ -n "$WORKDIR" ] && rm -rf "$WORKDIR"
}

# A mid-month weekday timestamp: weekday satisfies the mon-sun time-condition,
# mid-month keeps it inside the current billing period (pcard validity).
weekday_ts() {
	local ym base dow
	ym=$(date +%Y-%m)
	base="$ym-15"
	dow=$(date -d "$base" +%u 2>/dev/null || echo 3) # 1=Mon..7=Sun
	case "$dow" in
	6) base="$ym-17" ;;
	7) base="$ym-16" ;;
	esac
	echo "$base 10:00:00"
}

setup() {
	[ -x "$RE_BIN" ] || die "RateEngine not found/executable: $RE_BIN (install it or set RE_PREFIX)"
	[ -d "$RE_MODULES" ] || die "modules dir not found: $RE_MODULES"
	for m in pgsql.so cdrm.so rt.so; do
		[ -e "$RE_MODULES/$m" ] || die "module $m not installed in $RE_MODULES (build/install it)"
	done
	[ -f "$SCHEMA_SQL" ] || die "schema not found: $SCHEMA_SQL"
	command -v psql >/dev/null || die "psql not found in PATH"

	# DuckDB parity pass is optional - only if its modules are installed.
	if [ -e "$RE_MODULES/duckdb.so" ] && [ -e "$RE_MODULES/rt_duckdb.so" ]; then
		HAVE_DUCKDB=1
	fi

	WORKDIR=$(mktemp -d "${TMPDIR:-/tmp}/re7_rating.XXXXXX") || die "mktemp failed"
	ln -s "$RE_MODULES" "$WORKDIR/modules"
	mkdir -p "$WORKDIR/logs" "$WORKDIR/config/cdr_profiles"
	LOGFILE="$WORKDIR/logs/rate_engine.log"
}

# (Re)create the throwaway DB and load schema + synthetic fixture + CDRs.
build_db() {
	psql_adm -c "DROP DATABASE IF EXISTS $TESTDB;" >/dev/null 2>&1 || true
	psql_adm -c "CREATE DATABASE $TESTDB;" >/dev/null 2>&1 ||
		die "could not CREATE DATABASE $TESTDB (does $DBUSER have CREATEDB? is $DBHOST reachable?)"
	DB_CREATED=1
	psql_test -q -f "$SCHEMA_SQL" >/dev/null || die "failed to load schema $SCHEMA_SQL"
	# rt_pgsql_v2.sql ships 'db_screenshot' change-tracking RULES (DELETE+INSERT of
	# the table name) that collide with multi-row seed inserts (UNIQUE tbl_name).
	# The offline rater doesn't read db_screenshot, so drop these rules.
	psql_test -q -c "DO \$\$ DECLARE r record; BEGIN
	  FOR r IN SELECT tablename, rulename FROM pg_rules WHERE rulename LIKE 'db_screenshot%'
	  LOOP EXECUTE format('DROP RULE %I ON public.%I', r.rulename, r.tablename); END LOOP;
	END \$\$;" >/dev/null || die "failed to drop db_screenshot rules"
	psql_test -q -f "$FIXTURE_SQL" >/dev/null || die "failed to load fixture $FIXTURE_SQL"
	local ts
	ts=$(weekday_ts)
	psql_test -q -v call_ts="$ts" -f "$CDRS_SQL" >/dev/null || die "failed to seed CDRs $CDRS_SQL"
}

# gen_config ENGINE  (rt.so | rt_duckdb.so) - offline rating config for one engine.
gen_config() {
	local engine=$1 modules rating_mod=""
	if [ "$engine" = "rt_duckdb.so" ]; then
		modules='    <param name="module" value="pgsql.so" />
    <param name="module" value="cdrm.so" />
    <param name="module" value="duckdb.so" />
    <param name="module" value="rt_duckdb.so" />'
		rating_mod='    <param name="RatingModule" value="rt_duckdb.so" />'
	else
		modules='    <param name="module" value="pgsql.so" />
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
$modules
 </LoadModules>
 <DB>
    <param name="dbtype" value="pgsql" />
    <param name="dbhost" value="$DBHOST" />
    <param name="dbname" value="$TESTDB" />
    <param name="dbuser" value="$DBUSER" />
    <param name="dbpass" value="$DBPASS" />
    <param name="dbport" value="$DBPORT" />
    <param name="NumberRetries" value="3" />
    <param name="IntervalRetries" value="1" />
 </DB>
 <Rating>
    <param name="active" value="no" />
$rating_mod
    <param name="leg" value="a" />
    <param name="RatingInterval" value="300" />
    <param name="WaitRatingInterval" value="500" />
    <param name="BatchLimit" value="5000" />
    <param name="UsePCard" value="no" />
    <param name="BillingDay" value="01" />
 </Rating>
 <CDRMediator>
    <param name="CDRProfilesDIR" value="$WORKDIR/config/cdr_profiles/" />
 </CDRMediator>
 <Logs>
    <param name="LogFile" value="logs/rate_engine.log" />
    <param name="LogMaxFileSize" value="40960000" />
    <param name="LogSeparator" value="|" />
    <param name="LogDateFormat" value="" />
    <param name="LogDebugLevel" value="3" />
 </Logs>
</RateEngine>
EOF
}

# Rate leg a: the engine rates one batch (active=no) then idles in the keeper
# loop, so run it in the background, poll until no unrated CDR remains, stop it.
run_rating() {
	local cfg="$WORKDIR/config/RateEngine7.xml" i unrated
	: >"$LOGFILE" 2>/dev/null || true
	(
		cd "$WORKDIR" &&
			exec env LD_LIBRARY_PATH="$RE_LIBS:${LD_LIBRARY_PATH:-}" \
				"$RE_BIN" -c "$cfg" -r a
	) >"$WORKDIR/daemon.stdout" 2>&1 &
	RE_PID=$!

	for i in $(seq 1 60); do
		kill -0 "$RE_PID" 2>/dev/null || break
		unrated=$(q "SELECT count(*) FROM cdrs WHERE leg_a = 0;")
		[ "${unrated:-1}" = "0" ] && return 0
		sleep 0.5
	done
	unrated=$(q "SELECT count(*) FROM cdrs WHERE leg_a = 0;")
	[ "${unrated:-1}" = "0" ]
}

# capture_results OUTFILE - one line per CDR: call_uid|sum_price|sum_billsec
# (LEFT JOIN so an unrated CDR shows as 0|0 rather than vanishing).
capture_results() {
	# PAID and FREE portions kept SEPARATE - never summed. An allowance-boundary
	# call yields two rating rows (free = negative price, paid = positive), and
	# SUM()ing them nets one against the other: two different splits with the same
	# net would compare equal. Fields: uid|paid_price|free_price|paid_bsec|free_bsec
	q "SELECT c.call_uid
	     ||'|'||COALESCE(SUM(r.call_price)   FILTER (WHERE r.call_price > 0),0)
	     ||'|'||COALESCE(SUM(r.call_price)   FILTER (WHERE r.call_price < 0),0)
	     ||'|'||COALESCE(SUM(r.call_billsec) FILTER (WHERE r.call_price > 0),0)
	     ||'|'||COALESCE(SUM(r.call_billsec) FILTER (WHERE r.call_price <= 0),0)
	     FROM cdrs c LEFT JOIN rating r ON r.call_id = c.id
	    GROUP BY c.call_uid ORDER BY c.call_uid;" >"$1"
}

# rate_engine ENGINE OUTFILE - fresh DB, config, rate, capture, stop.
rate_engine() {
	local engine=$1 out=$2
	build_db
	gen_config "$engine"
	if ! run_rating; then
		dump_logs
		die "rating with $engine did not complete (CDRs still unrated) - see log above"
	fi
	capture_results "$out"
	re_stop
}

# res_get FILE UID -> "paid_price|free_price|paid_bsec|free_bsec" for that CDR.
res_get() { awk -F'|' -v u="$2" '$1==u{print $2"|"$3"|"$4"|"$5}' "$1"; }

# num_eq A B TOL -> 0 when |A-B| < TOL
num_eq() { awk -v a="$1" -v b="$2" -v t="$3" 'BEGIN{d=a-b;if(d<0)d=-d;exit (d<t)?0:1}'; }

# check_invariants LABEL - money-correctness assertions on what rating LEFT
# BEHIND (balance + free_billsec_balance), which golden.tsv cannot see: a
# free-billsec call is supposed to carry a negative call_price, so the rating
# rows stayed correct while the balance was charged that negative and the ledger
# collapsed onto balance_id = 0. Runs against the throwaway TESTDB.
check_invariants() {
	local label=$1 line
	echo "== $label invariants =="
	if [ ! -x "$INVARIANTS" ]; then
		note "$INVARIANTS not executable - skipping invariant checks"
		return
	fi
	# Feed each PASS/FAIL back through our own counters so one summary covers
	# golden, parity and invariants. VERBOSE=1 makes the checker print the
	# offending rows under a failure (indented); pass those straight through -
	# without them a FAIL says "1 offending row" and nothing about WHICH row.
	while IFS= read -r line; do
		case "$line" in
			*"  PASS: "*) pass "$label ${line#*PASS: }" ;;
			*"  FAIL: "*) fail "$label ${line#*FAIL: }" ;;
			*"  INFO: "*) note "$label ${line#*INFO: }" ;;
			"         "*) echo "$line" ;;
		esac
	done < <(DBHOST="$DBHOST" DBPORT="$DBPORT" DBUSER="$DBUSER" DBPASS="$DBPASS" \
	         DBNAME="$TESTDB" VERBOSE=1 "$INVARIANTS" 2>&1)
}

# compare_golden RESULTS_FILE LABEL
# golden.tsv columns: uid <TAB> paid_price <TAB> free_price <TAB> paid_bsec <TAB> free_bsec
compare_golden() {
	local res=$1 label=$2 uid w_pp w_fp w_pb w_fb got g_pp g_fp g_pb g_fb
	echo "== $label vs golden =="
	while IFS=$'\t' read -r uid w_pp w_fp w_pb w_fb; do
		case "$uid" in ''|\#*) continue ;; esac
		got=$(res_get "$res" "$uid")
		if [ -z "$got" ]; then
			fail "$label $uid: no result row"
			continue
		fi
		IFS='|' read -r g_pp g_fp g_pb g_fb <<<"$got"

		num_eq "$g_pp" "$w_pp" 0.005 &&
			pass "$label $uid: paid price $g_pp ~= $w_pp" ||
			fail "$label $uid: paid price $g_pp != $w_pp"
		num_eq "$g_fp" "$w_fp" 0.005 &&
			pass "$label $uid: free price $g_fp ~= $w_fp" ||
			fail "$label $uid: free price $g_fp != $w_fp"
		[ "$g_pb" = "$w_pb" ] &&
			pass "$label $uid: paid billsec $g_pb == $w_pb" ||
			fail "$label $uid: paid billsec $g_pb != $w_pb"
		[ "$g_fb" = "$w_fb" ] &&
			pass "$label $uid: free billsec $g_fb == $w_fb" ||
			fail "$label $uid: free billsec $g_fb != $w_fb"
	done <"$GOLDEN_TSV"
}

# compare_parity FILE_A FILE_B - per-CDR equality between two engines.
compare_parity() {
	local a=$1 b=$2 uid a_pp a_fp a_pb a_fb pb b_pp b_fp b_pb b_fb
	echo "== rt.so vs rt_duckdb.so parity =="
	while IFS='|' read -r uid a_pp a_fp a_pb a_fb; do
		pb=$(res_get "$b" "$uid")
		IFS='|' read -r b_pp b_fp b_pb b_fb <<<"$pb"

		num_eq "$a_pp" "$b_pp" 0.005 &&
			pass "$uid: paid price parity ($a_pp)" ||
			fail "$uid: paid price differs (rt=$a_pp duckdb=$b_pp)"
		num_eq "$a_fp" "$b_fp" 0.005 &&
			pass "$uid: free price parity ($a_fp)" ||
			fail "$uid: free price differs (rt=$a_fp duckdb=$b_fp)"
		[ "$a_pb" = "$b_pb" ] &&
			pass "$uid: paid billsec parity ($a_pb)" ||
			fail "$uid: paid billsec differs (rt=$a_pb duckdb=$b_pb)"
		[ "$a_fb" = "$b_fb" ] &&
			pass "$uid: free billsec parity ($a_fb)" ||
			fail "$uid: free billsec differs (rt=$a_fb duckdb=$b_fb)"
	done <"$a"
}

main() {
	trap cleanup EXIT
	setup
	echo "run_rating: engine=$RE_BIN  db=$DBHOST:$DBPORT testdb=$TESTDB  duckdb=${HAVE_DUCKDB:-no}"

	local res_rt="$WORKDIR/res_rt.tsv" res_duck="$WORKDIR/res_duckdb.tsv"

	rate_engine "rt.so" "$res_rt"
	compare_golden "$res_rt" "rt.so"
	check_invariants "rt.so"

	if [ -n "$HAVE_DUCKDB" ]; then
		rate_engine "rt_duckdb.so" "$res_duck"
		compare_golden "$res_duck" "rt_duckdb.so"
		check_invariants "rt_duckdb.so"
		compare_parity "$res_rt" "$res_duck"
	else
		note "duckdb.so / rt_duckdb.so not installed - skipping DuckDB parity pass"
	fi

	echo
	local duck_state=skipped
	[ -n "$HAVE_DUCKDB" ] && duck_state=yes
	echo "================= rating regression summary ================="
	echo "  PASS=$PASS  FAIL=$FAIL  (duckdb parity pass: $duck_state)"
	echo "============================================================="
	[ "$FAIL" -eq 0 ]
}

main "$@"
