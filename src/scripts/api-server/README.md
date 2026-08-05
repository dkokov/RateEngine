# RE7 API server (`rateengine/re7-api`)

JSON/HTTP provisioning API for RE7, on top of `rateengine/re7-lib`. Step-by-step
resource operations (create bill plan, tariff, prefix, rate, account, reports) —
**bulk CSV import stays in the CLI** (`../cli/py_cli`). JWT bearer auth backed by a
local **SQLite** auth store; provisioning writes go to the **RE7 billing DB** (remote,
via re7-lib). Rating and heavy reports are **delegated to the engine**, never
computed here.

## Two data stores

| Store | Where | Holds |
|-------|-------|-------|
| Auth (SQLite) | local to this server/container | users, refresh tokens, audit log |
| RE7 billing (Postgres) | remote, via re7-lib | bill plans, tariffs, accounts, rates … |

Config is in `.env` (see `.env.example`): `API_AUTH_DB`, `RE7_DB_*`, `JWT_*`.

## Auth

- `POST /auth/login {username,password}` → `{access_token, refresh_token, expires_in}`
- `POST /auth/refresh {refresh_token}` → new pair (old refresh is revoked/rotated)
- `POST /auth/logout {refresh_token}` → revoke
- All other routes require `Authorization: Bearer <access>` and a scope
  (`provisioning:read` / `provisioning:write` / `rating:read` / `admin`).

Users are managed from the CLI (passwords hashed with `password_hash`):

```bash
php bin/re7-api-user.php init                       # create/upgrade the auth schema
php bin/re7-api-user.php add <user> --role admin    # prompts for a password (min 8)
```

## Local development

```bash
composer install
cp .env.example .env          # set RE7_DB_* + a JWT_SECRET
php bin/re7-api-user.php add dev --role admin
php -S 127.0.0.1:8099 -t public
```

Then:

```bash
BASE=http://127.0.0.1:8099
curl -s $BASE/health
ACCESS=$(curl -s -X POST $BASE/auth/login -H 'Content-Type: application/json' \
  -d '{"username":"dev","password":"…"}' \
  | php -r '$d=json_decode(stream_get_contents(STDIN),true);echo $d["access_token"]??"";')
curl -s -X POST $BASE/bill-plans -H "Authorization: Bearer $ACCESS" \
  -H 'Content-Type: application/json' -d '{"name":"PLAN_X","type":"prepaid"}'
```

> Note: the `re7-lib` dependency is a path repo with `symlink:false`, so `vendor/`
> is self-contained (portable to the deploy host). After editing `re7-lib`, run
> `composer install` again to refresh the copy in `vendor/`.

## Endpoints (current)

| Method(s) | Path | Scope |
|-----------|------|-------|
| GET | `/health` | public |
| POST | `/auth/login` · `/auth/refresh` · `/auth/logout` | public |
| POST · GET | `/bill-plans` · `/bill-plans/{name}` | write · read |
| POST · GET | `/tariffs` · `/tariffs/{name}` | write · read |
| POST · GET · DELETE | `/tariffs/{name}/calc-functions[/{pos}]` | write · read · write |
| POST · GET · DELETE | `/tariffs/{name}/time-conditions[/{id}]` | write · read · write |
| POST · GET | `/prefixes` · `/prefixes/{prefix}` | write · read |
| POST · GET | `/rates` · `/rates?bill_plan=` | write · read |
| POST · GET | `/free-billsec` | write · read |
| POST · GET · PATCH · DELETE | `/accounts` · `/accounts/{username}` | write · read · write · write |
| POST | `/accounts/{username}/numbers` | write |
| GET · PATCH | `/numbers/{number}` · `/numbers/{number}/bill-plan` | read · write |
| POST · GET | `/accounts/{username}/pcards` | write · read |
| PATCH | `/pcards/{id}/status` · `/pcards/{id}/limit` | write |
| GET | `/accounts/{username}/balance` | read |
| POST · GET · DELETE | `/services` · `/services/{username}` | write · read · write |
| GET | `/ref/{resource}` (currencies, pcard-types/statuses, round/rating-modes, bill-plan-types) | read |
| GET | `/reports/rated-calls?account=&from=&to=&limit=` | rating:read |

_Not yet wired: `POST /rate` (engine `cprice`/`rate` are stubs)._

## Curl cheat-sheet

All calls are HTTPS with a self-signed cert (`-k`). First create a user, then log
in and reuse the token (`$AT`). Replace `apiuser` / `s3cret-pass` with your own.

```bash
# one-time: create an API user in the SQLite auth store
php bin/re7-api-user.php add apiuser --role admin      # prompts for a password

BASE=https://127.0.0.1:8443
# login -> capture the access token
AT=$(curl -k -s -X POST $BASE/auth/login -H 'Content-Type: application/json' \
      -d '{"username":"apiuser","password":"s3cret-pass"}' \
    | php -r '$d=json_decode(stream_get_contents(STDIN),true);echo $d["access_token"]??"";')
H="Authorization: Bearer $AT"

# rate plan: plan -> prefix/tariff -> rate -> pricing formula
curl -k -s -X POST $BASE/bill-plans -H "$H" -H 'Content-Type: application/json' -d '{"name":"PLAN_A","type":"prepaid"}'
curl -k -s -X POST $BASE/tariffs    -H "$H" -H 'Content-Type: application/json' -d '{"name":"TAR_STD"}'
curl -k -s -X POST $BASE/prefixes   -H "$H" -H 'Content-Type: application/json' -d '{"prefix":"359"}'
curl -k -s -X POST $BASE/rates      -H "$H" -H 'Content-Type: application/json' -d '{"bill_plan":"PLAN_A","prefix":"359","tariff":"TAR_STD"}'
curl -k -s -X POST $BASE/tariffs/TAR_STD/calc-functions -H "$H" -H 'Content-Type: application/json' -d '{"pos":1,"delta_time":60,"fee":"0.05","iterations":1}'

# subscriber: whole service in one call, then check it
curl -k -s -X POST $BASE/services -H "$H" -H 'Content-Type: application/json' \
     -d '{"username":"ACC1","number":"35910000001","bill_plan":"PLAN_A","pcard":{"amount":20,"status":"active"},"balance":{"amount":0}}'
curl -k -s -H "$H" $BASE/services/ACC1

# change ops
curl -k -s -X PATCH $BASE/numbers/35910000001/bill-plan -H "$H" -H 'Content-Type: application/json' -d '{"bill_plan":"PLAN_A"}'
curl -k -s -X POST  $BASE/accounts/ACC1/pcards -H "$H" -H 'Content-Type: application/json' -d '{"amount":50,"status":"active"}'
curl -k -s -H "$H" $BASE/accounts/ACC1/balance

# reference lists + report
curl -k -s -H "$H" $BASE/ref/currencies
curl -k -s -H "$H" "$BASE/reports/rated-calls?account=ACC1&limit=5"

# teardown
curl -k -s -X DELETE $BASE/services/ACC1 -H "$H"
```

## Tests

Black-box functional + performance harness lives in
[`scripts/tests/api`](../tests/api/README.md):

```bash
cd ../tests/api
BASE=https://127.0.0.1:8443 API_USER=apiuser API_PASS=s3cret-pass ./run_api_test.sh   # 24 assertions
BASE=https://127.0.0.1:8443 API_USER=apiuser API_PASS=s3cret-pass ./api_perf.sh /ref/currencies 1000 16
```

## Deploy (nginx + php-fpm, HTTPS only on :8443)

```bash
sudo ./install.sh                       # self-signed cert, defaults
sudo ./install.sh --path /srv/re7-api --server-name api.example --port 8443
sudo ./install.sh --cert /path/fullchain.pem --key /path/privkey.pem   # provided cert
sudo ./install.sh --uninstall
```

Defaults (Fedora), each overridable by flag:

| Flag | Default | Meaning |
|------|---------|---------|
| `--path` | `/usr/share/nginx/re7-api` | app root (nginx serves `…/public`) |
| `--server-name` | host FQDN | TLS/vhost name |
| `--port` | `8443` | HTTPS port (no port 80, no redirect) |
| `--nginx-conf` | `/etc/nginx/conf.d` | where the vhost conf is written |
| `--fpm-pool` | `/etc/php-fpm.d` | where the pool conf is written |
| `--user` | `nginx` | php-fpm pool user |
| `--cert` / `--key` | (self-signed) | provide your own cert/key |

The installer: runs `composer install`, copies the app, writes `.env` (generates
`JWT_SECRET`; **you must edit `RE7_DB_*`**), generates a self-signed cert via
`../gen_tls_cert.sh` (or uses `--cert/--key`), renders the nginx vhost + a dedicated
php-fpm pool, initialises the SQLite auth store, fixes permissions, and reloads
services. TLS 1.2/1.3, HSTS, `limit_req` on `/auth/login`, front-controller-only
(`.php` elsewhere → 404).

After install: edit `.env` → `RE7_DB_*`, create a user, then
`curl -k https://<host>:8443/health`.

## Security notes

- HTTPS only + HSTS; keys `chmod 600`; `.env` `chmod 600`.
- JWT alg pinned; short access token + rotating refresh.
- Passwords hashed (`password_hash`), timing-safe verify, never logged.
- Give the RE7 DB role **least privilege** (provisioning tables only, not superuser).
- Every write is recorded in `audit_log` (who / method / path / status / ip).
