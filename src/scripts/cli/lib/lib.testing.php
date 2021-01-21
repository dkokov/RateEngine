<?php
 require('../config.app.php');
 require('lib.re5.php');
 
 define("PREFIX","35910");
 
// define("PCARD_AMOUNT",20);
 $pcard_amount = 20;
 define("PCARD_TYPE","credit");
 define("PCARD_STATUS","active");
 
 define("CDR_SERVER_ID",1);
 define("CURR_ID",1);
 define("LEG",'a');
 define("BDAY","01");
 
 define("RT_MODE","calling_number");
 
 function create_test_calling_number_racc($num_acc,$n,$bplan,$sm_bplan)
 {
	global $pcard_amount;
	
	 if(empty($bplan)) return;
	 	 
	 re5_conn();
	 
	 $i = $n;
	 while($i < ($num_acc+$n)) {
		 $acc = "";		 
		 $user = "";
		 
		 $len = strlen($i);
		 
		 for($c=0;$c<(6-$len);$c++) $acc .= "0"; 
		 		 
		 $acc = PREFIX.$acc.$i;
		 
		 $user = array(
			'cdr_server_id'   => CDR_SERVER_ID,
			'billing_account' => "bacc_$acc",
			'billing_day'     => BDAY,
			'curr_id'         => CURR_ID,
			'leg'             => LEG,
			'rating_account'  => $acc,
			'rating_mode'     => RT_MODE,
			're5_bill_plan'       => $bplan,
			'pcard_type'      => PCARD_TYPE,
			'pcard_status'    => PCARD_STATUS,
			'amount'          => $pcard_amount,
			'start_date'      => NULL,
			'end_date'        => NULL
		);
		 
		 if(!empty($sm_bplan)) {
			$user['sm_bill_plan'] = $sm_bplan;
		 } else {
			$user['sm_bill_plan'] = NULL;
		 }
		
		 $ret = re5_create_account($user);
		 if($ret['re5_racc_id'] > 0) echo "$acc\n";
		
		 $i++;
	 }
	 
	 re5_close();
 }
 
// create_test_calling_number_racc($argv[1],$argv[2],$argv[3],$argv[4]);

//create_test_calling_number_racc(10000,0,"MobileStart","SMS");
//create_test_calling_number_racc(10000,10000,"MobileStandart","SMS");
//create_test_calling_number_racc(30000,20000,"MobileUnlimited","SMS");
 
$pcard_amount = 0.000001;
create_test_calling_number_racc(50000,50000,"MobilePromo1","SMS-Promo1");

?>
