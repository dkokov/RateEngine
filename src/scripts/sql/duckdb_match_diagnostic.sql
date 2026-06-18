-- RatingDuckDB match diagnostic
-- Counts how many DISTINCT unrated CDRs survive each join stage.
-- Run against the PostgreSQL rating DB. Watch where "cdrs" drops to 0.
--
-- Pairs to compare:
--   5a (rate DIRECT, what DuckDB does now)  vs  5b (rate via bill_plan_tree, what /Rating does)
--   8  (calc_function all pos)              vs  8b (calc_function pos=1, what DuckDB does now)
--
-- NOTE: assumes leg 'a' / calling_number lookup (the common rec type).
--       SMS/transit rec types use sm_bill_plan_id / src tables and aren't covered here.

WITH c AS (
    SELECT id, calling_number, called_number, cdr_server_id
    FROM cdrs
    WHERE leg_a = 0
    ORDER BY id
    LIMIT 5000
)
SELECT '0. unrated sample'                          AS stage, count(DISTINCT c.id) AS cdrs FROM c
UNION ALL
SELECT '1. +calling_number', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg ON clg.calling_number = c.calling_number
UNION ALL
SELECT '2. +calling_number_deff', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg      ON clg.calling_number = c.calling_number
  JOIN calling_number_deff cd  ON cd.calling_number_id = clg.id
UNION ALL
SELECT '3a. +billing_account (NO server filter)', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg      ON clg.calling_number = c.calling_number
  JOIN calling_number_deff cd  ON cd.calling_number_id = clg.id
  JOIN billing_account ba      ON ba.id = clg.billing_account_id
UNION ALL
SELECT '3b. +billing_account (cdr_server_id match)', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg      ON clg.calling_number = c.calling_number
  JOIN calling_number_deff cd  ON cd.calling_number_id = clg.id
  JOIN billing_account ba      ON ba.id = clg.billing_account_id
                              AND ba.cdr_server_id = c.cdr_server_id
UNION ALL
SELECT '4. +bill_plan (root)', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg      ON clg.calling_number = c.calling_number
  JOIN calling_number_deff cd  ON cd.calling_number_id = clg.id
  JOIN billing_account ba      ON ba.id = clg.billing_account_id
                              AND ba.cdr_server_id = c.cdr_server_id
  JOIN bill_plan bp            ON bp.id = cd.bill_plan_id
UNION ALL
SELECT '5a. +rate DIRECT  (current DuckDB)', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg      ON clg.calling_number = c.calling_number
  JOIN calling_number_deff cd  ON cd.calling_number_id = clg.id
  JOIN billing_account ba      ON ba.id = clg.billing_account_id
                              AND ba.cdr_server_id = c.cdr_server_id
  JOIN bill_plan bp            ON bp.id = cd.bill_plan_id
  JOIN rate rt                 ON rt.bill_plan_id = bp.id
UNION ALL
SELECT '5b. +rate via bill_plan_tree  (/Rating)', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg      ON clg.calling_number = c.calling_number
  JOIN calling_number_deff cd  ON cd.calling_number_id = clg.id
  JOIN billing_account ba      ON ba.id = clg.billing_account_id
                              AND ba.cdr_server_id = c.cdr_server_id
  JOIN bill_plan bp            ON bp.id = cd.bill_plan_id
  JOIN bill_plan_tree tree     ON tree.root_bplan_id = bp.id
  JOIN rate rt                 ON rt.bill_plan_id = tree.bill_plan_id
UNION ALL
SELECT '6. +prefix match (via tree)', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg      ON clg.calling_number = c.calling_number
  JOIN calling_number_deff cd  ON cd.calling_number_id = clg.id
  JOIN billing_account ba      ON ba.id = clg.billing_account_id
                              AND ba.cdr_server_id = c.cdr_server_id
  JOIN bill_plan bp            ON bp.id = cd.bill_plan_id
  JOIN bill_plan_tree tree     ON tree.root_bplan_id = bp.id
  JOIN rate rt                 ON rt.bill_plan_id = tree.bill_plan_id
  JOIN prefix pr               ON pr.id = rt.prefix_id
                              AND c.called_number LIKE pr.prefix || '%'
UNION ALL
SELECT '7. +tariff (via tree)', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg      ON clg.calling_number = c.calling_number
  JOIN calling_number_deff cd  ON cd.calling_number_id = clg.id
  JOIN billing_account ba      ON ba.id = clg.billing_account_id
                              AND ba.cdr_server_id = c.cdr_server_id
  JOIN bill_plan bp            ON bp.id = cd.bill_plan_id
  JOIN bill_plan_tree tree     ON tree.root_bplan_id = bp.id
  JOIN rate rt                 ON rt.bill_plan_id = tree.bill_plan_id
  JOIN prefix pr               ON pr.id = rt.prefix_id
                              AND c.called_number LIKE pr.prefix || '%'
  JOIN tariff tr               ON tr.id = rt.tariff_id
UNION ALL
SELECT '8. +calc_function ALL pos (via tree)', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg      ON clg.calling_number = c.calling_number
  JOIN calling_number_deff cd  ON cd.calling_number_id = clg.id
  JOIN billing_account ba      ON ba.id = clg.billing_account_id
                              AND ba.cdr_server_id = c.cdr_server_id
  JOIN bill_plan bp            ON bp.id = cd.bill_plan_id
  JOIN bill_plan_tree tree     ON tree.root_bplan_id = bp.id
  JOIN rate rt                 ON rt.bill_plan_id = tree.bill_plan_id
  JOIN prefix pr               ON pr.id = rt.prefix_id
                              AND c.called_number LIKE pr.prefix || '%'
  JOIN tariff tr               ON tr.id = rt.tariff_id
  JOIN calc_function cf        ON cf.tariff_id = rt.tariff_id
UNION ALL
SELECT '8b. +calc_function pos=1 (current DuckDB)', count(DISTINCT c.id)
  FROM c
  JOIN calling_number clg      ON clg.calling_number = c.calling_number
  JOIN calling_number_deff cd  ON cd.calling_number_id = clg.id
  JOIN billing_account ba      ON ba.id = clg.billing_account_id
                              AND ba.cdr_server_id = c.cdr_server_id
  JOIN bill_plan bp            ON bp.id = cd.bill_plan_id
  JOIN bill_plan_tree tree     ON tree.root_bplan_id = bp.id
  JOIN rate rt                 ON rt.bill_plan_id = tree.bill_plan_id
  JOIN prefix pr               ON pr.id = rt.prefix_id
                              AND c.called_number LIKE pr.prefix || '%'
  JOIN tariff tr               ON tr.id = rt.tariff_id
  JOIN calc_function cf        ON cf.tariff_id = rt.tariff_id AND cf.pos = 1
ORDER BY stage;
