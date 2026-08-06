#!/usr/bin/env bash
#
# lib_e2e.sh - low-level helpers for the CallControl end-to-end tests.
#
# Sourced by run_e2e.sh. Pure, reusable primitives only (no daemon/config
# knowledge lives here): a plaintext request/reply over TCP, a port-ready wait,
# and a handful of assertions that keep PASS/FAIL counters.
#
# TCP I/O uses bash's /dev/tcp so the tests need no extra client binary for the
# plaintext interface. The CallControl server is one-request-per-connection
# (net_parallel_server close()s after each reply), so a single send + read-to-EOF
# is exactly one transaction.

# PASS/FAIL counters (globals, consumed by run_e2e.sh for the exit code).
PASS=0
FAIL=0

note() { echo "  INFO: $*"; }
pass() { PASS=$((PASS + 1)); echo "  PASS: $1"; }
fail() { FAIL=$((FAIL + 1)); echo "  FAIL: $1"; }

# tcp_send HOST PORT PAYLOAD  -> prints the server's reply on stdout.
# Opens the connection, writes PAYLOAD, reads until the server closes (with a
# hard timeout so a hung/misbehaving server cannot wedge the suite). Returns
# non-zero if the connection could not be opened at all.
tcp_send() {
	local host=$1 port=$2 payload=$3 resp
	if ! exec 3<>"/dev/tcp/$host/$port" 2>/dev/null; then
		return 1
	fi
	printf '%s' "$payload" >&3
	resp=$(timeout 3 cat <&3)
	exec 3<&- 3>&- 2>/dev/null || true
	printf '%s' "$resp"
}

# _e2e_req PORT PAYLOAD [READ_TIMEOUT]  -> one request/reply on stdout.
# Reads the whole reply until the server closes (EOF) or READ_TIMEOUT (default
# 4s), using bash's own timed `read` - deliberately NO `cat`: a `cat` launched
# under `timeout bash -c '...'` is an orphan grandchild that `timeout` (no -k)
# never SIGKILLs, so a wedged server would hang the burst. `read -t` is bounded
# and spawns no subprocess. Exported for `timeout bash -c '...'` child shells.
_e2e_req() {
	local to=${3:-4} reply=""
	exec 3<>"/dev/tcp/127.0.0.1/$1" 2>/dev/null || return 1
	printf '%s' "$2" >&3
	# -d '' : read until NUL (never present) i.e. until EOF/timeout, capturing
	# the full reply (no trailing-newline assumption); -t bounds the wait.
	IFS= read -r -d '' -t "$to" reply <&3
	printf '%s' "$reply"
	exec 3<&- 3>&- 2>/dev/null || true
}
export -f _e2e_req

# wait_port HOST PORT [TIMEOUT_SECONDS]  -> 0 once connectable, 1 on timeout.
wait_port() {
	local host=$1 port=$2 timeout=${3:-20} i=0 max
	max=$((timeout * 10))
	while [ "$i" -lt "$max" ]; do
		if (exec 3<>"/dev/tcp/$host/$port") 2>/dev/null; then
			exec 3<&- 3>&- 2>/dev/null || true
			return 0
		fi
		sleep 0.1
		i=$((i + 1))
	done
	return 1
}

# wait_port_free HOST PORT [TIMEOUT] - the inverse: block until nothing accepts
# on the port any more. The test groups reuse the same ports one after another,
# so starting the next daemon while the previous listener is still up means the
# new one fails to bind and every assertion silently runs against the OLD
# process (e.g. an mtls client checked against a verify-client=no listener).
wait_port_free() {
	local host=$1 port=$2 timeout=${3:-15} i=0 max
	max=$((timeout * 10))
	while [ "$i" -lt "$max" ]; do
		if ! (exec 3<>"/dev/tcp/$host/$port") 2>/dev/null; then
			return 0
		fi
		exec 3<&- 3>&- 2>/dev/null || true
		sleep 0.1
		i=$((i + 1))
	done
	return 1
}

# assert_contains HAYSTACK NEEDLE LABEL
assert_contains() {
	case "$1" in
	*"$2"*) pass "$3" ;;
	*) fail "$3 (expected to contain '$2', got: ${1:-<empty>})" ;;
	esac
}

# assert_not_contains HAYSTACK NEEDLE LABEL
assert_not_contains() {
	case "$1" in
	*"$2"*) fail "$3 (unexpected '$2' in: $1)" ;;
	*) pass "$3" ;;
	esac
}

# assert_eq GOT WANT LABEL
assert_eq() {
	if [ "$1" = "$2" ]; then
		pass "$3"
	else
		fail "$3 (expected '$2', got '$1')"
	fi
}
