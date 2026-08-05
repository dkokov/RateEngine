#!/usr/bin/env bash
#
# run_api_test.sh - black-box functional tests for the RE7 provisioning API
# (scripts/api-server). Logs in, exercises the whole provisioning surface
# (bill plans, tariffs, prefixes, rates, calc-functions, reference lists,
# composite services, change ops, pcards, balance) with HTTP-code assertions,
# then tears the account down via DeleteService.
#
# Runs against a REAL, running API server backed by a reachable RE7 DB.
# All test objects are prefixed ($PFX, default APITEST) so they are easy to spot.
#
# Env:
#   BASE       API base URL   (default https://127.0.0.1:8443)
#   API_USER   admin/provisioning user in the API auth store   (REQUIRED)
#   API_PASS   its password                                    (REQUIRED)
#   PFX        test-object name prefix   (default APITEST)
#
# Exit: 0 all assertions passed, 1 a failure, 2 prerequisites missing.

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
# shellcheck source=./lib_api.sh
. "$HERE/lib_api.sh"
: "${PFX:=APITEST}"

[ -n "$API_USER" ] && [ -n "$API_PASS" ] || { echo "set API_USER and API_PASS"; exit 2; }

echo "== RE7 API functional test =="
echo "base=$BASE user=$API_USER prefix=$PFX"

# health (public, no auth)
TOKEN=""
req GET /health
assert_code 200 "health"

TOKEN=$(api_login)
if [ -n "$TOKEN" ]; then pass "login"; else fail "login (no token)"; echo "PASS=$PASS FAIL=$FAIL"; exit 1; fi

PLAN="${PFX}_PLAN"; PLAN2="${PFX}_PLAN2"; TAR="${PFX}_TAR"; PRE="359"
ACC="${PFX}_ACC"; NUM="359999${RANDOM}"

# --- rate-plan definition ---
req POST /bill-plans "{\"name\":\"$PLAN\",\"type\":\"prepaid\"}";   assert_created "create bill-plan"
req POST /bill-plans "{\"name\":\"$PLAN2\",\"type\":\"postpaid\"}"; assert_created "create bill-plan2"
req POST /tariffs "{\"name\":\"$TAR\"}";                            assert_created "create tariff"
req POST /prefixes "{\"prefix\":\"$PRE\"}";                         note "prefix $PRE (code $REPLY_CODE, may pre-exist)"
req POST /rates "{\"bill_plan\":\"$PLAN\",\"prefix\":\"$PRE\",\"tariff\":\"$TAR\"}"; assert_created "create rate"
req GET "/rates?bill_plan=$PLAN";                                   assert_code 200 "list rates"; assert_contains "$TAR" "rate list has tariff"
req POST "/tariffs/$TAR/calc-functions" '{"pos":1,"delta_time":60,"fee":"0.05","iterations":1}'; assert_code 201 "calc-function create"
req GET "/tariffs/$TAR/calc-functions";                            assert_contains '"pos":1' "calc-function listed"

# --- reference lists ---
req GET /ref/currencies;      assert_code 200 "ref currencies"; assert_contains "currencies" "ref currencies body"
req GET /ref/pcard-statuses;  assert_contains "active" "ref pcard-statuses"

# --- composite service (CreateService / CheckService) ---
req POST /services "{\"username\":\"$ACC\",\"number\":\"$NUM\",\"bill_plan\":\"$PLAN\",\"pcard\":{\"amount\":20,\"status\":\"active\"},\"balance\":{\"amount\":0}}"
assert_code 201 "CreateService"
req GET "/services/$ACC";     assert_code 200 "CheckService"; assert_contains "$NUM" "service has number"

# --- change ops ---
req PATCH "/numbers/$NUM/bill-plan" "{\"bill_plan\":\"$PLAN2\"}";   assert_code 200 "ChangeBillPlan"
req GET "/accounts/$ACC/balance";                                  assert_code 200 "balance read"
req POST "/accounts/$ACC/pcards" '{"amount":50,"status":"active"}'; assert_code 201 "CreatePCard"
PCID=$(reply_field id)
if [ -n "$PCID" ]; then
    req PATCH "/pcards/$PCID/limit"  '{"amount":75}';    assert_code 200 "UpdateCreditLimit"
    req PATCH "/pcards/$PCID/status" '{"status":"block"}'; assert_code 200 "ChangePCardStatus"
fi

# --- negative paths ---
req GET /bill-plans/__nope__;  assert_code 404 "unknown bill-plan -> 404"
OLD=$TOKEN; TOKEN=""; req GET "/bill-plans/$PLAN"; TOKEN=$OLD; assert_code 401 "no token -> 401"

# --- cleanup (DeleteService tears down the account atomically) ---
req DELETE "/services/$ACC"; assert_code 200 "DeleteService (cleanup)"
note "shared defs left in DB: $PLAN, $PLAN2, $TAR (drop via SQL if desired)"

echo "== done: PASS=$PASS FAIL=$FAIL =="
[ "$FAIL" -eq 0 ] || exit 1
