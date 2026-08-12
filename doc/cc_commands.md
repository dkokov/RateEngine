# CallControl Console Commands

  Quick reference for driving a CallControl interface by hand from a console.
The **application protocol** (`CC-proto`: `my_cc` or `jsonrpc_cc`) and the
**transport** (`proto`: `tcp` / `tls` / `udp` / `sctp`) are independent — every
interface config in `IntConfigDIR` combines one of each on its own port. See
[cc_int_prof.md](cc_int_prof.md) for the interface config parameters.

The example ports below follow the shipped sample interface configs:

| Interface config | CC-proto | transport | port |
|---|---|---|---|
| `my_cc.xml`          | my_cc      | tcp | 9090 |
| `jsonrpc_cc.xml`     | jsonrpc_cc | tcp | 9091 |
| `jsonrpc_cc_tls.xml` | jsonrpc_cc | tls | 9092 |
| `my_cc_tls.xml`      | my_cc      | tls | 9093 |

Example values used throughout: `cdr_server_id=42`, calling number
`clg=35929998877`, called number `cld=359887654321`, `call-uid=2222`.

---

## 1. my_cc (CSV) over TCP — port 9090

Every request is comma-separated and begins with the same 4-field header, then
command-specific fields:

```
cdr_server_id , transaction_id , <command> , timestamp [ , command-specific fields ]
```

| Command | Full field layout |
|---|---|
| **status**  | `cdr_server_id,transaction_id,status,timestamp` |
| **maxsec**  | `…,maxsec,timestamp,clg,cld,call_uid` |
| **balance** | `…,balance,timestamp,clg` |
| **rate**    | `…,rate,timestamp,clg,cld` |
| **cprice**  | `…,cprice,timestamp,clg,cld,billsec` |
| **term**    | `…,term,timestamp,status,call_uid[,billsec,duration]` |

`term` status codes: `nc` = normal clear, `c` = cancel, `b` = busy, `e` = error.
`billsec,duration` are sent only when the status is `nc`.

The console client is `src/clients/my_cc/2cclient.c`
(`gcc -o 2cclient src/clients/my_cc/2cclient.c`):

```sh
# server alive?
./2cclient 127.0.0.1 9090 42,1,status,1234

# authorize a call at setup -> reply = allowed seconds (or a negative reason code)
./2cclient 127.0.0.1 9090 42,1,maxsec,1234,359200001,359111,2222

# subscriber balance -> reply = amount
./2cclient 127.0.0.1 9090 42,1,balance,1234,35920001

# per-minute rate for clg -> cld
./2cclient 127.0.0.1 9090 42,1,rate,1234,359200001,359111

# price of a given billsec
./2cclient 127.0.0.1 9090 42,1,cprice,1234,359200001,359111,120

# call ended normally (billsec=36, duration=60) -> triggers CDR insert + rating
./2cclient 127.0.0.1 9090 42,1,term,1234,nc,2222,36,60

# call cancelled (no billsec/duration)
./2cclient 127.0.0.1 9090 42,1,term,1234,c,2222
```

Reply format: `cdr_server_id,transaction_id,<result>` — `<result>` is the seconds
for `maxsec`, the amount for `balance`/`rate`, `ok`/`nok` for `term`, `ok` for
`status`/`cprice`.

Any of these also work with `nc` instead of `2cclient`, e.g.
`printf '42,1,maxsec,1234,359200001,359111,2222' | nc -N 127.0.0.1 9090`.

---

## 2. jsonrpc_cc (JSON-RPC 2.0) over TCP — port 9091

Raw TCP carrying JSON-RPC 2.0 (not HTTP — test with `nc`, not `curl`). One
request object in, one response object out. Param keys: `cdr_server_id`, `clg`,
`cld`, `call-uid`, `billsec`, `duration`, `status`.

```sh
# maxsec  (params: cdr_server_id, call-uid, clg, cld)  -> result {maxsec}
printf '{"jsonrpc":"2.0","method":"maxsec","params":{"cdr_server_id":42,"call-uid":"2222","clg":"359200001","cld":"359111"},"id":3}' | nc -N 127.0.0.1 9091
# <-- {"jsonrpc":"2.0","result":{"maxsec":3600},"id":3}

# balance (cdr_server_id, clg) -> {amount}
printf '{"jsonrpc":"2.0","method":"balance","params":{"cdr_server_id":42,"clg":"359200001"},"id":31}' | nc -N 127.0.0.1 9091
# <-- {"jsonrpc":"2.0","result":{"amount":2.41},"id":31}

# rate (cdr_server_id, clg, cld) -> {amount}
printf '{"jsonrpc":"2.0","method":"rate","params":{"cdr_server_id":42,"clg":"359200001","cld":"359111"},"id":611}' | nc -N 127.0.0.1 9091

# cprice (cdr_server_id, clg, cld, billsec) -> {amount}
printf '{"jsonrpc":"2.0","method":"cprice","params":{"cdr_server_id":42,"clg":"359200001","cld":"359111","billsec":120},"id":61}' | nc -N 127.0.0.1 9091

# term (cdr_server_id, call-uid, status[nc|c|b|e], billsec, duration) -> {status}
printf '{"jsonrpc":"2.0","method":"term","params":{"cdr_server_id":42,"call-uid":"2222","status":"nc","billsec":120,"duration":125},"id":9}' | nc -N 127.0.0.1 9091
# <-- {"jsonrpc":"2.0","result":{"status":"ok"},"id":9}

# state (cdr_server_id) -> {status:"idle"}
printf '{"jsonrpc":"2.0","method":"state","params":{"cdr_server_id":42},"id":62}' | nc -N 127.0.0.1 9091
```

A malformed/unknown request returns an error object:
`{"jsonrpc":"2.0","error":{"code":-32600,"message":"Invalid Request"},"id":0}`.

(`nc -N` closes the socket after sending; on BSD/macOS use `nc -q1`.)

---

## 3. TLS variants

Generate a test PKI first (paths match the `*_tls.xml` sample configs):

```sh
./src/scripts/gen_tls_cert.sh /usr/local/RateEngine/config/certs localhost
# -> ca.crt/ca.key, server.crt/server.key (+ client.crt/client.key for mutual TLS)
```

The payloads are identical to the plain-TCP variants — only the transport changes.
Connect with `openssl s_client` and pipe the same request string:

```sh
# my_cc over TLS (port 9093)
printf '42,1,maxsec,1234,359200001,359111,2222' \
  | openssl s_client -quiet -connect 127.0.0.1:9093 -CAfile /usr/local/RateEngine/config/certs/ca.crt

# jsonrpc_cc over TLS (port 9092)
printf '{"jsonrpc":"2.0","method":"maxsec","params":{"cdr_server_id":42,"call-uid":"2222","clg":"359200001","cld":"359111"},"id":3}' \
  | openssl s_client -quiet -connect 127.0.0.1:9092 -CAfile /usr/local/RateEngine/config/certs/ca.crt
```

**Mutual TLS:** when the interface sets `verify-client="yes"` + a `ca` bundle, the
client must also present a certificate signed by that CA — add
`-cert client.crt -key client.key` to the `s_client` line. With the sample default
`verify-client="no"`, no client certificate is needed.

---

## Errors and reason codes

### maxsec reason codes

On failure `maxsec` returns a **negative** value instead of a seconds count. The
values are identical for both protocols (my_cc puts it in the reply's result
field; jsonrpc_cc returns it in-band as `{"result":{"maxsec":<negative>}}`):

| Code | Name | Meaning |
|---|---|---|
| `-1`  | NO_BACC   | no billing account for this `clg` + `cdr_server_id` |
| `-2`  | NO_BPLAN  | account has no bill plan |
| `-3`  | NO_PCARD  | no active payment card |
| `-4`  | NO_CLIMIT | credit limit already reached (remaining credit ≤ 0) |
| `-6`  | CRESTICT  | concurrent-call / shared-pcard restriction (too many simultaneous calls) |
| `-7`  | NO_TARIFF | no tariff / rate for the destination |
| `-8`  | NO_CREDIT | not enough credit for even one billing unit (computed maxsec ≤ 0) |
| `-9`  | NO_PRE    | internal allocation error |
| `-10` | NO_CCTBL  | call table full (`SimCalls` reached) |

(`-5` is unused.)

### my_cc reply literals

| Reply | Meaning |
|---|---|
| `ok`    | `term` accepted / `status` alive |
| `nok`   | `term` for an unknown `call_uid` (not in the table) |
| `empty` | request recognized but produced no data |
| `error` | request could not be parsed |

### jsonrpc_cc errors

Two distinct channels:

* **Business failures** come back *in-band* in a normal `result` — a negative
  `maxsec` (codes above), or `term` → `{"result":{"status":"nok"}}`.
* **Malformed / unrecognized requests** come back as a JSON-RPC error object:
  `{"jsonrpc":"2.0","error":{"code":<n>,"message":"..."},"id":<id>}` — standard
  JSON-RPC codes (e.g. `-32600` Invalid Request) plus these internal codes:

| Code | Meaning |
|---|---|
| `-50` | params object missing |
| `-51` | maxsec params missing |
| `-52` | balance params missing |
| `-53` | term params missing |
| `-54` | `term` `call-uid` not found in the call table |
| `-59` | unknown event / method |

---

## Notes

* The functional handlers are `maxsec`, `term`, and `status`/`state`. `balance`,
  `rate` and `cprice` are accepted and return a well-formed reply, but their
  handlers are not yet complete (see [call_control.md](call_control.md)).
* Everything here assumes the CallControl server is running with the matching
  interface loaded — the modules built and `CallControl` active in
  `RateEngine7.xml`, with `rt.so` loaded (online `maxsec` is computed there).
