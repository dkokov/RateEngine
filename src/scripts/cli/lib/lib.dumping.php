<?php

function re6_dump_free_billsec_settings()
{
	$arr = get_free_billsec();
	
	if($arr == NULL) return;
		
	$i = 0;
	while(!empty($arr[$i])) {
		echo "id: ".$arr[$i]['id']." , free_billsec: ".$arr[$i]['free_billsec']."\n";
		
		$i++;	
	}
}

function re6_dump_time_condition_deff()
{
	$arr = get_time_condition_deff();
	
	if($arr == NULL) return;
		
	$i = 0;
	while(!empty($arr[$i])) {
		echo "id: ".$arr[$i]['id']." , tc name: ".$arr[$i]['tc_name']." , hours: ".$arr[$i]['hours'];
		echo " , days_week: ".$arr[$i]['days_week']." , tc_date: ".$arr[$i]['tc_date']." , year: ".$arr[$i]['year'];
		echo " , month: ".$arr[$i]['month']." , day_month: ".$arr[$i]['day_month']."\n";
		
		$i++;	
	}
}

function re6_dump_cdr_servers()
{
	$list = get_cdr_servers();

	if(empty($list)) return;
		
	$i=0;
	while(!empty($list[$i])) {
		echo $list[$i]['cdr_server_id']." , ".$list[$i]['profile_name']." , ".$list[$i]['get_mode']." , ".$list[$i]['active']."\n";
		
		$i++;
	}
}

function re6_dump_cdr_storage_sched()
{
	re5_conn();
	
	$ts = re6_cdr_storage_sched_ts();
	
	re5_close();

	if(empty($ts)) return;
		
	$i=0;
	while(!empty($ts[$i])) {
		echo $ts[$i]['profile_name']." , ".$ts[$i]['last']." , ".$ts[$i]['start']."\n";
		
		$i++;
	}
}

function re6_dump_bplan_settings($bplan)
{	
	$bid = get_bill_plan_id($bplan);
	if($bid) {
		/* Is this Bill Plan tree ? */
		$bplans = get_bill_plans_tree($bid);
		
		if(empty($bplans)) {
			/* Only one Bill Plan without tree */
			$bplans = get_bill_plans_v2($bid);
		}
		
		/* File Headers */
		echo "\nbill_plan,prefix_id,prefix,tariff_id,tariff,free_billsec_id,free_billsec,pos,delta_time,fee,iterations\n";
		
		$i=0;
		while(!empty($bplans[$i])) {
			if($bplans[$i]['id']) {
				echo "\n".$bplans[$i]['bplan'].",";
				$rates = get_rate_data($bplans[$i]['id']);
				
				$c=0;
				while(!empty($rates[$c])) {
					/* All rates (prefix,tariff,tc) per this Bill Plan */
					echo "\n ,".$rates[$c]['prefix_id'].",".$rates[$c]['prefix'].",".$rates[$c]['tariff_id'].",".$rates[$c]['tariff']."";

					$free = get_free_billsec_v2($rates[$c]['tariff_id']);
					
					if(!empty($free)) {
						/* Free Billsec into this tariff */
						echo ",".$free['free_billsec_id'].",".$free['free_billsec']; 
					}
					
					$calc = get_calc_functions_v2($rates[$c]['tariff_id']);
					
					$p=0;
					while(!empty($calc[$p])) {
						/* Calculate functions into this tariff */
						echo "\n , , , , , , ,".$calc[$p]['pos'].",".$calc[$p]['delta_time'].",".$calc[$p]['fee'].",".$calc[$p]['iterations'];
						$p++;
					}

					echo "\n";

					$c++;
				}
				
				echo "\n";
			}
			
			$i++;
		}
	}
}
?>
