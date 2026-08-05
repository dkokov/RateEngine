#!/usr/bin/env bash
#
# api_perf.sh - throughput/latency for the RE7 API read paths.
#
# Login is rate-limited (nginx limit_req on /auth/login), so this logs in ONCE
# and reuses the bearer token, then fires TOTAL requests at CONC concurrency
# against a chosen endpoint. Uses `hey` or `ab` if present (better latency
# percentiles), otherwise falls back to parallel curl (xargs -P) for a coarse
# req/s figure.
#
# Env:   BASE, API_USER, API_PASS   (credentials only needed for auth'd paths)
# Args:  [path] [total] [conc]
#   e.g. ./api_perf.sh /health          5000 64     # public, no auth, no DB
#        ./api_perf.sh /ref/currencies  2000 32     # auth + DB read
#        ./api_perf.sh /bill-plans/SOME 2000 32     # auth + DB point lookup
#
# Exit: 0 ok, 1 login failed, 2 prereqs missing.

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
# shellcheck source=./lib_api.sh
. "$HERE/lib_api.sh"

RPATH=${1:-/health}
TOTAL=${2:-1000}
CONC=${3:-16}

TOKEN=""
if [ "$RPATH" != "/health" ]; then
    [ -n "$API_USER" ] && [ -n "$API_PASS" ] || { echo "set API_USER/API_PASS for an authenticated path"; exit 2; }
    TOKEN=$(api_login)
    [ -n "$TOKEN" ] || { echo "login failed"; exit 1; }
fi

echo "perf: $RPATH  total=$TOTAL conc=$CONC  base=$BASE  auth=$([ -n "$TOKEN" ] && echo yes || echo no)"

URL="$BASE$RPATH"

if command -v hey >/dev/null 2>&1; then
    if [ -n "$TOKEN" ]; then hey -n "$TOTAL" -c "$CONC" -H "Authorization: Bearer $TOKEN" "$URL"
    else hey -n "$TOTAL" -c "$CONC" "$URL"; fi
elif command -v ab >/dev/null 2>&1; then
    if [ -n "$TOKEN" ]; then ab -k -n "$TOTAL" -c "$CONC" -H "Authorization: Bearer $TOKEN" "$URL"
    else ab -k -n "$TOTAL" -c "$CONC" "$URL"; fi
else
    note "hey/ab not found - using parallel curl (coarse throughput only)"
    export URL AUTH="$TOKEN"
    one() {
        if [ -n "$AUTH" ]; then curl -k -s -o /dev/null -H "Authorization: Bearer $AUTH" "$URL"
        else curl -k -s -o /dev/null "$URL"; fi
    }
    export -f one
    start=$(date +%s.%N)
    seq 1 "$TOTAL" | xargs -P "$CONC" -I{} bash -c 'one'
    end=$(date +%s.%N)
    awk -v s="$start" -v e="$end" -v n="$TOTAL" \
        'BEGIN{ d=e-s; printf "wall=%.2fs  throughput=%.1f req/s  (%.2f ms/req avg)\n", d, n/d, 1000*d/n }'
fi
