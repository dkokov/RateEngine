<?php

define("PREFIX","35910");
define("PCARD_AMOUNT",20);
define("PCARD_TYPE","credit");
define("PCARD_STATUS","active");

define("CDR_SERVER_ID",1);
define("CURR_ID",1);
define("LEG",'a');
define("BDAY","01");

define("RT_MODE","calling_number");

$pcard_amount = PCARD_AMOUNT;

$clg_dst = array("112","35980011011",
				 "359608","359658","359312","359375","359514","359594","359743","359847","359974",
				 "35910",
				 "359");

$tgr_dst = array("359","359700","359800");

function gen_E164_num($prefix,$fill,$n)
{
	$num = NULL;
	
	$len = strlen($n);
	
	for($c=0;$c<($fill-$len);$c++) $num .= "0";
	
	return $prefix.$num.$n;
}

function generate_cdrs($rt_mode,$n)
{
	global $FS;
	global $clg_dst;
	global $tgr_dst;
	
	if(empty($n)) $n = 1;
	if(empty($rt_mode)) $rt_mode = "calling_number";
	
	if(strlen($FS['dbhost'])) $config = "host=".$FS['dbhost'];
	if(strlen($FS['dbname'])) $config = $config." dbname=".$FS['dbname'];
	if(strlen($FS['dbuser'])) $config = $config." user=".$FS['dbuser'];
	if(strlen($FS['dbpass'])) $config = $config." password=".$FS['dbpass']."";
	
	$dbconn = pg_connect($config);
	if($dbconn == FALSE) {
		echo "ERROR! Cannot connect to PGSQL server!\n";
		exit;
	}
	
	$clg_dst_cnt  = count($clg_dst) - 1;
	$tgr_dst_cnt  = count($tgr_dst) - 1;
	
	for($i=0;$i<$n;$i++) {
		$uuid = shell_exec("uuid");
		$ts   = date("Y-m-d H:i:s");
		
		$src  = NULL;
		$dst  = NULL;
		
		$bsec = mt_rand(0,3660);
		$dur  = $bsec + mt_rand(0,45);
		
		if($rt_mode == "calling_number") {
			$src  = gen_E164_num(PREFIX,6,mt_rand(0,100000));
			$dst  = $clg_dst[mt_rand(0,$clg_dst_cnt)];

			$sql = "insert into cdrs (call_uid,ts,src,dst,billsec,duration) values ('$uuid','$ts','$src','$dst',$bsec,$dur)";
		} else if($rt_mode == "account_code") {
			$acc  = "acc-1";
			$src  = "359";
			$dst  = $clg_dst[mt_rand(0,$clg_dst_cnt)];

			$sql = "insert into cdrs (call_uid,ts,src,dst,account_code,billsec,duration) values ('$uuid','$ts','$src','$dst','$acc',$bsec,$dur)";
		} else if($rt_mode == "src_context") {
			$src = "359";
			$dst = $tgr_dst[mt_rand(0,$tgr_dst_cnt)];
			
			$src_context = "abc-context";
			
			$sql = "insert into cdrs (call_uid,ts,src,dst,src_context,billsec,duration) values ('$uuid','$ts','$src','$dst','$src_context',$bsec,$dur)";
		} else if($rt_mode == "src_tgroup") {
			$src = "359";
			$dst = $tgr_dst[mt_rand(0,$tgr_dst_cnt)];
			
			$src_tgroup = "abc-tgroup";
			
			$clg_nadi = mt_rand(0,4);
			$cld_nadi = mt_rand(3,4);
			
			$sql = "insert into cdrs (call_uid,ts,src,dst,src_tgroup,billsec,duration,clg_nadi,cld_nadi) values ('$uuid','$ts','$src','$dst','$src_tgroup',$bsec,$dur,$clg_nadi,$cld_nadi)";
		} else if($rt_mode == "dst_context") {
			$src  = gen_E164_num(PREFIX,6,mt_rand(0,100000));
			$dst = $tgr_dst[mt_rand(0,$tgr_dst_cnt)];
			
			$dst_context = "abc-context";
			
			$sql = "insert into cdrs (call_uid,ts,src,dst,dst_context,billsec,duration) values ('$uuid','$ts','$src','$dst','$dst_context',$bsec,$dur)";
		} else if($rt_mode == "dst_tgroup") {
			$src  = gen_E164_num(PREFIX,6,mt_rand(0,100000));
			$dst = $tgr_dst[mt_rand(0,$tgr_dst_cnt)];
			
			$dst_tgroup = "abc-tgroup";
			
			$clg_nadi = mt_rand(0,4);
			$cld_nadi = mt_rand(3,4);
			
			$sql = "insert into cdrs (call_uid,ts,src,dst,dst_tgroup,billsec,duration,clg_nadi,cld_nadi) values ('$uuid','$ts','$src','$dst','$dst_tgroup',$bsec,$dur,$clg_nadi,$cld_nadi)";
		}
		
		pg_query($dbconn,$sql);
	}
	
	pg_close($dbconn);
}

function generate_cdrs_model_1()
{
	generate_cdrs('calling_number',100000);
	
	generate_cdrs('account_code',1000);
	
	generate_cdrs('src_context',10000);

	generate_cdrs('dst_context',10000);
	
	generate_cdrs('src_tgroup',10000);
	
	generate_cdrs('dst_tgroup',10000);
}

function generate_cdrs_file($filename,$rt_mode,$n)
{
	global $clg_dst;
	global $tgr_dst;
	
	if(empty($n)) $n = 1;
	if(empty($rt_mode)) $rt_mode = "calling_number";
	
	$clg_dst_cnt  = count($clg_dst) - 1;
	$tgr_dst_cnt  = count($tgr_dst) - 1;
	
	if(!$fp = fopen($filename, 'w')) {
		echo "Cannot open file ($filename)";
		return;
	}
	
	for($i=0;$i<$n;$i++) {
		$uuid = trim(shell_exec("uuid"),"\n");
		$ts   = date("Y-m-d H:i:s");
		
		$src  = NULL;
		$dst  = NULL;
		
		$bsec = mt_rand(0,3660);
		$dur  = $bsec + mt_rand(0,45);
		
		if($rt_mode == "calling_number") {
			$src  = gen_E164_num(PREFIX,6,mt_rand(0,100000));
			$dst  = $clg_dst[mt_rand(0,$clg_dst_cnt)];
			
			fwrite($fp,"'$uuid','$ts','$src','$dst',$bsec,$dur\n");
		} else if($rt_mode == "account_code") {
			$acc  = "acc-1";
			$src  = "359";
			$dst  = $clg_dst[mt_rand(0,$clg_dst_cnt)];
			
			fwrite($fp,"'$uuid','$ts','$src','$dst','$acc',$bsec,$dur\n");
		} else if($rt_mode == "src_context") {
			$src = "359";
			$dst = $tgr_dst[mt_rand(0,$tgr_dst_cnt)];
			
			$src_context = "abc-context";
			
			fwrite($fp,"'$uuid','$ts','$src','$dst','$src_context',$bsec,$dur\n");
		} else if($rt_mode == "src_tgroup") {
			$src = "359";
			$dst = $tgr_dst[mt_rand(0,$tgr_dst_cnt)];
			
			$src_tgroup = "abc-tgroup";
			
			$clg_nadi = mt_rand(0,4);
			$cld_nadi = mt_rand(3,4);
			
			fwrite($fp,"'$uuid','$ts','$src','$dst','$src_tgroup',$bsec,$dur,$clg_nadi,$cld_nadi\n");
		} else if($rt_mode == "dst_context") {
			$src  = gen_E164_num(PREFIX,6,mt_rand(0,100000));
			$dst = $tgr_dst[mt_rand(0,$tgr_dst_cnt)];
			
			$dst_context = "abc-context";
			
			fwrite($fp,"'$uuid','$ts','$src','$dst','$dst_context',$bsec,$dur");
		} else if($rt_mode == "dst_tgroup") {
			$src  = gen_E164_num(PREFIX,6,mt_rand(0,100000));
			$dst = $tgr_dst[mt_rand(0,$tgr_dst_cnt)];
			
			$dst_tgroup = "abc-tgroup";
			
			$clg_nadi = mt_rand(0,4);
			$cld_nadi = mt_rand(3,4);
			
			fwrite($fp,"'$uuid','$ts','$src','$dst','$dst_tgroup',$bsec,$dur,$clg_nadi,$cld_nadi\n");
		}
	}
	
	fclose($fp);
}

function create_test_calling_number_racc($fp,$num_acc,$n,$bplan,$sm_bplan)
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
		're5_bill_plan'   => $bplan,
		'pcard_type'      => PCARD_TYPE,
		'pcard_status'    => PCARD_STATUS,
		'amount'          => $pcard_amount,
		'start_date'      => date('Y-m-d'),
		'end_date'        => NULL
	);
	
	if(!empty($sm_bplan)) {
	    $user['sm_bill_plan'] = $sm_bplan;
	} else {
	    $user['sm_bill_plan'] = NULL;
	}
	
	$ret = re5_create_account($user);
	if($ret['re5_racc_id'] > 0) {
	    if($fp) fwrite($fp,$acc."\n");
	    else echo "$acc\n";
	}
	
	$i++;
    }

    re5_close();
}

function create_test_model_1_bp()
{
	/* 'Time conditions deff' */
	re6_import_settings('test_bp/tc.csv');
	
	/* 'SMS' bill plan */
	re6_import_settings('test_bp/SMS.csv');
	
	/* 'FreeCalls' bill plan */
	re6_import_settings('test_bp/FreeCalls.csv');
	
	/* 'FIX' bill plan */
	re6_import_settings('test_bp/FIX.csv');
	
	/* 'MobilePromo1' bill plans */
	re6_import_settings('test_bp/MobilePromo1/International-Promo1.csv'); 
	re6_import_settings('test_bp/MobilePromo1/SMS-Promo1.csv'); 
	re6_import_settings('test_bp/MobilePromo1/MobilePromo1.csv');

	/* 'MobileStart' bill plan */
	re6_import_settings('test_bp/MobileStart/International-Start.csv');
	re6_import_settings('test_bp/MobileStart/MobileStart.csv');

	/* 'MobileStandart' bill plans */
	re6_import_settings('test_bp/MobileStandart/International-Standart.csv');
	re6_import_settings('test_bp/MobileStandart/MobileStandart.csv');
	
	/* 'MobileUnlimited' bill plans */
	re6_import_settings('test_bp/MobileUnlimited/International-Unlimited.csv'); 
	re6_import_settings('test_bp/MobileUnlimited/MobileUnlimited.csv');
	
	/* 'AccountCode' bill plans and rating account */
	re6_import_settings('test_bp/AccountCode/International-acc.csv');
	re6_import_settings('test_bp/AccountCode/AccountCode.csv');
	
	/* 'src_context' bill plans and rating_account */
	re6_import_settings('test_bp/Trunks/trunk_src_context.csv');
	
	/* 'dst_context' bill plans and rating_account */
	re6_import_settings('test_bp/Trunks/trunk_dst_context.csv');
}

function create_test_model_1($filename)
{
    if(strlen($filename) > 0) {
		$fp = fopen($filename,"a+");
		if(!$fp) {
	    	echo "ERROR!Cannot open file!\n";
	    	return 1;
		}
    } else $fp = NULL;

    create_test_calling_number_racc($fp,10000,0,"MobileStart","SMS");
    create_test_calling_number_racc($fp,10000,10000,"MobileStandart","SMS");
    create_test_calling_number_racc($fp,30000,20000,"MobileUnlimited","SMS");

    $pcard_amount = 0.000001;
    create_test_calling_number_racc($fp,50000,50000,"MobilePromo1","SMS-Promo1");

    if($fp) fclose($fp);

    return 0;
}

?>
