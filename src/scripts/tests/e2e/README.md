# CallControl end-to-end tests

Black-box tests that start a **real, installed** RateEngine and drive its
CallControl JSON-RPC interface (module `jsonrpc_cc`) over the `tcp` and `tls`
transports. This is the Phase 1 functional coverage for the 0.7.x CallControl
surface (concurrent online charging + the TLS transport).

## What it checks

| Group | Interface | Assertions |
|-------|-----------|------------|
| jsonrpc_cc protocol | tcp | `state`/`rate`/`balance`/`cprice` stubs reply with a `result`; malformed input returns the correct JSON-RPC error codes (`-32700` parse, `-32600` bad version, `-32601` unknown method) |
| TLS transport | tls | handshake + a `state` round-trip succeeds; plaintext spoken to a TLS port is refused |
| Mutual TLS | tls (`verify-client=yes`) | a client **without** a cert is rejected; a client presenting a CA-signed cert is accepted |
| Worker-pool smoke | tcp | a burst of concurrent `maxsec` requests all reply and the daemon stays up |

The stub methods and error paths need **no seeded rating data** — only a
database the daemon can connect to, because CallControl binds no socket until
its startup DB connect succeeds. Rating-value correctness (real `maxsec`,
`rate`, `cprice` numbers) is covered separately by the Phase 2 golden-CDR
regression, not here.

## Running

Requires an installed engine (`make install`) and a reachable DB with the
schema loaded.

```sh
# from src/ :
make e2e

# or directly, overriding defaults via env:
RE_PREFIX=/usr/local/RateEngine \
DBHOST=127.0.0.1 DBNAME=rate_engine DBUSER=re_admin DBPASS=change_me DBPORT=5432 \
scripts/tests/e2e/run_e2e.sh
```

Exit code: `0` all passed, `1` a test failed, `2` prerequisites missing.

## How it works

`run_e2e.sh` builds a self-contained temp workdir: it symlinks the installed
`modules/` in (the module dir is `<System DIR>/modules/`), mints a throwaway
PKI with `scripts/gen_tls_cert.sh`, builds the TLS test client from
`clients/my_cc/tls_client.c`, generates a config that loads
`pgsql/cdrm/rt/cc/jsonrpc_cc/tcp/tls` and one interface per transport, then runs
the daemon in the foreground (`-2c`) and kills it by PID at teardown. Plaintext
requests use bash `/dev/tcp` (one request per connection, matching the server);
TLS/mTLS use the built `tls_client` whose exit codes encode accept (`0`) vs
handshake-reject (`2`).

`lib_e2e.sh` holds the reusable primitives (`tcp_send`, `wait_port`, the
`assert_*` helpers). Everything is torn down on exit; on failure the tail of the
engine log is printed for debugging.

## In CI

Runs as the final step of the `build` job in `.github/workflows/build.yml`,
which already installs the engine and initializes the Postgres schema.
