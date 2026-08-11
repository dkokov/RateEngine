#!/usr/bin/env bash
#
# check_invariants_selftest.sh - negative test for check_rating_invariants.sh.
#
# Builds a tiny THROWAWAY database with a known-good dataset, asserts the
# invariant checker reports it clean, then injects one deliberate fault at a
# time and asserts the matching invariant fires. Without this, a checker that
# silently always passes would look identical to a healthy system.
#
# Each fault reproduces a real defect:
#   1  free-marked negative price charged to the balance   (the V7 regression)
#   2  balance driven negative by free calls
#   3  ledger keyed to balance_id = 0                      (pre->bal_id never set)
#   4  ledger frozen / drifted from the rating rows
#   5  free allowance never depletes
#   6  duplicate balance rows / duplicate ledger keys      (concurrency)
#   7  rated period with no balance row                    (rt_duckdb bal_delta)
#   8  consumed free seconds with no ledger row            (rt_duckdb balance_id IS NOT NULL)
#
# A fault may legitimately trip more than one invariant; the test only requires
# that the EXPECTED one fires.
#
# Env: DBHOST, DBUSER, DBPASS, DBPORT (admin DB for CREATE DATABASE: DBNAME),
#      SELFTEST_DB (throwaway name, default re7_inv_selftest).
#
# Exit: 0 the checker detects every fault, 1 it missed one, 2 prerequisites missing.

set -u

HERE=$(cd "$(dirname "$0")" && pwd)
CHECKER="$HERE/check_rating_invariants.sh"

RE_PREFIX=${RE_PREFIX:-/usr/local/RateEngine}
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
SELFTEST_DB=${SELFTEST_DB:-re7_inv_selftest}
export PGPASSWORD="$DBPASS"

PASS=0
FAIL=0
DB_CREATED=""

pass() { PASS=$((PASS + 1)); echo "  PASS: $1"; }
fail() { FAIL=$((FAIL + 1)); echo "  FAIL: $1"; }

[ -x "$CHECKER" ] || { echo "selftest: $CHECKER not executable" >&2; exit 2; }
command -v psql >/dev/null 2>&1 || { echo "selftest: psql not in PATH" >&2; exit 2; }

adm() { psql -tA -v ON_ERROR_STOP=1 -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$DBNAME" -c "$1" >/dev/null 2>&1; }
t()   { psql -q -v ON_ERROR_STOP=1 -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$SELFTEST_DB" -c "$1" >/dev/null 2>&1; }

cleanup() {
	[ -n "$DB_CREATED" ] || return 0
	psql -tA -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$DBNAME" \
		-c "DROP DATABASE IF EXISTS $SELFTEST_DB;" >/dev/null 2>&1 || true
}
trap cleanup EXIT

adm "DROP DATABASE IF EXISTS $SELFTEST_DB;" ||
	{ echo "selftest: cannot administer $DBUSER@$DBHOST:$DBPORT/$DBNAME" >&2; exit 2; }
adm "CREATE DATABASE $SELFTEST_DB;" ||
	{ echo "selftest: CREATE DATABASE $SELFTEST_DB failed (needs CREATEDB)" >&2; exit 2; }
DB_CREATED=1

# Minimal schema - only the four tables the invariants touch, with the column
# types that matter (balance start/end are VARCHAR, as in rt_pgsql_v2.sql; that
# is exactly what forces the ::timestamp casts in the checker).
psql -q -v ON_ERROR_STOP=1 -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$SELFTEST_DB" >/dev/null 2>&1 <<'SQL' || { echo "selftest: schema load failed" >&2; exit 2; }
CREATE TABLE free_billsec (id integer PRIMARY KEY, free_billsec integer);
CREATE TABLE balance (id serial PRIMARY KEY, billing_account_id integer,
  amount numeric DEFAULT 0, last_update timestamp DEFAULT now(),
  start_date varchar(32), end_date varchar(32), active boolean DEFAULT true);
CREATE TABLE rating (id serial PRIMARY KEY, call_price numeric, call_billsec integer,
  rate_id integer DEFAULT 1, billing_account_id integer, rating_mode_id integer DEFAULT 1,
  call_id integer DEFAULT 0, time_condition_id integer DEFAULT 0, pcard_id integer DEFAULT 0,
  call_ts timestamp, last_update timestamp DEFAULT now(), free_billsec_id integer DEFAULT 0);
CREATE TABLE free_billsec_balance (id serial PRIMARY KEY, balance_id integer,
  tariff_id integer DEFAULT 0, free_billsec integer DEFAULT 0,
  last_update timestamp DEFAULT now(), free_billsec_id integer DEFAULT 0);
-- tariff + calc_function are needed by invariant 5: the allowance may legally be
-- exceeded by the tier rounding of the boundary call, so the check tolerates one
-- first-tier block. delta_time = 1 here -> tolerance 0, keeping the fault strict.
CREATE TABLE tariff (id integer PRIMARY KEY, free_billsec_id integer DEFAULT 0);
CREATE TABLE calc_function (id serial PRIMARY KEY, tariff_id integer, pos integer,
  delta_time integer, fee numeric, iterations integer);
INSERT INTO free_billsec VALUES (19, 1000);
INSERT INTO tariff VALUES (1, 19);
INSERT INTO calc_function (tariff_id,pos,delta_time,fee,iterations) VALUES (1,1,1,0.02,0);
SELECT setval(pg_get_serial_sequence('balance','id'), 1000);
SQL

# Known-good dataset: one paid call (10.00) and one free-marked call (-2.00, 60s
# drawn from a 1000s allowance). Balance carries ONLY the paid price.
reset_db() {
	t "TRUNCATE balance, rating, free_billsec_balance;
	   INSERT INTO balance (id,billing_account_id,amount,start_date,end_date)
	     VALUES (100,1,10.0,'2024-01-01','2024-02-01');
	   INSERT INTO rating (billing_account_id,call_price,call_billsec,call_ts,free_billsec_id,pcard_id)
	     VALUES (1,10.0,300,'2024-01-05',0,7),(1,-2.0,60,'2024-01-06',19,7);
	   INSERT INTO free_billsec_balance (balance_id,free_billsec_id,free_billsec)
	     VALUES (100,19,60);"
}

# failed_invariants -> the numeric ids of every invariant that reported FAIL
failed_invariants() {
	DBHOST="$DBHOST" DBPORT="$DBPORT" DBUSER="$DBUSER" DBPASS="$DBPASS" DBNAME="$SELFTEST_DB" \
		"$CHECKER" 2>&1 | sed -nE 's/^  FAIL: ([0-9]+[ab]?)\..*/\1/p' | tr '\n' ' '
}

echo "check_invariants_selftest: $DBUSER@$DBHOST:$DBPORT db=$SELFTEST_DB"
echo

# ---- the healthy dataset must come back clean -------------------------------
reset_db
got=$(failed_invariants)
if [ -z "$got" ]; then
	pass "healthy dataset reports no violations"
else
	fail "healthy dataset reported violations: $got"
fi

# ---- each fault must trip its invariant -------------------------------------
# want<TAB>description<TAB>fault SQL
run_fault() {
	local want=$1 desc=$2 sql=$3 got
	reset_db
	t "$sql"
	got=$(failed_invariants)
	case " $got " in
		*" $want "*) pass "fault '$desc' -> invariant $want fired${got:+ (all: ${got% })}" ;;
		"  ")        fail "fault '$desc' -> NOTHING fired (expected $want)" ;;
		*)           fail "fault '$desc' -> expected $want, got: ${got% }" ;;
	esac
}

run_fault 1 "free negative charged to balance" \
	"UPDATE balance SET amount = 8.0 WHERE id=100"

run_fault 2 "balance driven negative by free calls" \
	"UPDATE rating SET call_price = 0 WHERE call_price > 0; UPDATE balance SET amount = -2.0 WHERE id=100"

run_fault 3 "ledger keyed to balance_id = 0" \
	"UPDATE free_billsec_balance SET balance_id = 0 WHERE balance_id = 100"

run_fault 4 "ledger frozen / drifted from rating" \
	"UPDATE free_billsec_balance SET free_billsec = 47 WHERE balance_id = 100"

run_fault 5 "free allowance never depletes" \
	"INSERT INTO rating (billing_account_id,call_price,call_billsec,call_ts,free_billsec_id,pcard_id)
	   VALUES (1,-50.0,5000,'2024-01-07',19,7);
	 UPDATE free_billsec_balance SET free_billsec = 5060 WHERE balance_id = 100"

run_fault 6a "duplicate (account,period) balance rows" \
	"INSERT INTO balance (billing_account_id,amount,start_date,end_date)
	   VALUES (1,10.0,'2024-01-01','2024-02-01')"

run_fault 6b "duplicate (balance,fbid) ledger keys" \
	"INSERT INTO free_billsec_balance (balance_id,free_billsec_id,free_billsec)
	   VALUES (100,19,60)"

run_fault 7 "rated period with no balance row" \
	"DELETE FROM balance WHERE id = 100"

run_fault 8 "consumed free seconds with no ledger row" \
	"DELETE FROM free_billsec_balance"

echo
echo "============== invariant checker selftest summary =============="
echo "  PASS=$PASS  FAIL=$FAIL"
echo "==============================================================="

[ "$FAIL" -eq 0 ]
