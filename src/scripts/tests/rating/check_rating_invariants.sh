#!/usr/bin/env bash
#
# check_rating_invariants.sh - money-correctness invariants for a rated database.
#
# Asserts properties that must hold for ANY dataset after rating, so it needs no
# golden values and can be pointed at a throwaway test DB, a benchmark DB, or a
# production DB (it is strictly READ-ONLY - only SELECTs are issued).
#
# The invariants exist because the per-CDR golden regression cannot see these
# bugs: a free-billsec call is SUPPOSED to be stored with a negative call_price
# (that negative is the "covered by the allowance" marker), so the rating rows
# looked perfect while balance and free_billsec_balance were both wrong.
#
#   1  balance.amount == SUM(call_price > 0) for the period      (no free negatives charged)
#   2  no negative balance                                        (only with positive-fee tariffs; see ALLOW_NEGATIVE)
#   3  no free_billsec_balance row keyed to balance_id = 0        (ledger is per-balance, not global)
#   4  ledger free_billsec == derived SUM(call_billsec) of the period's free calls
#   5  no period consumes more free seconds than its allowance
#   6  no duplicate (account, period) balance rows and no duplicate (balance, fbid) ledger keys
#   7  every period with rated traffic HAS a balance row          (completeness)
#   8  every (balance, fbid) that consumed free seconds HAS a ledger row (completeness)
#
# 7 and 8 are what separate the engines: rt.so creates the period row for
# free-only periods, rt_duckdb historically did not, so its ledger came out
# nearly empty. Checking only 1-6 would call both engines correct.
#
# Env (all optional; DB params default to the installed engine config, then to
# the usual dev values):
#   RE_PREFIX, RE_CONF, DBHOST, DBNAME, DBUSER, DBPASS, DBPORT
#   ALLOW_NEGATIVE=1  tolerate negative balances (a tariff with negative fees, or
#                     real credits/refunds, makes invariant 2 meaningless)
#   PRICE_EPS         float tolerance for invariant 1 (default 0.000001)
#   VERBOSE=1         print up to $SAMPLE offending rows per failed invariant
#   SAMPLE            how many offenders to show (default 5)
#
# Exit: 0 all invariants hold, 1 at least one violated, 2 prerequisites missing.

set -u

HERE=$(cd "$(dirname "$0")" && pwd)

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
export PGPASSWORD="$DBPASS"

ALLOW_NEGATIVE=${ALLOW_NEGATIVE:-0}
PRICE_EPS=${PRICE_EPS:-0.000001}
VERBOSE=${VERBOSE:-0}
SAMPLE=${SAMPLE:-5}

PASS=0
FAIL=0

pass() { PASS=$((PASS + 1)); echo "  PASS: $1"; }
fail() { FAIL=$((FAIL + 1)); echo "  FAIL: $1"; }
note() { echo "  INFO: $*"; }

command -v psql >/dev/null 2>&1 || { echo "check_invariants: psql not in PATH" >&2; exit 2; }

q() { psql -tA -v ON_ERROR_STOP=1 -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$DBNAME" -c "$1" 2>&1; }

psql -tA -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$DBNAME" -c 'SELECT 1' >/dev/null 2>&1 ||
	{ echo "check_invariants: cannot connect to $DBUSER@$DBHOST:$DBPORT/$DBNAME" >&2; exit 2; }

# balance.start_date/end_date are varchar in the schema, so every join to
# rating.call_ts (timestamp) needs an explicit cast - without it PostgreSQL
# raises "operator does not exist: timestamp >= character varying".
PERIOD_JOIN="r.call_ts >= b.start_date::timestamp AND r.call_ts <= b.end_date::timestamp"

# recon: one row per balance, with what the amount SHOULD be and what the free
# (negative-priced) calls of that period add up to.
RECON="WITH recon AS (
  SELECT b.id, b.billing_account_id, b.start_date, b.end_date, b.amount AS engine_amount,
         COALESCE(SUM(r.call_price) FILTER (WHERE r.call_price > 0),0) AS should_be,
         COALESCE(SUM(r.call_price) FILTER (WHERE r.call_price < 0),0) AS free_marked
  FROM balance b
  LEFT JOIN rating r
    ON r.billing_account_id = b.billing_account_id AND $PERIOD_JOIN
  GROUP BY b.id, b.billing_account_id, b.start_date, b.end_date, b.amount
)"

# check NAME COUNT_SQL SAMPLE_SQL - passes when COUNT_SQL yields 0.
check() {
	local name=$1 count_sql=$2 sample_sql=${3:-}
	local n
	n=$(q "$count_sql")
	case "$n" in
		''|*[!0-9]*)
			fail "$name (query error: $n)"
			return
			;;
	esac
	if [ "$n" -eq 0 ]; then
		pass "$name"
	else
		fail "$name - $n offending row(s)"
		if [ "$VERBOSE" = "1" ] && [ -n "$sample_sql" ]; then
			q "$sample_sql" | sed 's/^/         /'
		fi
	fi
}

echo "check_rating_invariants: $DBUSER@$DBHOST:$DBPORT/$DBNAME"
echo

ratings=$(q "SELECT count(*) FROM rating")
balances=$(q "SELECT count(*) FROM balance")
ledger=$(q "SELECT count(*) FROM free_billsec_balance")
note "rating=$ratings  balance=$balances  free_billsec_balance=$ledger"
if [ "$ratings" = "0" ]; then
	note "no rating rows - nothing to check (did rating actually run?)"
fi
echo

# ---------------------------------------------------------------- invariant 1
# The money balance must equal the sum of the POSITIVE prices only. A negative
# call_price marks a free-billsec-covered call and must never move the balance
# (V6 enforced this with 'and call_price > 0' in bal_get_cp_sum).
check "1. balance.amount == SUM(call_price > 0)" \
	"$RECON SELECT count(*) FROM recon WHERE abs(engine_amount - should_be) > $PRICE_EPS" \
	"$RECON SELECT 'bal '||id||' acct '||billing_account_id||' '||start_date||'..'||end_date||
	        ' engine='||engine_amount||' should_be='||should_be||' free_marked='||free_marked
	   FROM recon WHERE abs(engine_amount - should_be) > $PRICE_EPS ORDER BY abs(engine_amount-should_be) DESC LIMIT $SAMPLE"

# ---------------------------------------------------------------- invariant 2
if [ "$ALLOW_NEGATIVE" = "1" ]; then
	note "2. negative balances - skipped (ALLOW_NEGATIVE=1)"
else
	check "2. no negative balance" \
		"SELECT count(*) FROM balance WHERE amount < 0" \
		"SELECT 'bal '||id||' acct '||billing_account_id||' amount='||amount
		   FROM balance WHERE amount < 0 ORDER BY amount LIMIT $SAMPLE"
fi

# ---------------------------------------------------------------- invariant 3
# free_billsec_balance is per (balance, free_billsec_id). balance_id = 0 means
# the ledger collapsed onto one global bucket - the pre->bal_id regression.
check "3. no ledger row keyed to balance_id = 0" \
	"SELECT count(*) FROM free_billsec_balance WHERE balance_id = 0" \
	"SELECT 'fbb '||id||' fbid='||free_billsec_id||' free_billsec='||free_billsec
	   FROM free_billsec_balance WHERE balance_id = 0 LIMIT $SAMPLE"

# ---------------------------------------------------------------- invariant 4
# The stored consumption must match what the rating rows actually say.
check "4. ledger free_billsec == derived consumption" \
	"SELECT count(*) FROM free_billsec_balance fbb
	   JOIN balance b ON b.id = fbb.balance_id
	   LEFT JOIN LATERAL (
	     SELECT COALESCE(SUM(r.call_billsec),0) AS derived FROM rating r
	      WHERE r.billing_account_id = b.billing_account_id AND $PERIOD_JOIN
	        AND r.call_price < 0 AND r.free_billsec_id = fbb.free_billsec_id
	   ) d ON true
	  WHERE fbb.free_billsec <> d.derived" \
	"SELECT 'fbb '||fbb.id||' bal '||b.id||' fbid='||fbb.free_billsec_id||
	        ' stored='||fbb.free_billsec||' derived='||d.derived
	   FROM free_billsec_balance fbb
	   JOIN balance b ON b.id = fbb.balance_id
	   LEFT JOIN LATERAL (
	     SELECT COALESCE(SUM(r.call_billsec),0) AS derived FROM rating r
	      WHERE r.billing_account_id = b.billing_account_id AND $PERIOD_JOIN
	        AND r.call_price < 0 AND r.free_billsec_id = fbb.free_billsec_id
	   ) d ON true
	  WHERE fbb.free_billsec <> d.derived LIMIT $SAMPLE"

# ---------------------------------------------------------------- invariant 5
# Saturating at exactly the allowance is correct (double_rating splits the call
# at the boundary); exceeding it means the allowance never depleted.
check "5. no period over its free allowance" \
	"WITH used AS (
	   SELECT b.id AS bal_id, r.free_billsec_id, SUM(r.call_billsec) AS secs
	     FROM balance b JOIN rating r
	       ON r.billing_account_id = b.billing_account_id AND $PERIOD_JOIN
	    WHERE r.call_price < 0 AND r.free_billsec_id > 0
	    GROUP BY 1,2)
	 SELECT count(*) FROM used u JOIN free_billsec f ON f.id = u.free_billsec_id
	  WHERE u.secs > f.free_billsec" \
	"WITH used AS (
	   SELECT b.id AS bal_id, b.billing_account_id AS acct, r.free_billsec_id, SUM(r.call_billsec) AS secs
	     FROM balance b JOIN rating r
	       ON r.billing_account_id = b.billing_account_id AND $PERIOD_JOIN
	    WHERE r.call_price < 0 AND r.free_billsec_id > 0
	    GROUP BY 1,2,3)
	 SELECT 'bal '||u.bal_id||' acct '||u.acct||' fbid='||u.free_billsec_id||
	        ' used='||u.secs||' allowance='||f.free_billsec
	   FROM used u JOIN free_billsec f ON f.id = u.free_billsec_id
	  WHERE u.secs > f.free_billsec ORDER BY u.secs - f.free_billsec DESC LIMIT $SAMPLE"

# ---------------------------------------------------------------- invariant 6
check "6a. no duplicate (account,period) balance rows" \
	"SELECT count(*) FROM (
	   SELECT 1 FROM balance GROUP BY billing_account_id,start_date,end_date HAVING count(*) > 1) x" \
	"SELECT 'acct '||billing_account_id||' '||start_date||'..'||end_date||' x'||count(*)
	   FROM balance GROUP BY billing_account_id,start_date,end_date
	  HAVING count(*) > 1 LIMIT $SAMPLE"

check "6b. no duplicate (balance,fbid) ledger keys" \
	"SELECT count(*) FROM (
	   SELECT 1 FROM free_billsec_balance GROUP BY balance_id,free_billsec_id HAVING count(*) > 1) x" \
	"SELECT 'bal '||balance_id||' fbid='||free_billsec_id||' x'||count(*)
	   FROM free_billsec_balance GROUP BY balance_id,free_billsec_id
	  HAVING count(*) > 1 LIMIT $SAMPLE"

# ---------------------------------------------------------------- invariant 7
# Completeness: a period that produced rating rows must have its balance row.
# Only accounts with an active pcard get a balance at all (both engines skip the
# rest), so restrict to those - otherwise this fires on unprovisioned accounts.
check "7. every rated period has a balance row" \
	"SELECT count(*) FROM (
	   SELECT DISTINCT r.billing_account_id, r.pcard_id
	     FROM rating r
	    WHERE r.pcard_id > 0
	      AND NOT EXISTS (
	        SELECT 1 FROM balance b
	         WHERE b.billing_account_id = r.billing_account_id AND $PERIOD_JOIN)) x" \
	"SELECT DISTINCT 'acct '||r.billing_account_id||' pcard '||r.pcard_id||' call_ts '||r.call_ts
	   FROM rating r
	  WHERE r.pcard_id > 0
	    AND NOT EXISTS (
	      SELECT 1 FROM balance b
	       WHERE b.billing_account_id = r.billing_account_id AND $PERIOD_JOIN)
	  LIMIT $SAMPLE"

# ---------------------------------------------------------------- invariant 8
# Completeness: consumed free seconds must be recorded in the ledger.
check "8. every consuming (balance,fbid) has a ledger row" \
	"SELECT count(*) FROM (
	   SELECT b.id AS bal_id, r.free_billsec_id
	     FROM balance b JOIN rating r
	       ON r.billing_account_id = b.billing_account_id AND $PERIOD_JOIN
	    WHERE r.call_price < 0 AND r.free_billsec_id > 0
	    GROUP BY 1,2
	   EXCEPT
	   SELECT balance_id, free_billsec_id FROM free_billsec_balance) x" \
	"SELECT 'bal '||bal_id||' fbid='||free_billsec_id||' has no ledger row' FROM (
	   SELECT b.id AS bal_id, r.free_billsec_id
	     FROM balance b JOIN rating r
	       ON r.billing_account_id = b.billing_account_id AND $PERIOD_JOIN
	    WHERE r.call_price < 0 AND r.free_billsec_id > 0
	    GROUP BY 1,2
	   EXCEPT
	   SELECT balance_id, free_billsec_id FROM free_billsec_balance) x LIMIT $SAMPLE"

echo
echo "================ rating invariants summary ================="
echo "  PASS=$PASS  FAIL=$FAIL   db=$DBNAME"
if [ "$FAIL" -ne 0 ] && [ "$VERBOSE" != "1" ]; then
	echo "  (re-run with VERBOSE=1 to see offending rows)"
fi
echo "============================================================"

[ "$FAIL" -eq 0 ]
