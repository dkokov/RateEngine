# RE7 API tests (`scripts/tests/api`)

Black-box tests for the provisioning API (`scripts/api-server`). They run against
a **real, running** API server backed by a reachable RE7 DB — nothing is mocked.

## Files
- `lib_api.sh` — shared helpers (login, `req`, `assert_code`, `assert_contains`, counters)
- `run_api_test.sh` — functional smoke over the whole provisioning surface, with cleanup
- `api_perf.sh` — throughput/latency for read paths (login once, then load)

## Prerequisites
- API server reachable at `$BASE` (default `https://127.0.0.1:8443`, self-signed → `curl -k`)
- A provisioning/admin user in the API auth store (`bin/re7-api-user.php add apiuser --role admin`)
- `php` on PATH (used to parse JSON); `ab` or `hey` optional (better perf stats)

## Functional test
```bash
BASE=https://127.0.0.1:8443 API_USER=apiuser API_PASS=s3cret-pass \
  ./run_api_test.sh
# exit 0 = all assertions passed; PFX=<name> to change the test-object prefix
```
Covers: health, login, bill-plans/tariffs/prefixes/rates, calc-functions, reference
lists, CreateService/CheckService/DeleteService, ChangeBillPlan, pcard create/limit/
status, balance read, and negative paths (404 / 401). It tears the test account down
via DeleteService; shared plan/tariff/prefix rows (prefixed `APITEST_`) are left.

## Performance
```bash
# public, no auth/DB
BASE=https://127.0.0.1:8443 ./api_perf.sh /health 5000 64
# authenticated read (JWT + DB): logs in once, reuses the token
BASE=... API_USER=apiuser API_PASS=s3cret-pass ./api_perf.sh /ref/currencies 2000 32
BASE=... API_USER=apiuser API_PASS=s3cret-pass ./api_perf.sh /bill-plans/SOMEPLAN 2000 32
```
Uses `hey`/`ab` if present (latency percentiles), else parallel `curl` (coarse req/s).
Login is deliberately hit only once — nginx rate-limits `/auth/login`.
