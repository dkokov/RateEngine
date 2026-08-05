-- ---------------------------------------------------------------------------
-- Synthetic benchmark dataset for offline rating (NO customer data).
--
-- Load AFTER rt_pgsql_v2.sql (schema + generic lookups) into a throwaway/bench DB.
-- Populates the full rating chain at scale so rt.so / rt_duckdb.so can be
-- benchmarked and parity-checked WITHOUT ever touching the real database.
--
--   5 tariff plans   : bill_plan 1..5, tariff 1..5 (varied calc_function),
--                      10 destination prefixes, rate[plan x prefix] -> tariff=plan
--   N_ACCOUNTS        : billing_account + calling_number (src) + calling_number_deff
--                      (round-robin to a plan) + active credit pcard   (default 50000)
--   N_CDRS            : unrated leg-A voip CDRs, random src (a calling_number) and
--                      dst (a prefix + 8 digits), weekday timestamps  (default 1e6)
--
-- Every CDR rates: its calling_number exists, its called_number starts with a
-- prefix, and every plan has a rate for every prefix (src/dst independent).
-- Timestamps are weekdays (Mon-Sat) so they satisfy the mon-sun time-condition;
-- pcards have wide validity so the pcard gate always passes. free_billsec_id=0
-- (no free-billsec split) to keep the base benchmark deterministic.
--
-- Scale is overridable:  psql -v n_accounts=50000 -v n_cdrs=1000000 -f gen_bench_db.sql
-- ---------------------------------------------------------------------------

\if :{?n_accounts}
\else
  \set n_accounts 50000
\endif
\if :{?n_cdrs}
\else
  \set n_cdrs 1000000
\endif

\echo Generating synthetic bench data: :n_accounts accounts, :n_cdrs CDRs

-- rt_pgsql_v2.sql ships db_screenshot change-tracking RULES on `rate` (DELETE+
-- INSERT of the table name) that collide with the multi-row rate insert below
-- (UNIQUE tbl_name). Not needed for rating; drop them in this bench DB.
DO $$ DECLARE r record; BEGIN
  FOR r IN SELECT tablename, rulename FROM pg_rules WHERE rulename LIKE 'db_screenshot%'
  LOOP EXECUTE format('DROP RULE %I ON public.%I', r.rulename, r.tablename); END LOOP;
END $$;

BEGIN;

-- ---- 5 tariff plans ----------------------------------------------------
INSERT INTO bill_plan (id, name, bill_plan_type_id, start_period, end_period)
SELECT g, 'bench_bp_'||g, 2, 0, 0 FROM generate_series(1,5) g;

INSERT INTO tariff (id, name, temp_id, start_period, end_period, free_billsec_id)
SELECT g, 'bench_tf_'||g, 0, 0, 0, 0 FROM generate_series(1,5) g;

-- Varied pricing: per-second, per-minute, two-tier, per-second, 30s blocks.
INSERT INTO calc_function (tariff_id, pos, delta_time, fee, iterations) VALUES
    (1, 1,  1, 0.010, 0),
    (2, 1, 60, 0.050, 0),
    (3, 1, 60, 0.100, 1),
    (3, 2, 60, 0.050, 0),
    (4, 1,  1, 0.020, 0),
    (5, 1, 30, 0.030, 0);

-- 10 destination prefixes (mutually non-prefixing).
INSERT INTO prefix (id, prefix, comm) VALUES
    (1,'20','bench'),(2,'27','bench'),(3,'33','bench'),(4,'44','bench'),(5,'49','bench'),
    (6,'86','bench'),(7,'90','bench'),(8,'91','bench'),(9,'359','bench'),(10,'380','bench');

-- One shared time-condition (mon-sun, all hours), one per tariff.
INSERT INTO time_condition_deff (id, hours, days_week, tc_name, tc_date, year, month, day_month)
    VALUES (1, '00:00-23:59', 'mon-sun', 'allweek', '', '', '', '');
INSERT INTO time_condition (id, tariff_id, time_condition_id, prior)
SELECT g, g, 1, 40 FROM generate_series(1,5) g;

-- rate[plan x prefix] -> plan's tariff. Every plan prices every prefix, so any
-- (account, destination) pair rates.
INSERT INTO rate (bill_plan_id, tariff_id, prefix_id)
SELECT bp, bp, pr FROM generate_series(1,5) bp CROSS JOIN generate_series(1,10) pr;

-- ---- N_ACCOUNTS subscribers -------------------------------------------
INSERT INTO billing_account (id, username, currency_id, leg, cdr_server_id, billing_day, round_mode_id, day_of_payment)
SELECT g, 'bench_sub_'||g, 1, 'a', 1, '01', 0, 0 FROM generate_series(1,:n_accounts) g;

-- calling_number = the subscriber's source number (src), '7000xxxxx'.
INSERT INTO calling_number (id, calling_number, billing_account_id)
SELECT g, (700000000 + g)::text, g FROM generate_series(1,:n_accounts) g;

-- assign each subscriber round-robin to one of the 5 plans.
INSERT INTO calling_number_deff (id, calling_number_id, bill_plan_id, sm_bill_plan_id)
SELECT g, g, (g % 5) + 1, 0 FROM generate_series(1,:n_accounts) g;

-- one active credit pcard per account, wide validity.
INSERT INTO pcard (id, amount, start_date, end_date, last_update, pcard_status_id, billing_account_id, pcard_type_id, call_number, saved_amount, sim)
SELECT g, 1000000, '2000-01-01', '2100-01-01', now(), 1, g, 2, 1, 0, 0 FROM generate_series(1,:n_accounts) g;

COMMIT;

-- ---- N_CDRS calls ------------------------------------------------------
-- src  = a random existing calling_number (700000001 .. 700000000+N_ACCOUNTS)
-- dst  = a random prefix from the prefix table + 8 random digits
-- when = a random weekday (Mon-Sat) in 2026-06, random second of day
--
-- leg_a = 1 marks the CDRs as "rated" so this DB is a stand-in for a populated
-- production DB: the replay bench/parity tools (bench_rating_replay.sh,
-- parity_real.sh) select the newest N with `WHERE leg_a > 0`, then DELETE their
-- rating rows and reset leg_a=0 before the timed re-rate - so the marker value
-- is never dereferenced (there are no real rating rows), it only needs to be >0.
-- To rate the whole DB from scratch instead, first: UPDATE cdrs SET leg_a = 0;
WITH p AS (
    SELECT array_agg(prefix ORDER BY id) AS arr, count(*)::int AS n FROM prefix
), wd AS (
    SELECT array_agg(d::date ORDER BY d) AS days, count(*)::int AS n
    FROM generate_series('2026-06-01'::timestamp, '2026-06-30'::timestamp, '1 day') d
    WHERE extract(dow FROM d) <> 0          -- exclude Sunday (dow 0) for mon-sun tc
)
INSERT INTO cdrs (cdr_server_id, cdr_rec_type_id, call_uid, leg_a, leg_b, start_ts,
                  calling_number, called_number, billsec, duration)
SELECT
    1, 3, 'bench-'||g, 1, 0,          -- leg_a=1 -> "rated" marker (see note above)
    wd.days[(g % wd.n) + 1]::timestamp + make_interval(secs => (random()*86399)::int),
    (700000000 + (floor(random()*:n_accounts)::int) + 1)::text,
    p.arr[floor(random()*p.n)::int + 1] || lpad(floor(random()*100000000)::bigint::text, 8, '0'),
    floor(random()*3600)::int + 1,
    floor(random()*3600)::int + 1
FROM generate_series(1,:n_cdrs) g CROSS JOIN p CROSS JOIN wd;

\echo Done. Row counts:
SELECT 'billing_account' AS tbl, count(*) FROM billing_account
UNION ALL SELECT 'calling_number', count(*) FROM calling_number
UNION ALL SELECT 'pcard', count(*) FROM pcard
UNION ALL SELECT 'bill_plan', count(*) FROM bill_plan
UNION ALL SELECT 'tariff', count(*) FROM tariff
UNION ALL SELECT 'rate', count(*) FROM rate
UNION ALL SELECT 'prefix', count(*) FROM prefix
UNION ALL SELECT 'cdrs (marked rated)', count(*) FROM cdrs WHERE leg_a > 0
ORDER BY tbl;
