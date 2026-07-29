-- ---------------------------------------------------------------------------
-- Synthetic rating fixture for the offline-rating golden regression.
--
-- ALL DATA HERE IS INVENTED for the test - it is NOT customer data. It builds
-- the minimal calling_number -> ... -> tariff chain the Rating engine (rt.so)
-- needs to price a leg-A voice CDR, across three tariff shapes:
--   A) flat per-second   (delta=1,  fee, iterations=0)
--   B) per-minute block  (delta=60, fee, iterations=0)
--   C) two-tier          (delta=60, fee1, iterations=1) + (delta=60, fee2, 0)
--
-- Load AFTER rt_pgsql.sql (schema + generic lookups). The CDRs live in
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
--   * all *_period = 0 and tariff.free_billsec_id = 0 -> no period/free-billsec
--     gating, no free-billsec price split.
-- ---------------------------------------------------------------------------

BEGIN;

-- One postpaid subscriber account on cdr_server_id = 1.
INSERT INTO billing_account (id, username, currency_id, leg, cdr_server_id,
                             billing_day, round_mode_id, day_of_payment)
    VALUES (1, 'e2e_sub', 1, 'a', 1, '01', 0, 0);

-- Mandatory active pcard (credit card, wide validity window covering any CDR).
INSERT INTO pcard (id, amount, start_date, end_date, last_update,
                   pcard_status_id, billing_account_id, pcard_type_id,
                   call_number, saved_amount, sim)
    VALUES (1, 1000000, '2000-01-01', '2100-01-01', now(),
            1 /*active*/, 1, 2 /*credit*/, 1, 0, 0);

-- Catch-all prefix and a shared time_condition_deff (mon-sun, all hours).
INSERT INTO prefix (id, prefix, comm) VALUES (1, '359', 'e2e catch prefix');
-- hours is varchar(11) -> HH:MM-HH:MM (a full day); days_week varchar(7) -> mon-sun.
INSERT INTO time_condition_deff (id, hours, days_week, tc_name, tc_date, year, month, day_month)
    VALUES (1, '00:00-23:59', 'mon-sun', 'allweek', '', '', '', '');

-- Bill plans (one per tariff shape; postpaid; no periods).
INSERT INTO bill_plan (id, name, bill_plan_type_id, start_period, end_period) VALUES
    (1, 'bp_persec', 2, 0, 0),
    (2, 'bp_permin', 2, 0, 0),
    (3, 'bp_tier',   2, 0, 0);

-- Tariffs (no periods, no free-billsec).
INSERT INTO tariff (id, name, temp_id, start_period, end_period, free_billsec_id) VALUES
    (1, 'tf_persec', 0, 0, 0, 0),
    (2, 'tf_permin', 0, 0, 0, 0),
    (3, 'tf_tier',   0, 0, 0, 0);

-- calc_function steps (the price formula inputs).
--   A) flat per-second:  0.02 / second.
--   B) per-minute block: 0.10 / 60s block (rounded up).
--   C) two-tier:         first 60s block 0.30, then 0.10 / 60s block.
INSERT INTO calc_function (id, tariff_id, pos, delta_time, fee, iterations) VALUES
    (1, 1, 1,  1, 0.02, 0),
    (2, 2, 1, 60, 0.10, 0),
    (3, 3, 1, 60, 0.30, 1),
    (4, 3, 2, 60, 0.10, 0);

-- One time_condition per tariff, all pointing at the mon-sun/all-hours deff.
INSERT INTO time_condition (id, tariff_id, time_condition_id, prior) VALUES
    (1, 1, 1, 40),
    (2, 2, 1, 40),
    (3, 3, 1, 40);

-- Rates: bill_plan -> (prefix, tariff). One rate per plan.
INSERT INTO rate (id, bill_plan_id, tariff_id, prefix_id) VALUES
    (1, 1, 1, 1),
    (2, 2, 2, 1),
    (3, 3, 3, 1);

-- Calling numbers (invented) -> account, and their bill-plan assignment.
INSERT INTO calling_number (id, calling_number, billing_account_id) VALUES
    (1, '359881000001', 1),
    (2, '359881000002', 1),
    (3, '359881000003', 1);

INSERT INTO calling_number_deff (id, calling_number_id, bill_plan_id, sm_bill_plan_id) VALUES
    (1, 1, 1, 0),
    (2, 2, 2, 0),
    (3, 3, 3, 0);

COMMIT;
