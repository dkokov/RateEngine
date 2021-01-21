<?php

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
