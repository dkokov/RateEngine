-- ---------------------------------------------------------------------------
-- Known, unrated leg-A voice CDRs for the golden regression (synthetic data).
--
-- The runner passes :call_ts (a mid-month weekday timestamp) so the calls land
-- on a weekday (time-condition match) inside the current billing period (pcard
-- validity), keeping the test date-independent. Prices depend only on billsec
-- and the tariff, not on the date.
--
-- Fields that matter for rating (see fixture.sql / rating source):
--   cdr_server_id=1        -> matches billing_account.cdr_server_id
--   cdr_rec_type_id=3      -> voip_a (calling_number leg-A path)
--   leg_a=0                -> unrated (picked up by cdr_get_cdrs)
--   calling_number         -> exact match to a calling_number row
--   called_number '359...' -> matches prefix '359'
--   billsec                -> the price driver
-- ---------------------------------------------------------------------------

INSERT INTO cdrs
    (cdr_server_id, cdr_rec_type_id, call_uid, leg_a, leg_b, start_ts,
     start_epoch, calling_number, called_number, clg_nadi, cld_nadi,
     billsec, duration, billusec)
VALUES
    -- A) per-second: billsec 125 -> price 125 * 0.02 = 2.50, billed 125
    (1, 3, 'gold-persec', 0, 0, :'call_ts', 0, '359881000001', '359881999001', 0, 0, 125, 125, 0),
    -- B) per-minute: billsec 90 -> ceil(90/60)=2 blocks -> 2*0.10 = 0.20, billed 120
    (1, 3, 'gold-permin', 0, 0, :'call_ts', 0, '359881000002', '359881999002', 0, 0,  90,  90, 0),
    -- C) two-tier: billsec 150 -> 0.30 + ceil(90/60)*0.10 = 0.50, billed 180
    (1, 3, 'gold-tier',   0, 0, :'call_ts', 0, '359881000003', '359881999003', 0, 0, 150, 150, 0),
    -- ---------------------------------------------------------------------
    -- D) free_billsec (allowance 100s, 0.02/s, account 2). ORDER MATTERS: these
    --    two are rated by ascending cdrs.id, and the second one's split depends
    --    on the first having consumed 60s. The regression runs single-threaded
    --    (RatingThreads unset -> RT_DEFAULT_THREADS = 1), so this is stable.
    --
    -- D1) 60s, fully inside the remaining 100s -> wholly free. append_free_billsec
    --     negates the price as the "was free" marker: -(60*0.02) = -1.20, billed 60.
    (1, 3, 'gold-free-under', 0, 0, :'call_ts', 0, '359881000004', '359881999004', 0, 0, 60, 60, 0),
    -- D2) 90s with only 40s of allowance left -> rt_double_rating splits it:
    --       phase 1 free 40s -> -(40*0.02) = -0.80
    --       phase 2 paid 50s -> +(50*0.02) = +1.00
    --     Two rating rows for one CDR; the harness compares the SUM per call_uid,
    --     so price = 0.20 and billsec = 90. Consumption lands on exactly 100 =
    --     the allowance, which is the boundary invariant 5 checks for.
    (1, 3, 'gold-free-split', 0, 0, :'call_ts', 0, '359881000004', '359881999004', 0, 0, 90, 90, 0);
