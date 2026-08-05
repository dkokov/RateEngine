#!/usr/bin/env bash
#
# lib_api.sh - shared helpers for the RE7 provisioning-API black-box tests
# (scripts/api-server). Sourced by run_api_test.sh and api_perf.sh.
#
# Env (all optional except credentials for auth'd paths):
#   BASE       API base URL          (default https://127.0.0.1:8443)
#   API_USER   provisioning/admin user in the API auth store
#   API_PASS   its password
#

: "${BASE:=https://127.0.0.1:8443}"
: "${API_USER:=}"
: "${API_PASS:=}"

# PASS/FAIL counters (consumed by the runner for the exit code).
PASS=0
FAIL=0
note() { echo "  INFO: $*"; }
pass() { PASS=$((PASS + 1)); echo "  PASS: $1"; }
fail() { FAIL=$((FAIL + 1)); echo "  FAIL: $1"; }

# json_field FIELD  - extract a top-level scalar from stdin JSON
json_field() {
    php -r '$d=json_decode(stream_get_contents(STDIN),true);echo is_array($d)?($d[$argv[1]]??""):"";' "$1" 2>/dev/null
}

# api_login -> echoes an access token (empty on failure). Uses $BASE/$API_USER/$API_PASS.
api_login() {
    curl -k -s -X POST "$BASE/auth/login" -H 'Content-Type: application/json' \
        -d "{\"username\":\"$API_USER\",\"password\":\"$API_PASS\"}" | json_field access_token
}

# req METHOD PATH [JSON_BODY]
#   uses $TOKEN if set; sets REPLY_CODE and REPLY_BODY
req() {
    local method=$1 path=$2 data=${3:-}
    local tmp; tmp=$(mktemp)
    local args=(-k -s -o "$tmp" -w '%{http_code}' -X "$method" "$BASE$path")
    [ -n "${TOKEN:-}" ] && args+=(-H "Authorization: Bearer $TOKEN")
    [ -n "$data" ] && args+=(-H 'Content-Type: application/json' -d "$data")
    REPLY_CODE=$(curl "${args[@]}")
    REPLY_BODY=$(cat "$tmp"); rm -f "$tmp"
}

# reply_field FIELD - scalar from the last REPLY_BODY
reply_field() { printf '%s' "$REPLY_BODY" | json_field "$1"; }

# assert_code WANT LABEL
assert_code() {
    if [ "$REPLY_CODE" = "$1" ]; then pass "$2 ($REPLY_CODE)"
    else fail "$2 (got $REPLY_CODE want $1; body: $REPLY_BODY)"; fi
}

# assert_created LABEL  - get-or-create success: 201 (new) or 200 (already existed)
assert_created() {
    case "$REPLY_CODE" in
        200 | 201) pass "$1 ($REPLY_CODE)" ;;
        *) fail "$1 (got $REPLY_CODE want 200/201; body: $REPLY_BODY)" ;;
    esac
}

# assert_contains NEEDLE LABEL  (searches REPLY_BODY)
assert_contains() {
    case "$REPLY_BODY" in
        *"$1"*) pass "$2" ;;
        *) fail "$2 (missing '$1' in: $REPLY_BODY)" ;;
    esac
}
