<?php

function re6_import_bplan($buf)
{
	if(!empty($buf[1])) $bill_plan  = trim($buf[1],'"');
	else return 0;
	
	if(!empty($buf[2])) {
		$start_date = trim($buf[2],'"');
		
		if(!empty($start_date)) $start_period = strtotime($start_date);
		else $start_period = 0;
	} else {
		$start_period = 0;
	}
	
	if(!empty($buf[3])) {
		$end_date   = trim($buf[3],'"');
		
		if(!empty($end_date)) $end_period = strtotime($end_date);
		else $end_period = 0;
	} else {
		$end_period = 0;
	}
			
	$bill_plan_id = get_bill_plan_id($bill_plan);
	if($bill_plan_id == 0) {
		insert_bill_plan($bill_plan,$start_period,$end_period);
	
		$bill_plan_id = get_bill_plan_id($bill_plan);
		if($bill_plan_id == 0) {
			echo "A BillPlan '$bill_plan' is not inserted!!!\n";
		}
	}
	
	return $bill_plan_id;
}

function re6_import_rate($bill_plan_id,$buf)
{
	$prefix = $buf[1];
	$prefix_desc = trim($buf[2],'"');
	$tariff = trim($buf[3],'"');
	$free_billsec = $buf[4];
	$time_zone  = trim($buf[5],'"');
	
	if(!empty($buf[6])) $start_date = trim($buf[6],'"');
	else $start_date = NULL;
	
	if(!empty($buf[7])) $end_date   = trim($buf[7],'"');
	else $end_date = NULL;
	
	if($bill_plan_id == 0) return 0;
	
	if(empty($free_billsec)) $free_billsec_id = 0;
	else {
		$free_billsec_id = get_free_billsec_id($free_billsec);
		
		if($free_billsec_id == 0) $free_billsec_id = insert_free_billsec($free_billsec);
	}
	
	$prefix_id = get_prefix_id($prefix);
	if($prefix_id == 0) {
		$prefix_id = insert_prefix($prefix,$prefix_desc);
	}
		
	if(empty($start_date)) $start_period_tr = 0;
	else $start_period_tr = strtotime($start_date);
	
	if(empty($end_date)) $end_period_tr = 0;
	else $end_period_tr = strtotime($end_date);
	
	$tariff_id = get_tariff_id($bill_plan_id,$tariff);
	if($tariff_id == 0){
		$tariff_id = insert_tariff($bill_plan_id,$tariff,$start_period_tr,$end_period_tr,$free_billsec_id);
	}
		
	$rate_id = get_rate_id($bill_plan_id,$prefix_id,$tariff_id);
	if(($rate_id == 0) and ($bill_plan_id) and ($prefix_id) and ($tariff_id)) {
		$rate_id = insert_rate($bill_plan_id,$prefix_id,$tariff_id);
	}
	
	echo "bplan(".$bill_plan_id."),prefix(".$prefix_id."),tariff(".$tariff_id."),rate(".$rate_id.")\n";	
	
	return $tariff_id;
}

function re6_import_racc($bill_plan_id,$buf)
{
	$billing_account = trim($buf[1],'"');
	$leg = $buf[2];
	$curr_id = $buf[3];
	$rating_mode    = trim($buf[4],'"');
	$rating_account = trim($buf[5],'"');
	
	if(!empty($buf[6])) $cdr_server_id = $buf[6];
	else $cdr_server_id = 0;
	
	if(!empty($buf[7])) $clg_nadi = $buf[7];
	else $clg_nadi = 0;
	
	if(!empty($buf[8])) $cld_nadi = $buf[8];
	else $cld_nadi = 0;
	
	if($bill_plan_id == 0) {
		echo "Error! A 'BillPlan' is empty!\n";
		return 0;
	}
			
	$rating_mode_id = get_rating_mode_id($rating_mode);
	if($rating_mode_id == 0) {
		echo "Invalid 'rating_mode' ... $rating_mode \n";
		return 0;
	}

	if(empty($rating_account)) {
		echo "Error! A 'rating_account' is empty!\n";
		return 0;
	}

	if(empty($billing_account)) {
		echo "Error! A 'billing_account' is empty!\n";
		return 0;
	}

	$billing_account_id = get_billing_account_id($billing_account);
	if($billing_account_id == 0) {
		$billing_account_id = insert_billing_account($billing_account,$curr_id,$leg,$cdr_server_id);
	}
		
	$rating_account_id = get_rating_account_id($rating_mode,$rating_account);
	if(($rating_account_id == 0)AND($billing_account_id > 0)) {
		if($rating_mode_id <= 4) $rating_account_id = insert_rating_account($rating_mode,$rating_account,$billing_account_id,$bill_plan_id);
		if($rating_mode_id >= 5) $rating_account_id = insert_rating_account_2($rating_mode,$rating_account,$billing_account_id,$bill_plan_id,$clg_nadi,$cld_nadi);
	}
	
	if($rating_account_id == 0) {
		echo "Error! A 'rating_account' is not inserted!\n";
		return 0;
	}
	
	return $billing_account_id;
}

function re6_import_pcard($billing_account_id,$buf)
{
	$pcard_type   = trim($buf[1],'"');
	$pcard_status = trim($buf[2],'"');
	$amount = $buf[3];
	$start_date = trim($buf[4],'"');
	$end_date   = trim($buf[5],'"');
		
	$pcard_id = 0;
	$pcard_type_id = 0;	
	$pcard_status_id  = 0;
	
	if($billing_account_id) {
		if(!empty($pcard_status)) $pcard_status_id = get_pcard_status_id($pcard_status);
		
		if(!empty($pcard_type)) $pcard_type_id = get_pcard_type_id($pcard_type);
		
		if($pcard_type_id) {
			$pcard_id = get_pcard_id($billing_account_id,$pcard_type_id,$pcard_status_id,$amount,$start_date,$end_date);
			if($pcard_id == 0) { 
				$pcard_id = insert_pcard($billing_account_id,$pcard_type_id,$pcard_status_id,$amount,$start_date,$end_date);
			}
		}
	}
	
	return $pcard_id;
}

function re6_import_settings($imp_file)
{
	$fp = fopen($imp_file,"r");
	if ($fp) {
		while(($buffer = fgets($fp, 4096)) !== false) {
			$row = trim($buffer,"\n");
			$buf = explode(",",$row);

			$config_mode = $buf[0];
				
			/* Bill Plan */
			if($config_mode == '1') {
				$bill_plan_id = re6_import_bplan($buf);
			}

			/* Tariff,Prefix => Rate */	    
			if($config_mode == '2') {
				if(empty($bill_plan_id)) {
					echo "You haven't got a bill plan!\n";
					exit;
				}
			
				$tariff_id = re6_import_rate($bill_plan_id,$buf);
			}
	    
	    	/* Calc Functions */
			if($config_mode == '*') {
				$pos = $buf[1];
				$fee = $buf[2];
				$delta_time = $buf[3];
				$iterations = $buf[4];
						
				if(empty($tariff_id)) {
					echo "You haven't got a tariff!\n";
					exit;
				}
				
				$calc_id = get_calc_id($tariff_id,$pos);		
				if($calc_id == 0) $calc_id = insert_calc_func($tariff_id,$pos,$fee,$delta_time,$iterations);
				
				echo "calc_id: $calc_id , tariff_id: $tariff_id\n";
			}
	    
	    	/* Rating Account */
			if($config_mode == '3') {
				if(empty($bill_plan_id)) {
					echo "You haven't got a 'bill plan'!\n";
					exit;
				}
				
				$billing_account_id = re6_import_racc($bill_plan_id,$buf);
			}
	
			/* PCard */
			if($config_mode == '4') {
				if(empty($billing_account_id)) {
					echo "You haven't got a 'billing_account' !\n";
					exit;
				}
				
				$pcard_id = re6_import_pcard($billing_account_id,$buf);
				if($pcard_id == 0) {
					echo "Error! A 'pcard' is not inserted!\n";
				}
			}
	
			/* Bill Plan Tree */
			if($config_mode == '5') {
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
			
			/* Free Billsec */
			if($config_mode == 'f') {
				if(!empty($buf[1])) {
					$free_billsec = $buf[1];
				
					$id = get_free_billsec_id($free_billsec); 
					if($id == 0) $id = insert_free_billsec($free_billsec);
						
					echo "free_billsec = $free_billsec (sec.),id = $id\n";
				} else echo "Error! A 'free_billsec' is empty!\n";
			}
			
			/* Time Condition definitions */
			if($config_mode == 't') {
				if(!empty($buf[1])) $tc_name = trim($buf[1],'"');
				else continue;
				
				if(!empty($buf[2])) $hours = trim($buf[2],'"');
				else $hours = NULL;
				
				if(!empty($buf[3])) $days_week = trim($buf[3],'"');
				else $days_week = NULL;
				
				if(!empty($buf[4])) $tc_date = trim($buf[4],'"');
				else $tc_date = NULL;
				
				if(!empty($buf[5])) $year = trim($buf[5],'"');
				else $year = NULL;
				
				if(!empty($buf[6])) $month = trim($buf[6],'"');
				else $month = NULL;
				
				if(!empty($buf[7])) $day_month = trim($buf[7],'"');
				else $day_month = NULL;
			
				$tc_id = get_time_condition_deff_id($tc_name);
				if($tc_id == 0) {
					$tc_id = insert_time_condition_deff($hours,$days_week,$tc_name,$tc_date,$year,$month,$day_month);
					if($tc_id == 0) {
						echo "Error! A 'time_condition' is not inserted!\n";
					}
				} else {
					echo "tc_id: $tc_id\n";
				}
			}
		}
	}
 
	fclose($fp);
}
?>
