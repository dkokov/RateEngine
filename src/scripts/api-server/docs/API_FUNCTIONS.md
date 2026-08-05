# RE7 API — function (handler) catalog

Derived from the **RE7 schema** (`src/scripts/sql/rt_pgsql.sql`) and RE7's actual
functionality (rating flow, CallControl, legacy `lib.re5.php`, a legacy SOAP provisioning service)
— **not** from prose docs. This is the authoritative list of endpoints the API
should expose. Bulk CSV import stays in the CLI; this API is step-by-step.

## Legend

- **scope**: `read` = provisioning:read · `write` = provisioning:write · `admin` · `rating`
- **G** = guarded by ActiveCallGate (refuse in-place edit/delete while calls are live)
- **D** = effective-dated (write a new version, never mutate a live row)
- **$** = money / engine-owned (API does not compute or freely mutate; restricted)
- **repo**: re7-lib repository — ✅ exists · ⏳ to add
- **P** = priority (1 = core / legacy parity, 2 = complete provisioning, 3 = advanced/infra)

---

## Implemented (current — deployed & tested over HTTPS :8443)

Auth (JWT bearer + SQLite user store, per-route scopes, audit log) plus the
provisioning and reporting endpoints below are live and tested against the RE7 DB.

| Method(s) | Path | Handler |
|---|---|---|
| GET | `/health` | — |
| POST | `/auth/login` · `/auth/refresh` · `/auth/logout` | AuthHandler |
| POST · GET | `/bill-plans` · `/bill-plans/{name}` | BillPlanHandler |
| POST · GET | `/tariffs` · `/tariffs/{name}` | TariffHandler |
| POST · GET | `/prefixes` · `/prefixes/{prefix}` | PrefixHandler |
| POST · GET | `/rates` · `/rates?bill_plan=` | RateHandler |
| POST · GET · DELETE | `/services` · `/services/{username}` | ServiceHandler |
| GET · PATCH | `/numbers/{number}` · `/numbers/{number}/bill-plan` (G) | NumberHandler |
| POST · GET | `/accounts/{username}/pcards` | PcardHandler |
| PATCH | `/pcards/{id}/status` · `/pcards/{id}/limit` (G) | PcardHandler |
| GET | `/accounts/{username}/balance` | BalanceHandler |
| GET | `/reports/rated-calls?account=&from=&to=&limit=` | ReportHandler |

- **`POST /services`** = CreateService: billing_account + calling_number(+bill plan)
  + optional pcard + optional balance, in one transaction, idempotent on account/number.
- **`DELETE /services/{username}`** = DeleteService: FK-safe teardown of the account's
  data; shared plan/tariff/prefix are kept.
- **(G)** endpoints run through the ActiveCallGate (currently a no-op until a live
  CallControl query is wired in P2).

**Not yet wired:** `POST /rate` — blocked: the engine's `cprice`/`rate` are stubs in
`mod/CallControl/cc.c`. Also granular account create/update and the P2/P3 items below.

---

## A. Rate-plan definition

| Endpoint | Op | scope | flags | repo | P |
|---|---|---|---|---|---|
| `POST /bill-plans` | create bill plan | write | D | BillPlan ✅ | 1 |
| `GET /bill-plans` · `GET /bill-plans/{name}` | list / get | read | | BillPlan ✅ | 1 |
| `PATCH /bill-plans/{name}` | update type/period | write | G D | BillPlan ⏳ | 2 |
| `DELETE /bill-plans/{name}` | remove | write | G | BillPlan ⏳ | 2 |
| `POST /bill-plans/{name}/tree` · `GET …/tree` | bill_plan_tree link | write/read | | BillPlanTree ⏳ | 2 |
| `POST /tariffs` · `GET /tariffs` · `GET /tariffs/{name}` | tariff create/list/get | write/read | D | Tariff ✅ | 1 |
| `PATCH /tariffs/{name}` | update period/free_billsec | write | G D | Tariff ⏳ | 2 |
| `POST /tariffs/{name}/calc-functions` · `GET` · `DELETE …/{pos}` | calc_function | write/read | G | CalcFunction ⏳ | 1 |
| `POST /tariffs/{name}/time-conditions` · `GET` | time_condition(_deff) | write/read | | TimeCondition ⏳ | 2 |
| `POST /prefixes` · `GET /prefixes` · `GET /prefixes/{prefix}` · `DELETE` | prefix | write/read | | Prefix ✅ | 1 |
| `POST /rates` · `GET /rates?bill_plan=` · `DELETE /rates/{id}` | rate (plan×prefix×tariff) | write/read | G | Rate ✅ | 1 |
| `POST /free-billsec` · `GET /free-billsec` | free_billsec | write/read | | FreeBillsec ⏳ | 2 |

## B. Subscriber / account provisioning

| Endpoint | Op | scope | flags | repo | P |
|---|---|---|---|---|---|
| `POST /accounts` | create billing_account | write | | BillingAccount ⏳ | 1 |
| `GET /accounts` · `GET /accounts/{username}` | list / get | read | | BillingAccount ⏳ | 1 |
| `PATCH /accounts/{username}` | currency/leg/billing_day/round_mode/day_of_payment | write | G | BillingAccount ⏳ | 2 |
| `DELETE /accounts/{username}` | remove (DeleteService) | write | G | BillingAccount ⏳ | 1 |
| `POST /accounts/{username}/numbers` | calling_number + _deff (bill_plan, sm_bill_plan) | write | | CallingNumber ⏳ | 1 |
| `GET /accounts/{username}/numbers` · `GET /numbers/{number}` | list / get | read | | CallingNumber ⏳ | 1 |
| `PATCH /numbers/{number}/bill-plan` | ChangeBillPlan | write | G D | CallingNumber ⏳ | 1 |
| `PATCH /numbers/{number}/sm-bill-plan` | secondary/SMS plan | write | G D | CallingNumber ⏳ | 2 |
| `DELETE /numbers/{number}` | remove | write | G | CallingNumber ⏳ | 2 |
| `GET /numbers/{number}/history` | clg_history | read | | ClgHistory ⏳ | 2 |
| `POST/GET /accounts/{username}/account-codes` | account_code(_deff) | write/read | | AccountCode ⏳ | 2 |
| `POST/GET /accounts/{username}/{src,dst}-contexts` | *_context(_deff) | write/read | | Context ⏳ | 3 |
| `POST/GET /accounts/{username}/{src,dst}-tgroups` | *_tgroup(_deff) | write/read | | Tgroup ⏳ | 3 |

## C. Balance & credit  ($ engine-owned)

| Endpoint | Op | scope | flags | repo | P |
|---|---|---|---|---|---|
| `GET /accounts/{username}/balance` | CheckUserBalance | read | $ | Balance ⏳ | 1 |
| `PATCH /balances/{id}/status` | ChangeBalanceStatus (active) | write | G $ | Balance ⏳ | 2 |
| `POST /accounts/{username}/pcards` | CreatePCard | write | $ | Pcard ⏳ | 1 |
| `GET /accounts/{username}/pcards` · `GET /pcards/{id}` | list / get | read | | Pcard ⏳ | 1 |
| `PATCH /pcards/{id}/status` | ChangePCardStatus (active/block) | write | G $ | Pcard ⏳ | 1 |
| `PATCH /pcards/{id}/limit` | UpdateCreditLimit (amount) | write | G D $ | Pcard ⏳ | 1 |
| `DELETE /pcards/{id}` | remove | write | G $ | Pcard ⏳ | 2 |
| `GET /accounts/{username}/free-billsec-balance` | free_billsec_balance | read | $ | FreeBillsecBalance ⏳ | 3 |

## D. Rating  (delegated to the engine — never priced in PHP)

| Endpoint | Op | scope | flags | repo | P |
|---|---|---|---|---|---|
| `POST /rate` | price a single call (clg,cld,billsec) | rating | $ | engine-delegate ⏳ | 1 |
| `POST /rate/batch` | price a set | rating | $ | engine-delegate ⏳ | 2 |

## E. Reports / analytics  (read-only; heavy = DuckDB/engine)

| Endpoint | Op | scope | flags | repo | P |
|---|---|---|---|---|---|
| `GET /reports/rated-calls` | GetRatedCallsReport (account/period) | rating | | Report ⏳ | 1 |
| `GET /reports/traffic` | operator/tariff traffic summary | rating | | Report ⏳ | 2 |
| `GET /cdrs` | query CDRs (paginated, filtered) | rating | | Cdr ⏳ | 2 |
| `GET /reports/bill/{username}` | invoice/bill file (GetBillFile) | rating | | (defer: PDF=GUI) | 3 |

## F. Reference data  (lookups; mostly read, admin create is rare)

| Endpoint | Tables | scope | P |
|---|---|---|---|
| `GET /ref/currencies` (+ `POST` admin) | currency | read/admin | 1 |
| `GET /ref/bill-plan-types` | bill_plan_type | read | 1 |
| `GET /ref/pcard-types` · `GET /ref/pcard-statuses` | pcard_type, pcard_status | read | 1 |
| `GET /ref/round-modes` · `GET /ref/rating-modes` | round_mode, rating_mode | read | 2 |

## G. System / infra  (admin)

| Endpoint | Tables | scope | P |
|---|---|---|---|
| `GET /health` · `GET /version` | version | public/read | 1 (done/❑) |
| `admin: users CRUD` | api_user (SQLite) | admin | 1 |
| `GET/POST /cdr-servers`, `/cdr-dbstorage`, `/cdr-profiles`, `/prefix-filters` | CDR ingestion config | admin | 3 (maybe CLI) |

---

## Legacy provisioning parity (proven external needs → all Priority 1)

CreateService → `POST /accounts` + `POST /accounts/{u}/numbers` (+ optional pcard);
DeleteService → `DELETE /accounts/{u}`; CheckService → `GET /accounts/{u}`;
ChangeServiceStatus → `PATCH /balances/{id}/status`; CreatePCard → `POST …/pcards`;
ChangePCardStatus / UpdateCreditLimit → `PATCH /pcards/{id}/…`;
CheckUserBalance → `GET …/balance`; ChangeBillPlan → `PATCH /numbers/{n}/bill-plan`;
ChangeBillingAccount → `PATCH /accounts/{u}`; CheckBillPlans → `GET /bill-plans`;
GetRatedCallsReport → `GET /reports/rated-calls`.
Out of scope (VoIPManager/NPS): ChangeDevice, ChangeUserPass, CheckOperatorsList, NP*.

## Build phases

- **P1 — core provisioning + legacy parity + rate/report delegate.**
  Handlers: bill-plans*, tariffs*, calc-functions, prefixes*, rates*, accounts,
  calling-numbers(+change-bill-plan), pcards(+status/limit), balance(read),
  rated-calls, `/rate` delegate, ref lists, admin users.
  re7-lib repos to add: CalcFunction, BillingAccount, CallingNumber, Pcard, Balance(read), Report.
- **P2 — complete provisioning:** updates/deletes with the gate, bill-plan-tree,
  time-conditions, free-billsec, account-codes, sm-bill-plan, clg_history, traffic report, cdrs query.
- **P3 — advanced/infra:** contexts/tgroups, free_billsec_balance, invoice/bill file,
  CDR-ingestion config (or leave in CLI).

## Notes / guards
- Every **Change\*** (PATCH) on data a live call can reference is **G** (active-call gate) and, for
  rate/plan/tariff, **D** (effective-dated) — see the risk analysis in the API-server plan.
- **$** rows: the API never computes price or freely rewrites balances; money moves through the engine.
- `(✅ done)`: `POST /bill-plans`, `GET /bill-plans/{name}`, auth, health.
