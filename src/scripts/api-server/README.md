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

| Method | Path | Scope |
|--------|------|-------|
| GET  | `/health` | public |
| POST | `/auth/login` · `/auth/refresh` · `/auth/logout` | public |
| POST | `/bill-plans` | provisioning:write |
| GET  | `/bill-plans/{name}` | provisioning:read |

_To come: tariffs, prefixes, rates, accounts, pcards, balance (read), reports, rating (delegated)._

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
