-- ---------------------------------------------------------------------------
-- Synthetic rating fixture for the offline-rating golden regression.
--
-- ALL DATA HERE IS INVENTED for the test - it is NOT customer data. It builds
-- the minimal calling_number -> ... -> tariff chain the Rating engine (rt.so)
-- needs to price a leg-A voice CDR, across four tariff shapes:
--   A) flat per-second   (delta=1,  fee, iterations=0)
--   B) per-minute block  (delta=60, fee, iterations=0)
--   C) two-tier          (delta=60, fee1, iterations=1) + (delta=60, fee2, 0)
--   D) per-second WITH a free_billsec allowance -> exercises append_free_billsec
--      and rt_double_rating (the free/paid split at the allowance boundary)
--
-- Load AFTER rt_pgsql_v2.sql (schema + generic lookups). The CDRs live in
-- cdrs_seed.sql (loaded separately so the runner can inject a weekday
-- timestamp). Expected prices are pinned in golden.tsv.
--
-- Chain requirements baked in (see the rating source):
--   * billing_account.cdr_server_id MUST equal cdrs.cdr_server_id (=1).
--   * an active pcard is MANDATORY (the rating insert path dereferences the
--     pcard pointer; no pcard -> NULL deref). Credit card, wide validity.
--   * a matching time_condition(+_deff) is provided so the tariff survives the
--     time-condition gate deterministically (mon-sun, all hours).
--   * NO bill_plan_tree rows -> the simple (non-tree) rate query is used.
--   * all *_period = 0 -> no period gating.
--   * tariffs A-C have free_billsec_id = 0 (no allowance, no price split);
--     tariff D has one, on its OWN account so its allowance is isolated.
--
-- Why tariff D exists: a call covered by the free allowance is stored with a
-- NEGATIVE call_price - that negative is the "was free" marker, not a credit.
-- Without a free-billsec chain here, nothing in CI ever produced a negative
-- price, so bugs in what happens downstream of it (balance charged the negative;
-- free_billsec_balance keyed to balance_id = 0) could not be caught. The golden
-- prices only pin per-CDR pricing; the balance/ledger consequences are asserted
-- by check_rating_invariants.sh, which the runner invokes after each engine.
-- ---------------------------------------------------------------------------

BEGIN;

-- Two postpaid subscriber accounts on cdr_server_id = 1. Account 2 carries the
-- free-billsec tariff alone, so its allowance can be reasoned about exactly.
INSERT INTO billing_account (id, username, currency_id, leg, cdr_server_id,
                             billing_day, round_mode_id, day_of_payment)
    VALUES (1, 'e2e_sub',      1, 'a', 1, '01', 0, 0),
           (2, 'e2e_free_sub', 1, 'a', 1, '01', 0, 0);

-- Mandatory active pcard per account (credit card, wide validity window).
INSERT INTO pcard (id, amount, start_date, end_date, last_update,
                   pcard_status_id, billing_account_id, pcard_type_id,
                   call_number, saved_amount, sim)
    VALUES (1, 1000000, '2000-01-01', '2100-01-01', now(),
            1 /*active*/, 1, 2 /*credit*/, 1, 0, 0),
           (2, 1000000, '2000-01-01', '2100-01-01', now(),
            1 /*active*/, 2, 2 /*credit*/, 1, 0, 0);

-- Catch-all prefix and a shared time_condition_deff (mon-sun, all hours).
INSERT INTO prefix (id, prefix, comm) VALUES (1, '359', 'e2e catch prefix');
-- hours is varchar(11) -> HH:MM-HH:MM (a full day); days_week varchar(7) -> mon-sun.
INSERT INTO time_condition_deff (id, hours, days_week, tc_name, tc_date, year, month, day_month)
    VALUES (1, '00:00-23:59', 'mon-sun', 'allweek', '', '', '', '');

-- Bill plans (one per tariff shape; postpaid; no periods).
INSERT INTO bill_plan (id, name, bill_plan_type_id, start_period, end_period) VALUES
    (1, 'bp_persec', 2, 0, 0),
    (2, 'bp_permin', 2, 0, 0),
    (3, 'bp_tier',   2, 0, 0),
    (4, 'bp_free',   2, 0, 0);

-- Free-billsec allowance for tariff D: 100 free seconds per balance period.
-- (ids 1-4 are seeded by rt_pgsql_v2.sql; use a distinct id.)
INSERT INTO free_billsec (id, free_billsec) VALUES (10, 100);

-- Tariffs. A-C have no allowance; D points at free_billsec 10.
INSERT INTO tariff (id, name, temp_id, start_period, end_period, free_billsec_id) VALUES
    (1, 'tf_persec', 0, 0, 0, 0),
    (2, 'tf_permin', 0, 0, 0, 0),
    (3, 'tf_tier',   0, 0, 0, 0),
    (4, 'tf_free',   0, 0, 0, 10);

-- calc_function steps (the price formula inputs).
--   A) flat per-second:  0.02 / second.
--   B) per-minute block: 0.10 / 60s block (rounded up).
--   C) two-tier:         first 60s block 0.30, then 0.10 / 60s block.
--   D) flat per-second:  0.02 / second (same shape as A, so the free/paid split
--                        arithmetic stays trivial to verify by hand).
INSERT INTO calc_function (id, tariff_id, pos, delta_time, fee, iterations) VALUES
    (1, 1, 1,  1, 0.02, 0),
    (2, 2, 1, 60, 0.10, 0),
    (3, 3, 1, 60, 0.30, 1),
    (4, 3, 2, 60, 0.10, 0),
    (5, 4, 1,  1, 0.02, 0);

-- One time_condition per tariff, all pointing at the mon-sun/all-hours deff.
INSERT INTO time_condition (id, tariff_id, time_condition_id, prior) VALUES
    (1, 1, 1, 40),
    (2, 2, 1, 40),
    (3, 3, 1, 40),
    (4, 4, 1, 40);

-- Rates: bill_plan -> (prefix, tariff). One rate per plan.
INSERT INTO rate (id, bill_plan_id, tariff_id, prefix_id) VALUES
    (1, 1, 1, 1),
    (2, 2, 2, 1),
    (3, 3, 3, 1),
    (4, 4, 4, 1);

-- Calling numbers (invented) -> account, and their bill-plan assignment.
-- Number 4 belongs to account 2 and is the only user of the free allowance.
INSERT INTO calling_number (id, calling_number, billing_account_id) VALUES
    (1, '359881000001', 1),
    (2, '359881000002', 1),
    (3, '359881000003', 1),
    (4, '359881000004', 2);

INSERT INTO calling_number_deff (id, calling_number_id, bill_plan_id, sm_bill_plan_id) VALUES
    (1, 1, 1, 0),
    (2, 2, 2, 0),
    (3, 3, 3, 0),
    (4, 4, 4, 0);

COMMIT;
