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
#   TCP_PORT   jsonrpc_cc/tcp port   (default 9091)
#   TLS_PORT   jsonrpc_cc/tls port   (default 9092)
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

DBTYPE=${DBTYPE:-pgsql}
DBHOST=${DBHOST:-127.0.0.1}
DBNAME=${DBNAME:-rate_engine}
DBUSER=${DBUSER:-re_admin}
DBPASS=${DBPASS:-_cfg.access}
DBPORT=${DBPORT:-5432}

TCP_PORT=${TCP_PORT:-9091}
TLS_PORT=${TLS_PORT:-9092}

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
}

# gen_config PROFILE  (PROFILE = plain | mtls)
#   plain : tcp:$TCP_PORT + tls:$TLS_PORT (verify-client=no)
#   mtls  : tls:$TLS_PORT (verify-client=yes)  [own daemon run - verify-client
#           is process-wide via the shared SSL_CTX, so it must not mix with a
#           plaintext-TLS interface in the same process.]
gen_config() {
	local profile=$1

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

	rm -f "$INTDIR"/*.xml

	if [ "$profile" = "plain" ]; then
		cat >"$INTDIR/00_tcp.xml" <<EOF
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
		gen_tls_interface no
	elif [ "$profile" = "mtls" ]; then
		gen_tls_interface yes
	else
		die "unknown profile: $profile"
	fi
}

# gen_tls_interface VERIFY_CLIENT (yes|no)
gen_tls_interface() {
	local verify=$1
	cat >"$INTDIR/01_tls.xml" <<EOF
<Interface>
 <config>
    <param name="CC-proto" value="jsonrpc_cc" />
    <param name="proto" value="tls" />
    <param name="ip-version" value="IPv4" />
    <param name="ip" value="" />
    <param name="port" value="$TLS_PORT" />
    <param name="cert" value="$CERTDIR/server.crt" />
    <param name="key"  value="$CERTDIR/server.key" />
    <param name="verify-client" value="$verify" />
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

	r=$(tcp_send 127.0.0.1 "$TCP_PORT" '{"jsonrpc":"2.0","method":"no_such_method","params":{"cdr_server_id":1},"id":6}')
	assert_contains "$r" '-32601' "unknown method -> -32601"
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
	echo "== worker-pool smoke: concurrent maxsec burst (tcp:$TCP_PORT) =="
	local n=50 conc=10 i=0 ok=0 f tmpd
	tmpd=$(mktemp -d "${TMPDIR:-/tmp}/re7_burst.XXXXXX")

	while [ "$i" -lt "$n" ]; do
		(
			r=$(tcp_send 127.0.0.1 "$TCP_PORT" \
				"{\"jsonrpc\":\"2.0\",\"method\":\"maxsec\",\"params\":{\"cdr_server_id\":1,\"call-uid\":\"e2e-$i\",\"clg\":\"359112\",\"cld\":\"359880001\"},\"id\":$i}")
			printf '%s' "$r" >"$tmpd/$i"
		) &
		i=$((i + 1))
		while [ "$(jobs -r | wc -l)" -ge "$conc" ]; do
			wait -n 2>/dev/null || sleep 0.05
		done
	done
	wait

	for f in "$tmpd"/*; do
		grep -q '"maxsec"' "$f" 2>/dev/null && ok=$((ok + 1))
	done
	rm -rf "$tmpd"

	assert_eq "$ok" "$n" "smoke: all $n concurrent maxsec requests replied"
	if kill -0 "$RE_PID" 2>/dev/null; then
		pass "smoke: daemon still alive after the burst"
	else
		fail "smoke: daemon died during the burst"
	fi
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

	echo "### profile: plain (tcp + server-side TLS) ###"
	gen_config plain
	re_start
	wait_port 127.0.0.1 "$TCP_PORT" 25 ||
		die "daemon did not bind tcp:$TCP_PORT (DB reachable? modules loaded?)"
	wait_port 127.0.0.1 "$TLS_PORT" 25 ||
		die "daemon did not bind tls:$TLS_PORT"
	test_jsonrpc_cc_tcp
	test_tls_plain
	test_cc_smoke_concurrency
	re_stop

	echo "### profile: mtls (mutual TLS) ###"
	gen_config mtls
	re_start
	wait_port 127.0.0.1 "$TLS_PORT" 25 ||
		die "daemon did not bind tls:$TLS_PORT (mtls profile)"
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
