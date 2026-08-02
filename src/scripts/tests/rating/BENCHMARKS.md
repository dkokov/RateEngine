# Rating benchmarks & billing-parity tools

Three tools for **offline rating** performance and cross-engine correctness. They
complement the golden regression ([README.md](README.md), which pins billing
*correctness* on 3 synthetic CDRs): these measure *throughput* and *real-data
parity* between the two rating engines.

| script | question it answers |
|--------|---------------------|
| [`gen_bench_db.sh`](gen_bench_db.sh) | build a **synthetic** rating DB at scale (no real data) to benchmark against |
| [`rating_perf_report.sh`](rating_perf_report.sh) | how fast did a rating run go? (parse a log → ms/cdr, cdr/s) |
| [`bench_rating_replay.sh`](bench_rating_replay.sh) | throughput of `rt.so` vs `rt_duckdb.so` on a DB (safe, on a clone) |
| [`parity_real.sh`](parity_real.sh) | do `rt.so` and `rt_duckdb.so` **bill identically** on a DB? |

The perf/parity tools clone a **source** DB (`SRCDB`, default `rate_engine`). Point
them at a synthetic DB built by `gen_bench_db.sh` to benchmark **without any real
data**: `SRCDB=re7_bench ./bench_rating_replay.sh`.

There is also a build-toolchain benchmark one level up,
[`../bench_toolchain.sh`](../bench_toolchain.sh) (gcc vs clang: build time +
golden + price parity) — see its header comment.

## Safety

`bench_rating_replay.sh` and `parity_real.sh` **only read** the live database
(via `pg_dump`) and do all rating/mutation on **throwaway clones** whose names
must differ from the source (asserted). Live balances are never touched. All
fixture/seed data elsewhere is synthetic — no customer data is committed.

## Prerequisites

- An **installed** engine: `make install` (+ `make module_install name=RatingDuckDB`
  and `name=db_duckdb` for the DuckDB engine). Needs modules `pgsql.so`, `cdrm.so`,
  `rt.so`, and for DuckDB `duckdb.so` + `rt_duckdb.so` under `$RE_PREFIX/modules`.
- A reachable PostgreSQL whose user can **`CREATEDB`** (the clones). If not:
  `ALTER ROLE <user> CREATEDB;` or pass a superuser via `DBUSER`/`DBPASS`.
- `psql` and `pg_dump` in `PATH`.

DB credentials default to the installed engine config
(`$RE_PREFIX/config/RateEngine7.xml`), then to the CI defaults. Override with env:
`DBHOST DBPORT DBNAME DBUSER DBPASS` (and `RE_PREFIX`).

---

## 0. `gen_bench_db.sh` — synthetic benchmark DB (no real data)

Builds a persistent DB (default `re7_bench`) from the committed schema
`rt_pgsql.sql` filled with generated, **non-customer** data: 5 tariff plans,
`N_ACCOUNTS` subscribers (billing_account + calling_number + pcard), and
`N_CDRS` CDRs (random src from the accounts, dst from 10 prefixes, weekday
timestamps). CDRs are marked `leg_a=1` ("rated") so the clone-and-replay tools
below accept it as a production stand-in.

```sh
cd src/scripts/tests/rating
./gen_bench_db.sh                              # 50k accounts, 1e6 CDRs (default)
N_ACCOUNTS=5000 N_CDRS=100000 ./gen_bench_db.sh   # smaller

# then benchmark / parity-check against it (SRCDB, not DBNAME):
SRCDB=re7_bench ./bench_rating_replay.sh
SRCDB=re7_bench MODULE=rt_duckdb.so ./bench_rating_replay.sh
SRCDB=re7_bench ./parity_real.sh
```

It refuses to target the real DB name, and only uses the configured DB as the
maintenance connection to `CREATE/DROP` the bench DB. Tariffs set
`free_billsec_id=0`, so `rt.so` and `rt_duckdb.so` bill identically here
(parity is a clean `MATCH`); the free-billsec drawdown difference seen on real
data needs a dedicated free-billsec fixture to reproduce.

## 1. `bench_rating_replay.sh` — throughput on CDRs

Clones the live DB read-only, `VACUUM ANALYZE`s the clone (so query plans are
representative), resets the newest `N` already-rated CDRs to unrated, and re-rates
them with the chosen engine over `REPEAT` warm passes — reporting the **median**
ms/cdr and cdr/s (median cuts the shared-DB run-to-run variance). Drops the clone.

```sh
cd src/scripts/tests/rating

# rt.so (default), 20k CDRs, 4 threads, 3 passes:
LABEL=0.7.6-rt ./bench_rating_replay.sh

# the DuckDB offline engine on the identical workload:
MODULE=rt_duckdb.so LABEL=0.7.6-duckdb ./bench_rating_replay.sh
```

Key env: `MODULE` (`rt.so` | `rt_duckdb.so`), `N` (CDRs, default 20000),
`THREADS` (default 4), `REPEAT` (passes, default 3), `LABEL` (row label),
`KEEP=1` (keep the clone + workdir for inspection).

Output — per-pass lines then a median summary, and one row appended to
`baseline_results.tsv`:

```
    pass 1/3: 3.879 ms/cdr  (257.8 cdr/s)
    ...
  ms/cdr : median 4.080  (min 3.879  max 4.533)
  cdr/s  : median 245.100
```

## 2. `rating_perf_report.sh` — parse an existing run's log

If you already have a RateEngine log (e.g. from a production or manual run), this
extracts the per-batch timing without running anything. Parses both engine
formats (`rt.so`'s `batch times: … avg/cdr …` and `rt_duckdb.so`'s
`cycle done: processed … in … sec`).

```sh
./rating_perf_report.sh /usr/local/RateEngine/logs/rate_engine.log 0.7.6-prod
```

Reports overall ms/cdr, cdr/s, per-batch min/median/max, and an **early→late
degradation ratio** (flags runtime slowdown from table bloat). Needs the engine
run at `LogDebugLevel >= 1` (INFO) so the timing line is emitted.

## 3. `parity_real.sh` — do the two engines bill the same?

The gate before switching production offline rating from `rt.so` to
`rt_duckdb.so`. Clones the live DB once, makes two identical copies via
`TEMPLATE`, rates the same `N` real CDRs with `rt.so` on one and `rt_duckdb.so`
on the other **from identical starting state**, and diffs per-CDR
`SUM(call_price)`/`SUM(call_billsec)`.

```sh
./parity_real.sh            # N=20000, THREADS=4
```

Key env: `N`, `THREADS`, `TOL` (price abs tolerance, default 0.005), `KEEP=1`.
Exit 0 = all match, 1 = mismatch (lists up to 20 differing CDRs).

```
  CDRs compared: 20000
  match: 19624   price/billsec diff: 376   missing: 0
  RESULT: MISMATCH - DuckDB is NOT a drop-in for these CDRs (see above)
```

---

## Reading the numbers (measurement gotchas)

These were learned the hard way; ignore them and you'll chase ghosts.

- **The DB is shared → runs are noisy.** On a shared PostgreSQL with fresh clones,
  the same binary can vary ±25–50% run-to-run (buffer cache, autovacuum,
  checkpoints, other load). **Compare the *minimum* ms/cdr across runs, not a
  single median** — interference only ever makes a run *slower*, so the fastest
  observation is closest to the truth.
- **Quiesce the DB** while benchmarking: stop the production `RateEngine -d` and
  anything else hitting that PostgreSQL.
- **Always `VACUUM ANALYZE` a fresh clone** before timing (the harness does this):
  a `pg_dump` restore has no planner statistics → seq-scans → unrepresentative,
  inflated numbers.
- **Only large effects are trustworthy here.** A ~20% difference is inside the
  noise; a ~10× difference (e.g. DuckDB) is real. Don't over-interpret small deltas.
- **`ms/cdr` already accounts for threads.** It's wall-seconds / CDRs-in-batch, so
  4 threads rating 1000 CDRs in 1 s reads as 1.0 ms/cdr (not 4.0).

## Reference results (0.7.6, 20k real CDRs, 4 threads)

| engine | ms/cdr (median) | cdr/s | notes |
|--------|-----------------|-------|-------|
| `rt.so` | ~3–4 | 250–330 | query/RTT-bound; noisy (±25%) |
| `rt_duckdb.so` | **~0.30** | **~3380** | set-based JOIN; ~10× faster, stable (~3%) |

Billing parity: **98.1% of real CDRs identical**; the ~1.9% difference is the
free-billsec drawdown boundary (a known reconciliation item before adopting
DuckDB for offline). The core rating (rates/tariffs/tiers/billsec) matches exactly.
