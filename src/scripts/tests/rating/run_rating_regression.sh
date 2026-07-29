#!/usr/bin/env bash
#
# run_rating_regression.sh - offline-rating golden regression for rt.so.
#
# Loads a SYNTHETIC schema+fixture into a THROWAWAY database, rates a set of
# known CDRs with the offline Rating engine (rt.so), and asserts the produced
# per-CDR price/billed-seconds against hand-computed golden values.
#
#   * Uses a dedicated test database (created + dropped here) - it NEVER touches
#     the engine's real database.
#   * Schema: src/scripts/sql/rt_pgsql.sql (structure + generic lookups only).
#   * Data:   fixture.sql + cdrs_seed.sql in this directory (all invented).
#   * Golden: golden.tsv in this directory.
#
# This is the rt.so-vs-golden pass; rt_duckdb.so parity is a later addition.
#
# Prerequisites:
#   * RateEngine installed under $RE_PREFIX (default /usr/local/RateEngine):
#       bin/RateEngine, libs/libre7core.so, modules/{pgsql,cdrm,rt}.so
#   * A reachable PostgreSQL the DB user can CREATE/DROP a database on.
#   * psql in PATH.
#
# Env (optional; DB params default to the installed engine config, then to the
# CI Postgres): RE_PREFIX, RE_CONF, DBHOST, DBNAME, DBUSER, DBPASS, DBPORT,
#               TESTDB (name of the throwaway db, default re7_rating_test).
#
# Exit: 0 all golden matches, 1 a mismatch/failure, 2 prerequisites missing.

set -u

HERE=$(cd "$(dirname "$0")" && pwd)
REPO_SRC=$(cd "$HERE/../../.." && pwd)           # rating -> tests -> scripts -> src
SCHEMA_SQL="$REPO_SRC/scripts/sql/rt_pgsql.sql"
FIXTURE_SQL="$HERE/fixture.sql"
CDRS_SQL="$HERE/cdrs_seed.sql"
GOLDEN_TSV="$HERE/golden.tsv"

RE_PREFIX=${RE_PREFIX:-/usr/local/RateEngine}
RE_BIN=${RE_BIN:-$RE_PREFIX/bin/RateEngine}
RE_LIBS=${RE_LIBS:-$RE_PREFIX/libs}
RE_MODULES=${RE_MODULES:-$RE_PREFIX/modules}
RE_CONF=${RE_CONF:-$RE_PREFIX/config/RateEngine7.xml}

# DB params: explicit env -> installed config <DB> block -> CI defaults.
db_from_conf() {
	[ -f "$RE_CONF" ] || return 0
	sed -n '/<DB>/,/<\/DB>/p' "$RE_CONF" 2>/dev/null |
		sed -nE "s/.*name=\"$1\"[^>]*value=\"([^\"]*)\".*/\1/p" | head -1
}
DBHOST=${DBHOST:-$(db_from_conf dbhost)}; DBHOST=${DBHOST:-127.0.0.1}
DBNAME=${DBNAME:-$(db_from_conf dbname)}; DBNAME=${DBNAME:-rate_engine}
DBUSER=${DBUSER:-$(db_from_conf dbuser)}; DBUSER=${DBUSER:-re_admin}
DBPASS=${DBPASS:-$(db_from_conf dbpass)}; DBPASS=${DBPASS:-_cfg.access}
DBPORT=${DBPORT:-$(db_from_conf dbport)}; DBPORT=${DBPORT:-5432}
TESTDB=${TESTDB:-re7_rating_test}
export PGPASSWORD="$DBPASS"

WORKDIR=""
RE_PID=""
LOGFILE=""
DB_CREATED=""
PASS=0
FAIL=0

pass() { PASS=$((PASS + 1)); echo "  PASS: $1"; }
fail() { FAIL=$((FAIL + 1)); echo "  FAIL: $1"; }

# psql helpers. adm = maintenance connection (to the existing DB, only for
# CREATE/DROP DATABASE); q = query the test DB, tuples-only unaligned.
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

cleanup() {
	[ -n "$RE_PID" ] && { kill -TERM "$RE_PID" 2>/dev/null; wait "$RE_PID" 2>/dev/null; }
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
	6) base="$ym-17" ;; # Sat -> Mon
	7) base="$ym-16" ;; # Sun -> Mon
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

	WORKDIR=$(mktemp -d "${TMPDIR:-/tmp}/re7_rating.XXXXXX") || die "mktemp failed"
	ln -s "$RE_MODULES" "$WORKDIR/modules"
	mkdir -p "$WORKDIR/logs" "$WORKDIR/config/cdr_profiles"
	LOGFILE="$WORKDIR/logs/rate_engine.log"
}

build_db() {
	echo "run_rating: (re)creating throwaway db '$TESTDB' on $DBHOST:$DBPORT (user $DBUSER)"
	psql_adm -c "DROP DATABASE IF EXISTS $TESTDB;" >/dev/null 2>&1 || true
	psql_adm -c "CREATE DATABASE $TESTDB;" >/dev/null 2>&1 ||
		die "could not CREATE DATABASE $TESTDB (does $DBUSER have CREATEDB? is $DBHOST reachable?)"
	DB_CREATED=1
	psql_test -q -f "$SCHEMA_SQL" >/dev/null || die "failed to load schema $SCHEMA_SQL"
	# rt_pgsql.sql ships 'db_screenshot' change-tracking RULES (DELETE+INSERT of
	# the table name) that collide with multi-row seed inserts (UNIQUE tbl_name).
	# The offline rater doesn't read db_screenshot, so drop these rules in the
	# throwaway DB to let the fixture load cleanly.
	psql_test -q -c "DO \$\$ DECLARE r record; BEGIN
	  FOR r IN SELECT tablename, rulename FROM pg_rules WHERE rulename LIKE 'db_screenshot%'
	  LOOP EXECUTE format('DROP RULE %I ON public.%I', r.rulename, r.tablename); END LOOP;
	END \$\$;" >/dev/null || die "failed to drop db_screenshot rules"
	psql_test -q -f "$FIXTURE_SQL" >/dev/null || die "failed to load fixture $FIXTURE_SQL"
	local ts
	ts=$(weekday_ts)
	echo "run_rating: seeding CDRs with call_ts=$ts"
	psql_test -q -v call_ts="$ts" -f "$CDRS_SQL" >/dev/null || die "failed to seed CDRs $CDRS_SQL"
}

gen_config() {
	cat >"$WORKDIR/config/RateEngine7.xml" <<EOF
<RateEngine version="0.7.6">
 <System>
    <param name="DIR" value="$WORKDIR/" />
    <param name="PIDFile" value="logs/rate_engine.pid" />
 </System>
 <LoadModules>
    <param name="module" value="pgsql.so" />
    <param name="module" value="cdrm.so" />
    <param name="module" value="rt.so" />
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
    <param name="leg" value="a" />
    <param name="RatingInterval" value="300" />
    <param name="WaitRatingInterval" value="500" />
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

# Rate leg a: rt.so rates one batch (active=no) then the process idles in the
# keeper loop, so we run it in the background, poll until no unrated CDR
# remains, then stop it.
run_rating() {
	local cfg="$WORKDIR/config/RateEngine7.xml" i unrated
	: >"$LOGFILE" 2>/dev/null || true
	(
		cd "$WORKDIR" &&
			exec env LD_LIBRARY_PATH="$RE_LIBS:${LD_LIBRARY_PATH:-}" \
				"$RE_BIN" -c "$cfg" -r a
	) >"$WORKDIR/daemon.stdout" 2>&1 &
	RE_PID=$!

	for i in $(seq 1 60); do          # up to ~30s
		if ! kill -0 "$RE_PID" 2>/dev/null; then
			break                     # process exited on its own
		fi
		unrated=$(q "SELECT count(*) FROM cdrs WHERE leg_a = 0;")
		[ "${unrated:-1}" = "0" ] && return 0
		sleep 0.5
	done
	unrated=$(q "SELECT count(*) FROM cdrs WHERE leg_a = 0;")
	[ "${unrated:-1}" = "0" ]
}

compare_golden() {
	echo "== offline rating: rt.so vs golden =="
	local total rated
	total=$(q "SELECT count(*) FROM cdrs;")
	rated=$(q "SELECT count(*) FROM cdrs WHERE leg_a > 0;")
	if [ "$rated" = "$total" ]; then
		pass "all $total CDRs rated (leg_a > 0)"
	else
		fail "only $rated/$total CDRs rated (leg_a > 0)"
	fi

	local uid want_price want_bs got
	while IFS=$'\t' read -r uid want_price want_bs; do
		case "$uid" in ''|\#*) continue ;; esac
		# per-CDR aggregate (a CDR may split into >1 rating rows).
		got=$(q "SELECT COALESCE(SUM(r.call_price),0)||'|'||COALESCE(SUM(r.call_billsec),0)
		           FROM cdrs c JOIN rating r ON r.call_id = c.id
		          WHERE c.call_uid = '$uid';")
		local got_price="${got%%|*}" got_bs="${got##*|}"
		if [ -z "$got" ] || [ "$got" = "|" ]; then
			fail "$uid: no rating row produced"
			continue
		fi
		# price within tolerance, billsec exact.
		awk -v gp="$got_price" -v wp="$want_price" 'BEGIN{d=gp-wp; if(d<0)d=-d; exit (d<0.005)?0:1}' &&
			pass "$uid: price $got_price ~= $want_price" ||
			fail "$uid: price $got_price != $want_price"
		[ "$got_bs" = "$want_bs" ] &&
			pass "$uid: billsec $got_bs == $want_bs" ||
			fail "$uid: billsec $got_bs != $want_bs"
	done <"$GOLDEN_TSV"
}

main() {
	trap cleanup EXIT
	setup
	echo "run_rating: engine=$RE_BIN  db=$DBHOST:$DBPORT testdb=$TESTDB"
	build_db
	gen_config
	if ! run_rating; then
		dump_logs
		die "rating did not complete (CDRs still unrated) - see log above"
	fi
	compare_golden

	echo
	echo "================= rating regression summary ================="
	echo "  PASS=$PASS  FAIL=$FAIL"
	echo "============================================================="
	[ "$FAIL" -eq 0 ]
}

main "$@"
