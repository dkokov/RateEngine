# Offline rating golden regression

Full-pipeline regression for the **offline Rating engine (`rt.so`)**: it loads a
synthetic schema + fixture into a throwaway database, rates a set of known CDRs,
and asserts the produced per-CDR price and billed seconds against hand-computed
golden values.

Where `tariff_pricing.c` (unit test) pins the pricing *formula*, this pins the
whole *pipeline* — the `calling_number → billing_account → bill_plan → rate →
prefix → tariff → calc_function` lookup chain, the time-condition/pcard gates,
and the write-back into the `rating` table.

> For **throughput** and **`rt.so` vs `rt_duckdb.so` billing parity** on real
> data, see [BENCHMARKS.md](BENCHMARKS.md).

## Data policy

**All fixture data is synthetic.** No customer data is used. The only shipped
input is the structure-and-lookups schema `src/scripts/sql/rt_pgsql_v2.sql`; the
account/tariff/rating rows (`fixture.sql`) and CDRs (`cdrs_seed.sql`) are
invented for the test.

## What it checks

Three tariff shapes, priced via `calc_cprice_2` (`units = ceil(billsec/delta)`):

| CDR | tariff | billsec | golden price | golden billed |
|-----|--------|---------|--------------|---------------|
| `gold-persec` | `delta=1, fee=0.02, iter=0` | 125 | 125·0.02 = **2.50** | **125** |
| `gold-permin` | `delta=60, fee=0.10, iter=0` | 90 | ceil(90/60)·0.10 = **0.20** | **120** |
| `gold-tier` | `delta=60,fee=0.30,iter=1` + `delta=60,fee=0.10,iter=0` | 150 | 0.30 + 2·0.10 = **0.50** | **180** |

Price is compared with a small tolerance (float), billed seconds exactly. A CDR
may split into more than one `rating` row (free-billsec), so the comparison uses
`SUM(call_price)`/`SUM(call_billsec)` grouped by `call_id` — here there is no
split (tariffs set `free_billsec_id=0`), but the aggregate is future-proof.

## Running

Needs an installed engine (`make install` → modules `pgsql.so`, `cdrm.so`,
`rt.so`) and a PostgreSQL the DB user can create a database on.

```sh
# from src/ :
make rating-regression

# or directly, overriding DB (defaults come from the installed engine config):
DBHOST=re7-db DBNAME=rate_engine DBUSER=re_admin DBPASS=... DBPORT=5432 \
  scripts/tests/rating/run_rating_regression.sh
```

Exit: `0` all golden matched, `1` a mismatch, `2` prerequisites missing.

## How it works

`run_rating_regression.sh`:
1. Creates a **throwaway** database (`re7_rating_test`, dropped on exit) — it
   never touches the engine's real DB; the configured DB is used only as the
   maintenance connection to issue `CREATE/DROP DATABASE`.
2. Loads `rt_pgsql_v2.sql` (schema+lookups) + `fixture.sql` + `cdrs_seed.sql`. CDR
   timestamps are a mid-month weekday injected at run time (satisfies the
   mon-sun time-condition and the current-period pcard, keeping the test
   date-independent).
3. Generates a config loading `pgsql/cdrm/rt` with `<Rating active="no">`, runs
   `RateEngine -c cfg -r a` (rates one batch), and polls until every CDR is
   rated (`leg_a > 0`).
4. Joins `rating` to `cdrs` and compares each CDR's aggregate to `golden.tsv`.

## Engines compared

The runner rates the same CDRs with each available engine on its own fresh load
of the fixture, then asserts:

- `rt.so` == golden
- `rt_duckdb.so` == golden  — **skipped** if `duckdb.so`/`rt_duckdb.so` are not
  installed (so the test still runs in rt.so-only environments)
- `rt.so` == `rt_duckdb.so`  (parity)

The two-tier case (`gold-tier`) is the discriminating one: `duckdb_match_diagnostic.sql`
noted the DuckDB path historically used `calc_function pos=1` only and rated
`rate` directly (vs `/Rating` via `bill_plan_tree` + all `pos`). Our fixtures use
direct rates (no `bill_plan_tree`), so the remaining question is whether DuckDB
now honors all tariff tiers. If it diverges on `gold-tier`, this test reports it
as a real finding rather than hiding it.
