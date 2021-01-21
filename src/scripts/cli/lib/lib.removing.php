<?php

$sql = "select id from bill_plan where name = 'MobilePromo1'";
root_bill_plan_id

$sql = "select bill_plan_id  from bill_plan_tree where root_bplan_id = 118";
bplans_id

$sql = "select tariff_id from rate where bill_plan_id = 116";
tariff_id

$sql = "select free_billsec_id from tariff where id = 418";
free_billsec_id

rate_engine=> delete from rate where bill_plan_id = 126;
DELETE 56
rate_engine=> delete from rate where bill_plan_id = 127;
DELETE 1
rate_engine=> 
rate_engine=> 
rate_engine=> 
rate_engine=> select * from tariff where id = 426;
 id  |      name      | temp_id | start_period | end_period | free_billsec_id 
-----+----------------+---------+--------------+------------+-----------------
 426 | зона-1(promo1) |     126 |            0 |          0 |               0
(1 row)

rate_engine=> delete from tariff where id = 426;
DELETE 1
rate_engine=> delete from tariff where id = 427;
DELETE 1
rate_engine=> 
rate_engine=> 
rate_engine=> select * from calc_function where tariff_id = 426;
 id  | tariff_id | pos | delta_time |   fee   | iterations 
-----+-----------+-----+------------+---------+------------
 462 |       426 |   2 |          1 | 0.00125 |           
 461 |       426 |   1 |         30 |  0.0375 |          1
(2 rows)


delete from calc_function where tariff_id = 426;

delete from calc_function where tariff_id = 427;

delete from bill_plan_tree where root_bplan_id = 127;

?>
