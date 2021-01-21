<?php

function re6_import_settings($imp_file)
{
	$fp = fopen($imp_file,"r");
	if ($fp) {
		while(($buffer = fgets($fp, 4096)) !== false) {
			$row = trim($buffer,"\n");
			$buf = explode(",",$row);
			$config_mode = $buf[0];
	
			if($config_mode == 1) {
				/* 1. Bill Plan */
				$bill_plan = trim($buf[1],'"');
				$bill_plan_type = trim($buf[2],'"');
				$start_period = $buf[3];
				$end_period = $buf[4];
		
				if(empty($start_period)) $start_period = 0;
				if(empty($end_period)) $end_period = 0;
		
				if(empty($bill_plan_type)) $bill_plan_type = "postpaid";
				$bill_plan_type_id = get_bill_plan_type_id($bill_plan_type);
				if($bill_plan_type_id == 0) exit;
		
				// postpaid = 2 , prepaid = 1 !!!
		
				$bill_plan_id = get_bill_plan_id($bill_plan);
				if($bill_plan_id == 0) {
					insert_bill_plan($bill_plan,$bill_plan_type_id,$start_period,$end_period);
				
					$bill_plan_id = get_bill_plan_id($bill_plan);
					if($bill_plan_id == 0) {
						echo "A BillPlan ($bill_plan)is not inserted!!!\n";
						exit;
					}
				}
			}
	    
			if($config_mode == '2') {
				if(empty($bill_plan_id)) {
					echo "You haven't got a bill plan!\n";
					exit;
				}
			
				/* Prefixes and Tariffs */
				$prefix = $buf[1];
				$prefix_desc = trim($buf[2],'"');
				$tariff = trim($buf[3],'"');
//				$free_billsec = $buf[4];
				$free_billsec_id = $buf[4];
				$time_zone = trim($buf[5],'"');
				$start_period_tr = $buf[6];
//			$end_period_tr = $buf[7];
		
				if(empty($free_billsec_id)) $free_billsec_id = 0;
		
				check_2:
				$prefix_id = get_prefix_id($prefix);
				if($prefix_id == 0) {
					insert_prefix($prefix,$prefix_desc);
					goto check_2;
				}
		
				if(empty($start_period_tr)) $start_period_tr = 0;
				if(empty($end_period_tr)) $end_period_tr = 0;
		
				check_3:
				$tariff_id = get_tariff_id($bill_plan_id,$tariff);
				if($tariff_id == 0){
					insert_tariff($bill_plan_id,$tariff,$start_period_tr,$end_period_tr,$free_billsec_id);
					goto check_3;
				}
		
				check_4:
				$rate_id = get_rate_id($bill_plan_id,$prefix_id,$tariff_id);
				if(($rate_id == 0) and ($bill_plan_id) and ($prefix_id) and ($tariff_id)) {
					insert_rate($bill_plan_id,$prefix_id,$tariff_id);
					goto check_4;
				}
/*		
			$tr_id = get_free_id($tariff_id);
			if(($tr_id == 0) AND ($free_billsec)) {
				insert_free_billsec($free_billsec,$tariff_id);
			}
		
			check_4b:
			$time_condition_id = get_time_condition_id($tariff_id,$time_zone);
			echo "time_condition_id = ".$time_condition_id." ".$time_zone."\n";
			if(($time_condition_id == 0) AND (!(empty($time_zone))))
			{
				insert_time_condition($tariff_id,$time_zone);
				goto check_4b;
			}
*/		
				echo "bplan(".$bill_plan_id."),prefix(".$prefix_id."),tariff(".$tariff_id."),rate(".$rate_id.")\n";
			}
	    
			if($config_mode == '*') {
				$pos = $buf[1];
				$fee = $buf[2];
				$delta_time = $buf[3];
				$iterations = $buf[4];
		
				$calc_id = get_calc_id($tariff_id,$pos);
				if($calc_id) continue;
		
				if(($tariff_id)) insert_calc_func($tariff_id,$pos,$fee,$delta_time,$iterations);
			}
	    
			if($config_mode == "3") {
				$billing_account = trim($buf[1],'"');
				$leg = $buf[2];
				$curr_id = $buf[3];
				$rating_mode_id = $buf[4];
				$rating_account = trim($buf[5],'"');
				$cdr_server_id = $buf[6];
				$clg_nadi = $buf[7];
				$cld_nadi = $buf[8];
		
				if($rating_mode_id == 0) continue;
		
				$rating_mode = get_rating_mode($rating_mode_id);
		
				if(empty($rating_mode)) continue;
		
				check_5:
				$billing_account_id = get_billing_account_id($billing_account);
				if($billing_account_id == 0) {
					insert_billing_account($billing_account,$curr_id,$leg,$cdr_server_id);
					goto check_5;
				}
		
				check_6:
				$rating_account_id = get_rating_account_id($rating_mode,$rating_account);
				if($rating_account_id == 0) {
					if($rating_mode <= 4) insert_rating_account($rating_mode,$rating_account,$billing_account_id,$bill_plan_id);
					if($rating_mode >= 5) insert_rating_account_2($rating_mode,$rating_account,$billing_account_id,$bill_plan_id,$clg_nadi,$cld_nadi);
					goto check_6;
				}
			}
	
			if($config_mode == "4") {
				$pcard_type = trim($buf[1],'"');
				$pcard_status = trim($buf[2],'"');
				$amount = $buf[3];
				$start_date = trim($buf[4],'"');
				$end_date = trim($buf[5],'"');
		
				$pcard_status_id  = 0;
				$pcard_type_id = 0;
		
				if($billing_account_id) {
					if(strcmp($pcard_status,"")) $pcard_status_id = get_pcard_status_id($pcard_status);
		
					if(strcmp($pcard_type,"")) {
						$pcard_type_id = get_pcard_type_id($pcard_type);
					} else $pcard_type_id = 2;
		
					if($pcard_type_id) {
						if((get_pcard_id($billing_account_id,$pcard_type_id,$pcard_status_id,$amount,$start_date,$end_date)) == 0) { 
							insert_pcard($billing_account_id,$pcard_type_id,$pcard_status_id,$amount,$start_date,$end_date);
						}
					}
				}
			}
	
			if($config_mode == "5") {
				$root_id = 0;

				$p = 1;
				while(!empty($buf[$p])) {
					$bplan_tree = trim($buf[$p],'"');
					$tree_id = get_bill_plan_id($bplan_tree);	
				
					if($p == 1) {
						/* root */
						if($tree_id > 0) $root_id = $tree_id;
						else $root_id = insert_bill_plan_v2($bplan_tree);

						$p++;
						continue;
					}
				
					if(($tree_id > 0)AND($root_id > 0))
						if((get_bill_tree_id($tree_id,$root_id)) == 0) insert_bill_tree($tree_id,$root_id);
				
					$p++;
				}
			}
		}
	}
 
	fclose($fp);
}
?>
