<?php

 include("config.app.php");

 $imp_file = $argv[1];
 if(empty($imp_file)) 
 {
    echo "You havent't got a input file!\n";
    exit;
 }

 $bill_plan_id = $argv[2];
 if(empty($bill_plan_id)) 
 {
    echo "You havent't got a bill_plan_id!\n";
    exit;
 }

 $tariff_id = $argv[3];
 if(empty($tariff_id)) 
 {
    echo "You havent't got a tariff_id!\n";
    exit;
 }

 $fp = fopen($imp_file,"r");
 if($fp) 
 {
    while(($buffer = fgets($fp, 4096)) !== false) 
    {
	$prefix = trim($buffer,"\n");
	
	check_1:
	$prefix_id = get_prefix_id($prefix);
	if($prefix_id == 0)
	{
	    insert_prefix($prefix,"");
	    goto check_1;
	}
	
	check_2:
	$rate_id = get_rate_id($bill_plan_id,$prefix_id,$tariff_id);
	if(($rate_id == 0) and ($bill_plan_id) and ($prefix_id) and ($tariff_id))
	{
	    insert_rate($bill_plan_id,$prefix_id,$tariff_id);
	    goto check_2;
	}
	
	echo $prefix.",".$prefix_id.",".$rate_id."\n";
    }
 }

 fclose($fp);
?>