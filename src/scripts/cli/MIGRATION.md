# RE6Commander CLI — PHP → Python 3 Migration Plan

Full **1:1 port** of the PHP admin/provisioning CLI under `scripts/cli/` to Python 3.
Scope: every function in every `lib/*.php` file, the `RE6Commander` entry point, and the
`config.app.php` configuration — preserving behaviour, while fixing the structural
problems the PHP carries (SQL injection, `goto`, `exit()`, global connection state).

This document is the contract for the migration. Work it phase by phase; each phase is
independently testable against the existing PHP using the `test_bp/*.csv` fixtures.

---

## 1. What we are porting

| PHP file | LOC | Role | Python target |
|----------|----:|------|---------------|
| `RE6Commander` | ~40 | CLI entry, `switch` on `-f/-d/-t/-p` | `re6commander.py` |
| `config.app.php` | 30 | DB creds, MyCC host/port, app info, opens `$dbconn` | `config.py` + `.env` |
| `lib/lib.db.php` | 1794 | ~120 RE6 schema getters/inserters/updaters | `re6/db.py` (split, see §4) |
| `lib/lib.re5.php` | 1958 | ~110 `re5_*` account/report/CRUD funcs, 2 DBs | `re5/*.py` |
| `lib/lib.importing.php` | 196 | CSV → DB dispatcher (`config_mode` 1–5, `*`) | `importer.py` |
| `lib/lib.dumping.php` | 57 | bill plan → CSV | `dumper.py` |
| `lib/lib.cdrserver.php` | 190 | FreeSWITCH CDR XML → `fs_cdrs` | `cdrserver.py` |
| `lib/lib.testing.php` | 78 | bulk test-account generator | `testing.py` |
| `lib/lib.removing.php` | 48 | **NOT code** — pasted SQL scratch notes | drop (keep as `notes/removing.sql`) |

`test_bp/` CSV fixtures and `testing_phone_number.csv` are data — carried unchanged.

---

## 2. Target architecture

```
scripts/cli/
├── re6commander.py          # argparse entry point (replaces RE6Commander + switch)
├── pyproject.toml           # deps: psycopg[binary]>=3.1
├── .env.example             # DB creds template (replaces hardcoded config.app.php)
├── re_cli/
│   ├── __init__.py
│   ├── config.py            # loads .env -> Config dataclass (RE6 + RE5 + MyCC + RE globals)
│   ├── database.py          # Database class: connection, fetch_one/val/all, exec_returning
│   ├── re6/
│   │   ├── db.py            # all lib.db.php functions, grouped (see §4.3)
│   │   ├── importer.py      # lib.importing.php
│   │   ├── dumper.py        # lib.dumping.php
│   │   └── cdrserver.py     # lib.cdrserver.php
│   ├── re5/
│   │   ├── accounts.py      # billing/rating account CRUD
│   │   ├── balances.py      # balance + pcard
│   │   ├── reports.py       # tariff/rating/operator reports
│   │   └── orchestration.py # re5_create_account()
│   └── testing.py           # lib.testing.php
├── notes/removing.sql       # ex-lib.removing.php (reference only)
└── MIGRATION.md             # this file
```

Two databases exist (RE6 = `re7`/`fs_cdrs`, RE5 = legacy). Model them as **two `Database`
instances** held on the `Config`, not module globals. `re6_*`/`re5_*` functions take a
`db` argument (or live as methods on a class bound to one connection).

---

## 3. Cross-cutting conventions (apply to EVERY ported function)

These rules turn the PHP idioms into safe, idiomatic Python. Decide them once, apply
mechanically.

1. **Parameterized SQL — mandatory.** Every query becomes
   `cur.execute("... WHERE x = %s", (val,))`. No f-strings/`.format`/concatenation of
   values into SQL. ~200 call sites; this is the bulk of the work and the main payoff.
   - Dynamic *identifiers* (e.g. `lib.db.php` builds `leg = "leg_".$_leg`) use
     `psycopg.sql.Identifier`, never string interpolation.
2. **Connection state → object.** `global $dbconn`/`$RE`/`$RE6` → `Database` instance
   passed in. No module-level mutable connection.
3. **`goto` insert-retry → `INSERT ... RETURNING id`.** The PHP pattern
   `check_N: id = get_x(); if(!id){ insert_x(); goto check_N; }` collapses to a single
   `INSERT ... ON CONFLICT DO NOTHING RETURNING id` (or `get` then insert-returning).
   Affects: `lib.importing.php` (`check_2..check_6`) and `lib.re5.php`
   `re5_create_account()` (lines ~1876–1955).
4. **`exit()` → exceptions.** Define `CliError(Exception)`. Replace every `exit;` /
   `die()` with `raise CliError(msg)`; the entry point catches and prints + sets exit code.
   Affects `lib.db.php` inserters (`insert_time_condition`, `insert_rating_account*`),
   `lib.importing.php`.
5. **Result access → named columns.** `pg_fetch_row()[0]` → use `psycopg.rows.dict_row`
   or `namedtuple` cursors. Replace numeric indices with column names while porting (this
   also documents the schema).
6. **Empty/return semantics.** PHP returns `0`/`""` for "not found". Standardise:
   getters return `None` for no row, `[]` for no rows. Callers test `is None`, not truthiness
   (avoid the `if($id)` trap where id `0` is valid).
7. **`empty()` → explicit.** Port `empty($x)` to the precise intent (`x is None`,
   `not x`, `x in (None, "", 0)`) — decide per call site, don't blanket-translate.
8. **PHP `date()`/`mktime()`/`mt_rand()`** → `datetime` / `time.time()` /
   `random` (these appear in `lib.cdrserver.php` and `re5_pcard_deactive`).
9. **`explode(",", $line)` → `csv` module.** The importer/dumper must use `csv.reader`/
   `csv.writer` (handles the quoted `"BillPlan"` fields correctly; PHP's `trim($x,'"')`
   hand-rolling is fragile).

---

## 4. File-by-file mapping

### 4.1 `RE6Commander` → `re6commander.py`
Replace the `switch($opt)` with `argparse` subcommands:

| PHP | Python |
|-----|--------|
| `-f file` → `re6_import_settings($file)` | `import <file>` → `importer.import_settings(db, file)` |
| `-d bplan` → `re6_dump_bplan_settings($bplan)` | `dump <bplan>` → `dumper.dump_bplan(db, bplan)` |
| `-t` (empty) | `test` subcommand → wire to `testing.py` (PHP stub was empty) |
| `-p` (empty) | `perf` subcommand → stub, document as TODO |

Keep the original short flags as aliases if you want muscle-memory compatibility.

### 4.2 `config.app.php` → `config.py` + `.env`
- Move all creds to `.env` (gitignored); commit `.env.example`.
- `Config` dataclass fields: `re6_{host,name,user,pass}`, `re5_{...}`, `cc_host`,
  `cc_port`, `billing_day`, `credit_limit`, `rounding`.
- **Gap to resolve:** the PHP reads `$RE6[...]` (lib.re5.php:48) and a `$DAYS` array
  (lib.db.php `compare_tc_ts`) that are **not defined in any file present**. Locate their
  real definitions (other include, or runtime) before porting `re5_conn`/`compare_tc_ts`,
  or these features are dead. Track as **OPEN-1**.

### 4.3 `lib.db.php` → `re6/db.py`
~120 functions. Port grouped (one module section or class per group). Spot-fixes flagged.

- **Connection:** `conn()` → `Database.connect()`.
- **Cards/balance:** `get_debit_card`, `get_credit_card`, `get_pcard_type_id`,
  `get_pcard_status_id`, `get_pcard_id`, `get_balance`, `get_credit_card_id`,
  `get_debit_card_id`, `get_balance_id`, `make_balance_debit`, `make_balance_credit`.
- **Accounts:** `find_billing_account_id`, `find_billing_account_data(_2)`,
  `get_billing_account_id`, `get_rating_account_id`.
- **Bill plans:** `get_bill_plan_id`, `get_bill_plans(_v2)`, `get_bill_plans_tree`,
  `get_bill_plan_type_id`, `find_bill_plan_periods`, `find_bill_plan`, `get_bill_tree_id`.
- **Tariffs/rates:** `get_tariff_id`, `get_tariff_name`, `get_rate_id`, `get_rate_data`.
- **Prefixes/time conditions:** `get_prefix_id`, `get_prefix_filters`,
  `get_time_condition_id`, `find_time_condition`, `get_tc_name(_2)`, `find_celebr_days`,
  `find_celebr_dt_deff` *(buggy: unused `$dt`, returns `$tc` — review intent, **FIX-1**)*.
- **Calc/free billsec:** `get_calc_functions(_v2)`, `get_calc_id`, `get_free_id`,
  `get_free_billsec(_v2/_id)`, `get_free_bill`.
- **CDR:** `get_cdr(_2)`, `get_cdr_servers`, `get_dbstorage`, `get_calling_number_cdrs`,
  `get_accountcode_cdrs`, `get_norating_cdr` *(commented in PHP — port or drop, decide)*.
- **Rating/balance/reports:** `get_bacc_balance`, `get_rating_mode`, `check_bill`,
  `check_bill_tariff` *(unused)*, `check_free_bill`, `get_tariff_stat`,
  `get_rating_calls`, `get_norating_calls`.
- **Inserts:** `write_rating_in_db`, `write_balance`, `insert_cdrs_into(_2)`, `mark_cdr`,
  `insert_bill_plan(_v2)`, `insert_bill_tree`, `insert_credit_card`, `insert_debit_card`,
  `insert_balance`, `insert_tariff`, `insert_billing_account`, `insert_rate`,
  `insert_free_billsec`, `insert_pcard`, `insert_time_condition`, `insert_prefix`,
  `insert_rating_account(_2)`, `insert_calc_func`. → all use `RETURNING id`, drop the
  post-insert reselect the callers do.
- **Updates:** `update_rating_id_cdrs`, `clear_rating` *(incomplete — no execute, **FIX-2**)*.
- **Util:** `compare_prefix`, `compare_tc_ts` *(needs `$DAYS`, see OPEN-1)*,
  `get_ts` *(**FIX-3**: lib.db.php:850 uses undefined `$date` — confirm correct source var)*.
- **Dead code to drop:** commented `find_rate()`, the duplicated block ~733–751,
  duplicate `get_tc_name_2`.

### 4.4 `lib.re5.php` → `re5/` package
~110 `re5_*` functions across two DB connections (`$RE`, `$RE6`). Split:
- `re5/database.py` — `re5_conn/close/query/begin/commit/rollback`,
  `re6_conn/close/query` → `Re5Db`/`Re6Db` classes with real
  `with db.transaction():` context managers (replaces manual BEGIN/COMMIT/ROLLBACK).
- `re5/accounts.py` — all `re5_get_bacc*`, `re5_insert/update/delete_billing_account*`,
  rating-account family (`re5_get_racc*`, `re5_insert_rating_account(_2)`,
  `re5_delete_racc*`, `re5_insert_clg_history`).
- `re5/balances.py` — balance family (`re5_get_balance*`, `re5_*_balances`,
  `re5_update_balance_*`, `re5_get_bill_file`, free-billsec-balance) + pcard family
  (`re5_get_pcard*`, `re5_insert_pcard(_2)`, `re5_update_pcard*`, `re5_*_credit_limit*`,
  `re5_pcard_deactive`, `re5_delete_pcard*`).
- `re5/reports.py` — `re5_get_tariff_report`, `re5_get_rating_stat`,
  `re5_get_rated_calls_report(_2)`, `re5_get_operator_*`, `re5_get_nsg_counters`,
  `re5_get_tariff_name`, lookups (`re5_get_rating_modes/mode/mode_id`, `re5_get_prefix`,
  `re5_get_currency`, `re5_get_cdr_servers`, `re5_get_round_modes`, bill-plan getters).
- `re5/orchestration.py` — `re5_create_account()` (the goto-retry orchestrator) +
  `re6_insert_sms_cdr`/`re6_get_id_sms_cdr`. Rewrite as a single transaction:
  get-or-create billing account → rating account → pcard, using `RETURNING`, wrapped in
  `with re5db.transaction():` so failure rolls back atomically (the PHP "transaction" was
  fake — it used flags + goto, never rolled back).

### 4.5 `lib.importing.php` → `re6/importer.py`
Dispatch on the first CSV column (`config_mode`). Keep the state machine but use `csv`:

| mode | meaning | PHP retry → Python |
|-----:|---------|--------------------|
| `1` | Bill plan | `get_or_create` via RETURNING |
| `2` | Prefix + tariff + rate (+free billsec) | `check_2/3/4` → 3× get-or-create |
| `*` | Calc function (depends on current tariff) | insert if absent |
| `3` | Billing + rating account | `check_5/6` → get-or-create |
| `4` | Pcard | insert if absent |
| `5` | Bill plan tree (root + children) | loop, insert tree links |

Carry the cross-row state (current `bill_plan_id`, `tariff_id`, `billing_account_id`) as
an explicit object, not loop-scoped PHP vars. The commented free_billsec/time_condition
blocks (modes 2) are disabled in PHP — replicate as-is, leave TODO.

### 4.6 `lib.dumping.php` → `re6/dumper.py`
`dump_bplan(db, name)`: resolve id → tree or single plan → for each: rates → free billsec
→ calc functions → write CSV rows. Use `csv.writer` to a stream; preserve the exact header
and column layout so dump output stays diff-comparable with the PHP.

### 4.7 `lib.cdrserver.php` → `re6/cdrserver.py`
`insert_cdr(arr)` — port the nested-key validation with `dict.get(...)` chains; the
INSERT (30+ cols) parameterized. Migrate `date()/mktime()/mt_rand()`. The commented
`objectsIntoArray()` / `myCC_term()` / `lib.mycc.php` socket bits: port only if MyCC
integration is actually used (track as **OPEN-2**); otherwise omit.

### 4.8 `lib.testing.php` → `testing.py`
`create_test_calling_number_racc(num, start, bplan, sm_bplan)` — straight port over the
`re5/orchestration.py` create path. Replace `define()` constants with module constants /
argparse args. Reads `testing_phone_number.csv`.

---

## 5. Phased execution checklist

- [ ] **Phase 0 — Setup & open questions**
  - [ ] Resolve OPEN-1 (`$RE6`/`$DAYS` real source), OPEN-2 (MyCC used?).
  - [ ] Confirm target Python/psycopg version (env has Python 3.13; psycopg not yet installed).
  - [ ] Snapshot a scratch DB schema for parity testing.
- [ ] **Phase 1 — Foundation**
  - [ ] `pyproject.toml`, `.env.example`, `config.py`, `database.py` (fetch helpers,
        `transaction()` context manager, `dict_row` cursors, `CliError`).
  - [ ] `re6commander.py` argparse skeleton (subcommands wired to stubs).
- [ ] **Phase 2 — RE6 data layer** (`re6/db.py`): port all ~120 functions per §4.3,
        parameterized, RETURNING-based inserts. Fix FIX-1/2/3. Unit-test getters against
        a seeded DB.
- [ ] **Phase 3 — Import/Dump** (`importer.py`, `dumper.py`): the primary CLI workflows.
  - [ ] **Parity gate:** import every `test_bp/*.csv` with PHP into DB-A and Python into
        DB-B; `pg_dump --data-only` both; diff. Must match (modulo serial ids).
  - [ ] Dump a known bill plan with both; diff CSV output byte-for-byte.
- [ ] **Phase 4 — RE5 package** (`re5/*`): accounts, balances, reports, orchestration.
        Rewrite `re5_create_account` as a real transaction. Test create→read→delete cycle.
- [ ] **Phase 5 — CDR server & testing** (`cdrserver.py`, `testing.py`).
- [ ] **Phase 6 — Cutover**
  - [ ] Update `test_bp/readme` examples to the new CLI invocation.
  - [ ] Move PHP `lib/` + `RE6Commander` to `legacy_php/` (don't delete until parity signed off).
  - [ ] Document new usage in this dir's README.

---

## 6. Bugs / gaps found in the PHP (fix or consciously preserve)

| ID | Location | Issue | Action |
|----|----------|-------|--------|
| FIX-1 | `lib.db.php` `find_celebr_dt_deff` | unused `$dt`, returns `$tc` — logic looks wrong | review intent before porting |
| FIX-2 | `lib.db.php` `clear_rating` | builds query, never executes | confirm intended, then implement or drop |
| FIX-3 | `lib.db.php:850` `get_ts` | `explode("-",$date)` — `$date` undefined | find correct source var |
| OPEN-1 | `lib.re5.php:48`, `compare_tc_ts` | `$RE6`/`$DAYS` not defined in any present file | locate definition or mark feature dead |
| OPEN-2 | `lib.cdrserver.php` | `lib.mycc.php` / `myCC_term` socket integration commented out | confirm if MyCC is still used |
| SEC-1 | all libs | ~200 string-concatenated queries (SQL injection) | fixed by §3.1 parameterization |
| SEC-2 | `config.app.php`, `lib.cdrserver.php` | hardcoded DB credentials | fixed by `.env` |

---

## 7. Notes
- The CLI is **not** on the C rating engine's runtime path — migrating it is low risk to
  production rating and can proceed independently of the branch work (0.7.x).
- PHP 8.4 is installed but the legacy `pg_*` extension may not be — verify the PHP tool
  still runs before relying on it as the parity oracle; if it doesn't, parity must be done
  against a known-good DB snapshot instead.
- `lib.removing.php` is not a program; preserve its SQL as `notes/removing.sql` only.
