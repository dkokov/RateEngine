# RatingDuckDB — analytical batch rating module

`RatingDuckDB` is a drop-in replacement for the classic `Rating` module (`rt.so`).
It rates completed CDRs **in bulk** using an embedded [DuckDB](https://duckdb.org)
engine instead of issuing 15–20 per-CDR SQL queries to PostgreSQL. It reproduces
the `/Rating` billing logic (account modes, multi-tier pricing, time conditions,
balance, free-seconds) but processes thousands of CDRs per cycle in one set of
analytical queries.

> **Scope:** offline/batch rating of finished calls. Real-time `maxsec` /
> credit-limit enforcement stays in **CallControl** (online). The two planes meet
> only at the `balance` / `free_billsec_balance` tables.

---

## 1. Principle (how it works)

The old module asks PostgreSQL one question per CDR (subscriber? bill plan? rate?
prefix? tariff? …). RatingDuckDB instead does, per cycle:

1. **Pin a window** — snapshot the next `BatchLimit` unrated CDRs
   (`leg=0`, ordered by id) into a DuckDB temp table `batch_window`.
2. **Resolve accounts set-based** — one `UNION` over all account-lookup modes
   (`calling_number`, `account_code`, `src/dst_context`, `src/dst_tgroup`, `sms`)
   in the same fallback order `/Rating` uses; pick the best match per CDR.
3. **Match rates** — JOIN through `bill_plan_tree` (nested plans) → `rate` →
   longest matching `prefix` → `tariff`.
4. **Price** — a recursive CTE replays `calc_cprice_2` (multi-tier: `delta`/`fee`/
   `iterations` steps) for every CDR at once. `round_billsec` and time conditions
   are applied in SQL.
5. **Split free/paid** — for free-allowance tariffs, a window function draws down
   the free pool in order and splits boundary calls into a free row (negative
   price) + a paid row (`double_rating`).
6. **Write back** — bulk `INSERT` into `rating`, set `cdrs.leg`, accumulate
   `balance`, maintain `free_billsec_balance`, mark the rest `leg=-1`.

The lookup/dimension tables (rates, tariffs, subscribers…) are **cached inside
DuckDB** so they aren't re-pulled from PostgreSQL every cycle.

### Engine layout
```
RateEngine (rt.so = RatingDuckDB)
   │  rt_duckdb_rate_batch()   ← the analytical cycle
   ├── db_duckdb engine (duckdb.so)  → embedded DuckDB (in-memory)
   │       └── ATTACH postgres (READ_ONLY)  ← reads cdrs/rating/balance live
   │       └── local TABLEs/VIEWs            ← cached dimensions
   └── pgsql.so (libpq)        → writes: rating / cdrs.leg / balance / free
```
`duckdb.so` is a first-class db-engine module (peer of `pgsql.so`), so any module
can use DuckDB through the standard `db_*` API.

---

## 2. Features (parity with `/Rating`)

| Feature | Status | Notes |
|---|---|---|
| Account modes (clg, account_code, src/dst_context, src/dst_tgroup, sms) | ✅ | rec-type fallback order matches `rt_main`/`rt_racc_*` |
| `bill_plan_tree` (nested plans) + longest-prefix match | ✅ | |
| Multi-tier pricing (`calc_cprice_2`) | ✅ | recursive CTE; validated by `unit_tests/tariff_pricing.c` |
| `round_billsec` (ceil/floor of `billusec`) | ✅ | |
| Time conditions (`tc_date` / day-of-week / hours, midnight-wrap) | ✅ | |
| Balance accumulation (period bill) | ✅ | credit→`billing_day` window, debit→card dates |
| `free_billsec` / `double_rating` (free/paid split) | ✅ | window running-sum drawdown |
| `free_billsec_balance` maintenance | ✅ | billing_day-window periods |
| `rating_mode_id`, `pcard_id`, `time_condition_id`, `free_billsec_id` | ✅ | written like `/Rating` |
| **SMS pricing** | ⚠️ better | flat-fee (the V6 `calc_cprice_sms` behaviour). NB: `/Rating` 0.7.x mis-prices SMS via `calc_cprice_group` |
| `maxsec` (online) | ➖ | out of scope — stays in CallControl |
| Count-based free-SMS | ⛔ | not yet ported |
| Debit-card free pools in `free_billsec_balance` | ⛔ | only billing_day-window periods covered |

---

## 3. Configuration

All parameters live in the `<Rating>` section of `RateEngine7.xml`:

```xml
<Rating>
    <param name="active" value="yes" />
    <param name="leg" value="a" />
    <param name="RatingInterval" value="300" />   <!-- idle poll seconds -->
    <param name="BatchLimit" value="10000" />      <!-- CDRs per cycle (1..50000) -->
    <param name="RatingThreads" value="4" />       <!-- DuckDB worker cap; 0 = all cores -->
    <param name="CacheDimensions" value="all" />   <!-- all | static | none -->
    <param name="BillingDay" value="01" />
    <param name="DayOfPayment" value="10" />
</Rating>
```

| Param | Meaning | Default |
|---|---|---|
| `active` | enable the module | `no` |
| `leg` | which leg to rate (`a`/`b`); empty = both | `a` |
| `RatingInterval` | seconds to sleep when the backlog is drained | 300 |
| `BatchLimit` | CDRs processed per cycle (clamped 1..50000) | 5000 |
| `RatingThreads` | cap DuckDB worker threads (server load control); `0` = all cores | 0 |
| `CacheDimensions` | dimension cache mode (see §5) | `all` |

Changing these needs only a **restart** (no recompile) — the module reads them at
startup.

---

## 4. Build & dependencies

DuckDB itself ships in the **`duckdb.so`** engine module; the ~70 MB SDK is
**fetched on demand**, never committed to git.

```sh
cd src/mod/db_duckdb   && make install          # builds duckdb.so (downloads SDK), dynamic lib
#   or:  make DUCKDB_LINK=static install         # self-contained, no runtime libduckdb.so
cd ../RatingDuckDB     && make install          # builds rt.so (no libduckdb link)
```
- Build/install **`db_duckdb` before `RatingDuckDB`** — `rt.so` binds the `duckdb`
  engine at runtime, so `duckdb.so` must be loaded first (it's declared as a
  module dependency).
- SDK version is pinned in `mod/db_duckdb/DUCKDB_VERSION`; fetched by
  `scripts/fetch_duckdb.sh`.
- `db_duckdb` must be in the runtime module-load list (and built in `config.md`).

See `make help` for the DuckDB build targets.

---

## 5. Tuning — what is optimal

The per-cycle time is roughly **`fixed_cost + ~k·BatchLimit`**. Two levers:

### `BatchLimit`
Bigger batches amortize the per-cycle fixed cost → higher throughput, but longer
per-cycle latency and more memory/write-spike per cycle.

| BatchLimit | observed throughput* |
|---|---|
| 5,000 | ~1.6K CDR/s |
| 10,000 | ~3.3K CDR/s |
| 50,000 | ~12–14K CDR/s |

\* on the test box; scales with hardware/data. The per-row ceiling is ~50K CDR/s.

**Optimal:** with `CacheDimensions=all`, the fixed cost is small, so a **moderate
`BatchLimit` (10k–20k)** gives high throughput *and* low latency. Very large
batches hit diminishing returns and bigger write/vacuum spikes — see "traps" below.

### `CacheDimensions` (memory vs speed)
The lookup tables are cached in DuckDB so they aren't re-pulled each cycle.

| Mode | What's cached | RAM | Speed |
|---|---|---|---|
| `all` (default) | every lookup table | highest | fastest |
| `static` | small config tables only (`rate`/`tariff`/`prefix`/`calc_function`/`bill_plan*`/`time_condition`/`free_billsec`); subscriber/account tables stay **live** | low | good |
| `none` | nothing (all live views over `pg.*`) | lowest | slowest |

The memory is dominated by the **account/subscriber tables** (`calling_number`,
`billing_account`, `pcard`, the `_deff`s). If RAM is tight, use `static`. Measure
with `ps -o rss -p <pid>`. The cache refreshes at startup, on each idle poll, and
every ~1000 cycles during a long drain (bounded staleness).

### `RatingThreads`
Caps DuckDB's worker pool (it otherwise uses all cores). Use it to leave CPU for
PostgreSQL/CallControl on a shared box. It rarely changes throughput much — the
cycle is usually bound by the serial write-back and PG I/O, not the DuckDB query.

---

## 6. `/Rating` vs `RatingDuckDB`

| | **Rating** (`/Rating`, classic) | **RatingDuckDB** |
|---|---|---|
| Model | per-CDR: 15–20 SQL queries each | per-cycle: a few analytical queries over N CDRs |
| Engine | libpq only | embedded DuckDB + libpq (writes) |
| Concurrency | C worker-thread pool (`RatingThreads`) | DuckDB internal parallelism (`RatingThreads` caps it) |
| Dimension data | queried per CDR | cached in DuckDB (configurable) |
| Throughput\* | ~870 CDR/s | ~1.6K–14K CDR/s (batch-dependent) |
| Stability | degraded over long runs (cache races, table bloat) | flat; no degradation |
| SMS pricing | mis-priced in 0.7.x (`calc_cprice_group`) | correct flat-fee |
| Writes | per-CDR `INSERT`/`UPDATE` | bulk per-cycle, in transactions |
| Crash safety | per-statement | rating + leg are one atomic tx (no duplicate rows) |
| Best for | low-latency trickle / online path | clearing large backlogs, high throughput |

\* test-box figures; indicative, not absolute.

---

## 7. Operational notes

- **Single instance:** the daemon refuses to start a second copy
  (`RateEngine is already running (PID …)`), via a pid-file `kill(pid,0)` check.
- **Transactions:** each cycle uses two transactions — (1) `INSERT rating` +
  `UPDATE cdrs.leg` (atomic → no duplicate rating rows on crash), and (2)
  `balance` + `free_billsec_balance` + the `-1` mark. Fewer fsyncs, crash-safe.
- **`/dev/shm`:** PostgreSQL parallel scans/maintenance need shared memory; the
  Docker default (64 MB) is too small (causes "could not resize shared memory
  segment"). `docker-compose.yml` sets `shm_size: 512m` on `re7-db`.
- **Re-rating:** to re-rate, reset `cdrs.leg` to 0 (and truncate/clear
  `rating`/`balance`/`free_billsec_balance` as needed); then restart.

---

## 8. Known limitations / traps

- **Pricing correctness depends on `calc_function.iterations`** — NULL on the
  final tier is treated as `0` (unbounded), matching `atoi`. (Fixed; was a
  0/0-pricing bug.)
- **Very large `BatchLimit`** has traps: O(n²) growth in the INSERT-string
  builder, bigger write transactions → larger WAL/checkpoint/autovacuum spikes,
  more DuckDB/PG memory, longer per-cycle latency. Keep it moderate.
- **Dimension-cache staleness:** rate/subscriber changes are visible only after a
  refresh (idle poll / ~1000 cycles). Restart for an immediate refresh.
- **`-1` marking churn:** each cycle updates the unmatched CDRs to `leg=-1`
  (~most of the window). On heavily-unregistered data this dominates PG writes
  and drives autovacuum stalls — a future "watermark" optimisation can avoid it.
- **Not yet ported:** count-based free-SMS; debit-card free pools.

---

## 9. Files

| File | Purpose |
|---|---|
| `rating_duckdb.c` | module wrapper: config, init, the rating loop, dimension refresh |
| `rt_duckdb.c` | the analytical cycle: account union, pricing CTE, free split, balance/free, writes |
| `rt_duckdb.h` | context struct + API + cache-mode constants |
| `lib/` | DuckDB SDK (fetched, gitignored) |
| `../db_duckdb/` | the `duckdb` db-engine module (`duckdb.so`) |
| `../../unit_tests/tariff_pricing.c` | parity test for the pricing/free-split math (`make check`) |
