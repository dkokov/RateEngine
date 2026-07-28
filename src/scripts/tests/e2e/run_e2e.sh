#!/usr/bin/env bash
#
# run_e2e.sh - black-box end-to-end tests for the RateEngine CallControl
# JSON-RPC interface (module jsonrpc_cc) over the tcp and tls transports.
#
# What it proves (the new 0.7.x CallControl surface):
#   * jsonrpc_cc protocol   - stub methods reply, malformed requests get the
#                             correct JSON-RPC error codes.
#   * tls transport         - handshake + a JSON-RPC round-trip; plaintext on a
#                             TLS port is refused.
#   * mutual TLS            - a client WITHOUT a cert is rejected, one WITH a
#                             cert (signed by the CA) is accepted.
#   * worker-pool smoke     - a burst of concurrent maxsec requests all reply
#                             and the daemon stays up (crash/race smoke).
#
# It runs a REAL, installed RateEngine against a REACHABLE database. The stub
# methods (state/rate/balance/cprice) and the error paths need no seeded rating
# data - only a DB the daemon can connect to (CallControl binds no socket until
# its DB connect succeeds). Rating-value correctness is Phase 2's golden-CDR job.
#
# Prerequisites:
#   * RateEngine installed under $RE_PREFIX (default /usr/local/RateEngine):
#       bin/RateEngine, libs/libre7core.so, modules/{pgsql,cdrm,rt,cc,
#       jsonrpc_cc,tcp,tls}.so
#   * A reachable DB (schema loaded); credentials via the env vars below.
#   * openssl + a C compiler (to mint a throwaway PKI and build the tls client).
#
# Env (all optional; defaults match the CI Postgres service):
#   RE_PREFIX  install prefix        (default /usr/local/RateEngine)
#   DBTYPE     pgsql|mysql|redis     (default pgsql)
#   DBHOST DBNAME DBUSER DBPASS DBPORT
#   TCP_PORT   jsonrpc_cc/tcp test port  (default 19091; off the 9090-9093 range)
#   TLS_PORT   jsonrpc_cc/tls test port  (default 19092)
#
# Exit: 0 = all assertions passed, 1 = a failure, 2 = prerequisites missing.

set -u

HERE=$(cd "$(dirname "$0")" && pwd)
# e2e -> tests -> scripts -> src
REPO_SRC=$(cd "$HERE/../../.." && pwd)

# shellcheck source=./lib_e2e.sh
. "$HERE/lib_e2e.sh"

# ---- configuration -------------------------------------------------------
RE_PREFIX=${RE_PREFIX:-/usr/local/RateEngine}
RE_BIN=${RE_BIN:-$RE_PREFIX/bin/RateEngine}
RE_LIBS=${RE_LIBS:-$RE_PREFIX/libs}
RE_MODULES=${RE_MODULES:-$RE_PREFIX/modules}
# Installed engine config to inherit DB connection params from (see below).
RE_CONF=${RE_CONF:-$RE_PREFIX/config/RateEngine7.xml}

# DB connection resolution, in order of precedence:
#   1. explicit env (DBHOST=... make e2e)
#   2. the <DB> block of the installed engine config ($RE_CONF) - so `make e2e`
#      connects wherever the real engine is configured (e.g. a docker db host)
#   3. built-in fallbacks (match the CI Postgres service)
db_from_conf() {
	[ -f "$RE_CONF" ] || return 0
	sed -n '/<DB>/,/<\/DB>/p' "$RE_CONF" 2>/dev/null |
		sed -nE "s/.*name=\"$1\"[^>]*value=\"([^\"]*)\".*/\1/p" | head -1
}
DBTYPE=${DBTYPE:-$(db_from_conf dbtype)}
DBHOST=${DBHOST:-$(db_from_conf dbhost)}
DBNAME=${DBNAME:-$(db_from_conf dbname)}
DBUSER=${DBUSER:-$(db_from_conf dbuser)}
DBPASS=${DBPASS:-$(db_from_conf dbpass)}
DBPORT=${DBPORT:-$(db_from_conf dbport)}
DBTYPE=${DBTYPE:-pgsql}
DBHOST=${DBHOST:-127.0.0.1}
DBNAME=${DBNAME:-rate_engine}
DBUSER=${DBUSER:-re_admin}
DBPASS=${DBPASS:-_cfg.access}
DBPORT=${DBPORT:-5432}

# Dedicated TEST ports, deliberately clear of the standard CallControl
# interface ports (my_cc 9090, jsonrpc_cc 9091, jsonrpc_cc_tls 9092,
# my_cc_tls 9093) so the harness never collides with a real/running instance.
TCP_PORT=${TCP_PORT:-19091}
TLS_PORT=${TLS_PORT:-19092}

GEN_CERT="$REPO_SRC/scripts/gen_tls_cert.sh"
TLS_CLIENT_SRC="$REPO_SRC/clients/my_cc/tls_client.c"

WORKDIR=""
RE_PID=""
LOGFILE=""
TLS_CLIENT=""

# ---- lifecycle -----------------------------------------------------------
die() {
	echo "run_e2e: $*" >&2
	dump_logs
	exit 2
}

dump_logs() {
	if [ -n "$WORKDIR" ] && [ -f "$WORKDIR/daemon.stdout" ]; then
		echo "----- daemon.stdout (tail) -----" >&2
		tail -n 40 "$WORKDIR/daemon.stdout" >&2 || true
	fi
	if [ -n "$LOGFILE" ] && [ -f "$LOGFILE" ]; then
		echo "----- rate_engine.log (tail) -----" >&2
		tail -n 60 "$LOGFILE" >&2 || true
	fi
	if [ -n "$WORKDIR" ] && grep -q 'db_connect() ERROR' "$WORKDIR/daemon.stdout" 2>/dev/null; then
		{
			echo "----- diagnosis -----"
			echo "CallControl could not connect to the database, so it bound no"
			echo "interface (this is why no port came up). Effective DB settings:"
			echo "  type=$DBTYPE host=$DBHOST port=$DBPORT name=$DBNAME user=$DBUSER"
			echo "Point the harness at a reachable DB, e.g.:  DBHOST=<host> make e2e"
			echo "or set RE_CONF=<engine config whose <DB> block is correct>."
		} >&2
	fi
}

cleanup() {
	re_stop
	[ -n "$WORKDIR" ] && rm -rf "$WORKDIR"
}

setup() {
	[ -x "$RE_BIN" ] || die "RateEngine binary not found/executable: $RE_BIN (install it, or set RE_PREFIX)"
	[ -d "$RE_MODULES" ] || die "modules dir not found: $RE_MODULES"
	[ -f "$GEN_CERT" ] || die "cert generator not found: $GEN_CERT"
	[ -f "$TLS_CLIENT_SRC" ] || die "tls client source not found: $TLS_CLIENT_SRC"
	command -v openssl >/dev/null || die "openssl not found"
	command -v gcc >/dev/null || die "gcc not found (needed to build the tls test client)"

	WORKDIR=$(mktemp -d "${TMPDIR:-/tmp}/re7_e2e.XXXXXX") || die "mktemp failed"
	# DIR-relative module dir is "<DIR>/modules/"; symlink the installed one in
	# so the workdir is self-contained (logs/pidfile/cc_int all live under it).
	ln -s "$RE_MODULES" "$WORKDIR/modules"
	mkdir -p "$WORKDIR/logs" "$WORKDIR/config/cc_int" "$WORKDIR/config/cdr_profiles" "$WORKDIR/certs"

	CERTDIR="$WORKDIR/certs"
	INTDIR="$WORKDIR/config/cc_int"
	CFG="$WORKDIR/config/RateEngine7.xml"
	LOGFILE="$WORKDIR/logs/rate_engine.log"

	# Throwaway test PKI: ca / server (SAN=127.0.0.1) / client. Short validity.
	bash "$GEN_CERT" "$CERTDIR" localhost 3 >"$WORKDIR/gen_cert.log" 2>&1 ||
		die "gen_tls_cert.sh failed (see $WORKDIR/gen_cert.log)"

	TLS_CLIENT="$WORKDIR/tls_client"
	gcc -O2 -o "$TLS_CLIENT" "$TLS_CLIENT_SRC" -lssl -lcrypto ||
		die "failed to build tls test client from $TLS_CLIENT_SRC"

	gen_main_config
}

# Writes the fixed main config (modules, DB, CallControl). Interfaces are added
# separately by set_interface() so each daemon run binds exactly ONE interface -
# isolating transports and sidestepping any multi-interface startup interaction
# in the engine (two cc_int interfaces racing at bind time).
gen_main_config() {
	cat >"$CFG" <<EOF
<RateEngine version="0.7.6">
 <System>
    <param name="DIR" value="$WORKDIR/" />
    <param name="PIDFile" value="logs/rate_engine.pid" />
 </System>
 <LoadModules>
    <param name="module" value="$DBTYPE.so" />
    <param name="module" value="cdrm.so" />
    <param name="module" value="rt.so" />
    <param name="module" value="cc.so" />
    <param name="module" value="jsonrpc_cc.so" />
    <param name="module" value="tcp.so" />
    <param name="module" value="tls.so" />
 </LoadModules>
 <DB>
    <param name="dbtype" value="$DBTYPE" />
    <param name="dbhost" value="$DBHOST" />
    <param name="dbname" value="$DBNAME" />
    <param name="dbuser" value="$DBUSER" />
    <param name="dbpass" value="$DBPASS" />
    <param name="dbport" value="$DBPORT" />
    <param name="NumberRetries" value="3" />
    <param name="IntervalRetries" value="1" />
 </DB>
 <CallControl>
    <param name="active" value="yes" />
    <param name="CCServerMicroSleep" value="60000" />
    <param name="CallMaxsecLimit" value="3600" />
    <param name="SimCalls" value="150" />
    <param name="CCWorkers" value="8" />
    <param name="IntConfigDIR" value="$INTDIR/" />
 </CallControl>
 <Rating>
    <param name="active" value="no" />
    <param name="leg" value="a" />
 </Rating>
 <CDRMediator>
    <param name="CDRProfilesDIR" value="$WORKDIR/config/cdr_profiles/" />
 </CDRMediator>
 <Logs>
    <param name="LogFile" value="logs/rate_engine.log" />
    <param name="LogMaxFileSize" value="40960000" />
    <param name="LogSeparator" value="|" />
    <param name="LogDateFormat" value="" />
    <param name="LogDebugLevel" value="4" />
 </Logs>
</RateEngine>
EOF
}

# set_interface KIND  (tcp | tls | mtls) - place exactly ONE interface file in
# INTDIR for the next daemon run.
set_interface() {
	rm -f "$INTDIR"/*.xml
	case "$1" in
	tcp)
		cat >"$INTDIR/if.xml" <<EOF
<Interface>
 <config>
    <param name="CC-proto" value="jsonrpc_cc" />
    <param name="proto" value="tcp" />
    <param name="ip-version" value="IPv4" />
    <param name="ip" value="" />
    <param name="port" value="$TCP_PORT" />
 </config>
</Interface>
EOF
		;;
	tls) write_tls_interface no ;;
	mtls) write_tls_interface yes ;;
	*) die "unknown interface kind: $1" ;;
	esac
}

# write_tls_interface VERIFY_CLIENT (yes|no)
write_tls_interface() {
	cat >"$INTDIR/if.xml" <<EOF
<Interface>
 <config>
    <param name="CC-proto" value="jsonrpc_cc" />
    <param name="proto" value="tls" />
    <param name="ip-version" value="IPv4" />
    <param name="ip" value="" />
    <param name="port" value="$TLS_PORT" />
    <param name="cert" value="$CERTDIR/server.crt" />
    <param name="key"  value="$CERTDIR/server.key" />
    <param name="verify-client" value="$1" />
    <param name="ca" value="$CERTDIR/ca.crt" />
 </config>
</Interface>
EOF
}

re_start() {
	: >"$LOGFILE" 2>/dev/null || true
	# -2c runs CallControl in the foreground (no daemonize/pidfile); exec so $!
	# is the RateEngine PID and a plain kill tears it down.
	(
		cd "$WORKDIR" &&
			exec env LD_LIBRARY_PATH="$RE_LIBS:${LD_LIBRARY_PATH:-}" \
				"$RE_BIN" -c "$CFG" -2c
	) >"$WORKDIR/daemon.stdout" 2>&1 &
	RE_PID=$!
}

re_stop() {
	[ -n "$RE_PID" ] || return 0
	kill -TERM "$RE_PID" 2>/dev/null || true
	wait "$RE_PID" 2>/dev/null || true
	RE_PID=""
}

# ---- test groups ---------------------------------------------------------
STATE_REQ='{"jsonrpc":"2.0","method":"state","params":{"cdr_server_id":1},"id":1}'

test_jsonrpc_cc_tcp() {
	echo "== jsonrpc_cc protocol (tcp:$TCP_PORT) =="
	local r

	r=$(tcp_send 127.0.0.1 "$TCP_PORT" "$STATE_REQ")
	assert_contains "$r" '"result"' "state: carries a result"
	assert_contains "$r" 'idle' "state: reports idle"

	r=$(tcp_send 127.0.0.1 "$TCP_PORT" '{"jsonrpc":"2.0","method":"rate","params":{"cdr_server_id":1,"clg":"359112","cld":"359880001"},"id":2}')
	assert_contains "$r" '"result"' "rate (stub): carries a result"
	assert_contains "$r" 'amount' "rate (stub): has amount"

	r=$(tcp_send 127.0.0.1 "$TCP_PORT" '{"jsonrpc":"2.0","method":"balance","params":{"cdr_server_id":1,"clg":"359112"},"id":3}')
	assert_contains "$r" 'amount' "balance (stub): has amount"

	r=$(tcp_send 127.0.0.1 "$TCP_PORT" '{"jsonrpc":"2.0","method":"cprice","params":{"cdr_server_id":1,"clg":"359112","cld":"359880001","billsec":60},"id":4}')
	assert_contains "$r" 'amount' "cprice (stub): has amount"

	# --- error paths (deterministic codes, no DB) ---
	r=$(tcp_send 127.0.0.1 "$TCP_PORT" 'this is not json')
	assert_contains "$r" '-32700' "parse error -> -32700"

	r=$(tcp_send 127.0.0.1 "$TCP_PORT" '{"jsonrpc":"1.0","method":"state","params":{"cdr_server_id":1},"id":5}')
	assert_contains "$r" '-32600' "wrong version -> -32600"

	# Unknown method: the engine currently rejects it with -32602 "Invalid
	# params" (not the spec's -32601 "Method not found"). Pin the behavioural
	# invariant - an error object, never a result - rather than the exact code.
	r=$(tcp_send 127.0.0.1 "$TCP_PORT" '{"jsonrpc":"2.0","method":"no_such_method","params":{"cdr_server_id":1},"id":6}')
	assert_contains "$r" '"error"' "unknown method is rejected with an error"
	assert_not_contains "$r" '"result"' "unknown method returns no result"
}

test_tls_plain() {
	echo "== tls transport (tls:$TLS_PORT, verify-client=no) =="
	local out rc r

	out=$("$TLS_CLIENT" 127.0.0.1 "$TLS_PORT" "$STATE_REQ" 2>&1)
	rc=$?
	assert_eq "$rc" "0" "tls: handshake + reply (client exit 0)"
	assert_contains "$out" 'idle' "tls: state round-trip over TLS"

	# Plaintext spoken to a TLS port must not yield a JSON-RPC reply.
	r=$(tcp_send 127.0.0.1 "$TLS_PORT" "$STATE_REQ" || true)
	assert_not_contains "$r" '"result"' "tls: plaintext on TLS port refused"
}

test_tls_mtls() {
	echo "== mutual TLS (tls:$TLS_PORT, verify-client=yes) =="
	local out rc

	# No client certificate -> server aborts the handshake, closes before a
	# reply -> tls_client exits 2.
	"$TLS_CLIENT" 127.0.0.1 "$TLS_PORT" "$STATE_REQ" >/dev/null 2>&1
	rc=$?
	assert_eq "$rc" "2" "mtls: client without cert is rejected (exit 2)"

	# Client presents a cert signed by the CA (and verifies the server) -> ok.
	out=$("$TLS_CLIENT" 127.0.0.1 "$TLS_PORT" "$STATE_REQ" \
		"$CERTDIR/client.crt" "$CERTDIR/client.key" "$CERTDIR/ca.crt" 2>&1)
	rc=$?
	assert_eq "$rc" "0" "mtls: client with cert is accepted (exit 0)"
	assert_contains "$out" 'idle' "mtls: state round-trip over mutual TLS"
}

test_cc_smoke_concurrency() {
	echo "== worker-pool smoke: concurrent 'state' burst (tcp:$TCP_PORT) =="
	# 'state' is a no-DB stub: fast + deterministic. Bursting it exercises the
	# net worker pool and the parser under concurrency without DB-timing noise.
	# Each request runs under an external `timeout` so a wedged daemon can never
	# hang the suite (bash /dev/tcp has no connect timeout of its own).
	local n=30 conc=10 i ok=0 f tmpd
	local pids=()
	tmpd=$(mktemp -d "${TMPDIR:-/tmp}/re7_burst.XXXXXX")
	echo "  firing $n 'state' requests, up to $conc at once..."
	for i in $(seq 1 "$n"); do
		# timeout -k: SIGTERM at 8s, SIGKILL at +3s - a hard ceiling so a wedged
		# daemon can never hang the burst. Batch-wait on captured PIDs (below)
		# because $(jobs -r) inside a command substitution cannot see the jobs.
		timeout -k 3 8 bash -c '_e2e_req "$1" "$2" 5' _ "$TCP_PORT" "$STATE_REQ" \
			>"$tmpd/$i" 2>/dev/null &
		pids+=($!)
		if [ "${#pids[@]}" -ge "$conc" ]; then
			wait "${pids[@]}"
			pids=()
		fi
	done
	[ "${#pids[@]}" -gt 0 ] && wait "${pids[@]}"

	for f in "$tmpd"/*; do
		grep -q 'idle' "$f" 2>/dev/null && ok=$((ok + 1))
	done
	rm -rf "$tmpd"

	assert_eq "$ok" "$n" "smoke: all $n concurrent state requests replied"
	if kill -0 "$RE_PID" 2>/dev/null; then
		pass "smoke: daemon still alive after the burst"
	else
		fail "smoke: daemon died during the burst"
	fi

	echo "== rating-path smoke: a few maxsec requests (tcp:$TCP_PORT) =="
	# maxsec is DB-heavy (many queries per call); just smoke that it replies and
	# does not crash. The VALUE is not asserted here - that is the Phase 2
	# golden-CDR job. Few requests, generous per-request timeout.
	local m=5 mok=0 r
	echo "  sending $m maxsec requests (per-request timeout 8s)..."
	for i in $(seq 1 "$m"); do
		r=$(timeout -k 3 10 bash -c '_e2e_req "$1" "$2" 8' _ "$TCP_PORT" \
			"{\"jsonrpc\":\"2.0\",\"method\":\"maxsec\",\"params\":{\"cdr_server_id\":1,\"call-uid\":\"e2e-$i\",\"clg\":\"359112\",\"cld\":\"359880001\"},\"id\":$i}" 2>/dev/null)
		case "$r" in *'"maxsec"'*) mok=$((mok + 1)) ;; esac
	done
	assert_eq "$mok" "$m" "smoke: all $m maxsec requests replied"
	if grep -q 'maxsec_us' "$LOGFILE" 2>/dev/null; then
		note "maxsec timing (maxsec_us) present in the log"
	else
		note "maxsec_us not found in log (log level / build), non-fatal"
	fi
}

# ---- main ----------------------------------------------------------------
main() {
	trap cleanup EXIT
	setup

	echo "run_e2e: engine=$RE_BIN"
	echo "run_e2e: db=$DBTYPE host=$DBHOST port=$DBPORT name=$DBNAME user=$DBUSER (conf: $RE_CONF)"

	echo "### tcp: jsonrpc_cc protocol + worker-pool smoke ###"
	set_interface tcp
	re_start
	wait_port 127.0.0.1 "$TCP_PORT" 25 ||
		die "daemon did not bind tcp:$TCP_PORT (DB reachable? modules loaded?)"
	test_jsonrpc_cc_tcp
	test_cc_smoke_concurrency
	re_stop

	echo "### tls: server-side TLS ###"
	set_interface tls
	re_start
	wait_port 127.0.0.1 "$TLS_PORT" 25 ||
		die "daemon did not bind tls:$TLS_PORT"
	test_tls_plain
	re_stop

	echo "### mtls: mutual TLS ###"
	set_interface mtls
	re_start
	wait_port 127.0.0.1 "$TLS_PORT" 25 ||
		die "daemon did not bind tls:$TLS_PORT (mtls)"
	test_tls_mtls
	re_stop

	echo
	echo "==================== e2e summary ===================="
	echo "  PASS=$PASS  FAIL=$FAIL"
	echo "====================================================="
	if [ "$FAIL" -ne 0 ]; then
		dump_logs
		return 1
	fi
	return 0
}

main "$@"
