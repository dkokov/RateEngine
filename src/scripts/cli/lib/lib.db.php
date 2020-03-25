<?php

 function conn()
 {
    global $RE;

    if(strlen($RE['dbhost'])) $config = "host=".$RE['dbhost'];
    if(strlen($RE['dbname'])) $config = $config." dbname=".$RE['dbname'];
    if(strlen($RE['dbuser'])) $config = $config." user=".$RE['dbuser'];
    if(strlen($RE['dbpass'])) $config = $config." password=".$RE['dbpass']."";

    $dbconn = pg_connect($config);
    return $dbconn;
 }

 function get_cdr($process)
 {
    global $dbconn;
    $query = "select dst,accountcode,billsec,calldate 
              from cdrs
              where uniqueid ='".$process->uniqueid."'";
    $result = pg_query($dbconn,$query);
    
    while($row = pg_fetch_row($result))
    {
	$process->called_number = $row[0];
	$process->accountcode = $row[1];
	$process->billsec = $row[2];
	$process->timestamp = $row[3];
    }
 }

function get_cdr_servers()
{
    global $dbconn;

    $servers = NULL;

    $query = "select srv.id,prof.profile_name,md.name,srv.active 
    		  from cdr_get_mode as md,cdr_servers as srv,cdr_profiles as prof 
    		  where srv.get_mode_id = md.id and srv.cdr_profiles_id = prof.id;";
    $result = pg_query($dbconn,$query);

    $i=0;
    while($row = pg_fetch_row($result)) {
		$servers[$i]['cdr_server_id'] = $row[0];
		$servers[$i]['profile_name']  = $row[1];
		$servers[$i]['get_mode'] = $row[2];
		$servers[$i]['active']   = $row[3];
		
		$i++;
    }
    
    return $servers;
}

 function get_dbstorage($id)
 {
    global $dbconn;

    $servers = NULL;

    $query = "select * from cdr_dbstorage where cdr_server_id = ".$id."";
    $result = pg_query($dbconn,$query);

    while($row = pg_fetch_row($result))
    {
	$servers['dbhost'] = $row[2];
	$servers['dbname'] = $row[3];
	$servers['dbuser'] = $row[4];
	$servers['dbpass'] = $row[5];
	$servers['cdr_table'] = $row[6];
	$servers['dbstorage_type_id'] = $row[7];
    }
    return $servers;
 }

 function get_debit_card($DebitCard)
 {
    global $dbconn;
    $query = "select dc.id,dc.billing_account_id,dc.amount,dc.end_date,dc.last_update,dc.active,dc.start_date
              from debit_card as dc
              where 
              dc.id =".$DebitCard->CardID." and
              dc.active = 't'";
    $result = pg_query($dbconn,$query);
    
    while($row = pg_fetch_row($result))
    {
	$DebitCard->CardID = $row[0];
	$DebitCard->BillingAccountID = $row[1];
	$DebitCard->Amount = $row[2];
	$DebitCard->EndDate = $row[3];
	$DebitCard->LastUpdate = $row[4];
	$DebitCard->Active = $row[5];
	$DebitCard->StartDate = $row[6];
    }
 }

 function get_credit_card($CreditCard)
 {
    global $dbconn;
    $query = "select dc.id,dc.amount
              from credit_card as dc
              where 
              dc.id =".$CreditCard->CardID."";
    $result = pg_query($dbconn,$query);
    
    while($row = pg_fetch_row($result))
    {
	$CreditCard->CardID = $row[0];
	$CreditCard->Amount = $row[1];
    }
 }

function get_pcard_type_id($pcard_type)
{
    global $dbconn;

    $id = 0;
    
    $query = "select id from pcard_type where name like '".$pcard_type."%'";
    $result = pg_query($dbconn,$query);

    while($row = pg_fetch_row($result)) {
		$id = $row[0];
    }
    
    return $id;
}

 function get_pcard_status_id($pcard_status)
 {
    global $dbconn;
    $id = 0;
    $query = "select id
              from pcard_status
              where 
              status like '".$pcard_status."%'";
    $result = pg_query($dbconn,$query);

    while($row = pg_fetch_row($result))
    {
	$id = $row[0];
    }
    return $id;
 }

 function get_pcard_id($bacc,$type_id,$status_id,$amount,$start,$end)
 {
    global $dbconn;
    $id = 0;
    $query = "select id
              from pcard
              where 
              billing_account_id = ".$bacc." and
              pcard_type_id = ".$type_id." and
              pcard_status_id = ".$status_id." and
              amount = ".$amount." and
              start_date = '".$start."' and
              end_date = '".$end."'";
    $result = pg_query($dbconn,$query);

    while($row = pg_fetch_row($result))
    {
	$id = $row[0];
    }
    return $id;
 }

 function get_balance($Balance)
 {
    global $dbconn;
    $query = "select bl.id,bl.debit_card_id,bl.credit_card_id,bl.amount,bl.last_update
              from balance as bl
              where 
              bl.billing_account_id =".$Balance->BillingAccountID."";
    $result = pg_query($dbconn,$query);
    
    while($row = pg_fetch_row($result))
    {
	$Balance->BalanceID = $row[0];
	$Balance->DebitCardID = $row[1];
	$Balance->CreditCardID = $row[2];
	$Balance->Amount = $row[3];
	$Balance->LastUpdate = $row[4];
    }
 }

 function get_bacc_balance()
 {
    global $dbconn;
    $bacc = NULL;
    
    $query = "select billing_account_id from balance order by billing_account_id";
    $result = pg_query($dbconn,$query);
    $i=0;
    while($row = pg_fetch_row($result))
    {
	$bacc[$i] = $row[0];
	$i++;
    }
    return $bacc;
 }
 
 function make_balance_debit($debit_card,$balance)
 {
    global $dbconn;
    $query = "select sum(rt.call_price) 
              from rating as rt,cdrs 
              where cdrs.id = rt.call_id and
              rt.billing_account_id = ".$balance->BillingAccountID." and
              cdrs.ts >= '".$debit_card->StartDate." 00:00:00' and cdrs.ts <= '".$debit_card->EndDate." 23:59:59'";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	if(!empty($row[0])) $balance->Amount = $row[0];
    }
 }

 function make_balance_credit($credit_card,$balance)
 {
    global $dbconn;
    $year = date('Y');
    $month = date('m');
    $next = $month + 1;
    
    $start_date = $year."-".$month."-01 00:00:00";
    if($next == '01') $year++; // pyrvi mesec ot nova godina !!!???
    $end_date = $year."-".$next."-01 00:00:00";
    
    $query = "select sum(rt.call_price) 
              from rating as rt,cdrs 
              where cdrs.id = rt.call_id and
              rt.call_price > 0 and
              rt.billing_account_id = ".$balance->BillingAccountID." and
              cdrs.ts >= '".$start_date."' and cdrs.ts <= '".$end_date."'";
    //echo $query."\n";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	if(!empty($row[0])) $balance->Amount = $row[0];
	else $balance->Amount = 0.00;
    }
 }

 function get_bill_tree_id($tree,$root)
 {
    global $dbconn;
    $id = 0;
    $query = "select id from bill_plan_tree 
              where bill_plan_id = ".$tree." and 
              root_bplan_id = ".$root."";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res)) $id = $row[0];
    return $id;
 }


 function get_bill_plan_id($bill_plan)
 {
    global $dbconn;
    $id = 0;
    $query = "select id from bill_plan where name = '".$bill_plan."'";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }

 function get_bill_plans()
 {
    global $dbconn;

    $query = "select id,name,to_timestamp(start_period),to_timestamp(end_period) from bill_plan";
    $res = pg_query($dbconn,$query);
    $i=0;
    while($row = pg_fetch_row($res))
    {
	$bplans[$i]['id'] = $row[0];
	$bplans[$i]['bplan'] = $row[1];
	$bplans[$i]['speriod'] = $row[2];
	$bplans[$i]['eperiod'] = $row[3];
	$i++;
    }
    return $bplans;
 }
 
 function get_bill_plans_v2($bid)
 {
	global $dbconn;

	$bplans = NULL;

	$query = "select id,name from bill_plan where id = $bid";
	$res = pg_query($dbconn,$query);

	$i=0;
	while($row = pg_fetch_row($res)) {
		$bplans[$i]['id'] = $row[0];
		$bplans[$i]['bplan'] = $row[1];
		$i++;
	}
    
    return $bplans;
 }

 function get_bill_plans_tree($tree_id)
 {
	global $dbconn;

	$bplans = NULL;

	$query = "select bill_plan.id,bill_plan.name 
			from bill_plan_tree,bill_plan 
			where bill_plan_tree.bill_plan_id = bill_plan.id and root_bplan_id = $tree_id order by bill_plan.id;";
	$res = pg_query($dbconn,$query);

	$i=0;
	while($row = pg_fetch_row($res)) {
		$bplans[$i]['id'] = $row[0];
		$bplans[$i]['bplan'] = $row[1];
		$i++;
	}
    
    return $bplans;
 }
 
 function get_bill_plan_type_id($bill_plan_type)
 {
    global $dbconn;
    $id = 0;
    $query = "select id from bill_plan_type where name = '".$bill_plan_type."'";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }
 

 function get_prefix_id($prefix)
 {
    global $dbconn;
    $id = 0;
    $query = "select id from prefix where prefix = '".$prefix."'";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }

 function get_prefix_filters()
 {
    global $dbconn;
    $filers = NULL;
    $query = "select filtering_prefix,filtering_number,replace_str 
              from prefix_filter";
    $res = pg_query($dbconn,$query);
    $i=0;
    while($row = pg_fetch_row($res))
    {
	$filters[$i]['prefix'] = $row[0];
	$filters[$i]['num'] = $row[1];
	$filters[$i]['replace'] = $row[2];
	$i++;
    }
    return $filters;
 }

 function get_free_id($tariff_id)
 {
    global $dbconn;
    $id = 0;
    $query = "select id from free_billsec where tariff_id = ".$tariff_id."";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }

 function get_time_condition_id($tariff_id,$time_zone)
 {
    global $dbconn;
    $id = 0;
    $query = "select tc.id 
              from time_condition as tc,time_condition_deff as deff
              where deff.tc_name = '".$time_zone."' and
              deff.id = tc.time_condition_id and tc.tariff_id = ".$tariff_id."
              ";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }
 

function get_time_condition_deff_id($time_zone)
{
    global $dbconn;
    
    $id = 0;
    
    $query = "select id from time_condition_deff where tc_name = '".$time_zone."'";
    $res = pg_query($dbconn,$query);
    
    while($row = pg_fetch_row($res)) {
		$id = $row[0];
    }
    
    return $id;	
}

 function get_billing_account_id($billing_account)
 {
    global $dbconn;
    $id = 0;
    $query = "select id from billing_account where username = '".$billing_account."'";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }

function get_bacc_count($id)
{
	global $dbconn;
	
	$count = 0;
	
	$query = "select count(*) from billing_account where cdr_server_id = $id";
	$res = pg_query($dbconn,$query);
	
	while($row = pg_fetch_row($res)) {
		$count = $row[0];
	}

	return $count;
}

 function get_rating_account_id($rating_mode,$rating_account)
 {
    global $dbconn;
    $id = 0;
    $query = "select id from ".$rating_mode." where ".$rating_mode." = '".$rating_account."'";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }


function get_rating_mode($id)
{
    global $dbconn;

    $mode = NULL;

    $query = "select name from rating_mode where id = ".$id."";
    
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res)) {
		$mode = $row[0];
    }

    return $mode;
}

function get_rating_mode_id($mode)
{
    global $dbconn;

    $id = 0;

    $query = "select id from rating_mode where name = '".$mode."'";
  
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res)) {
		$id = $row[0];
    }

    return $id;
}

function get_calc_id($tariff_id,$pos)
{
    global $dbconn;

    $id = 0;

    $query = "select id 
              from calc_function 
              where tariff_id = ".$tariff_id." and 
              pos = ".$pos."";
    $res = pg_query($dbconn,$query);

    while($row = pg_fetch_row($res)) $id = $row[0];

    return $id;
}

 function get_rate_id($bill_plan_id,$prefix_id,$tariff_id)
 {
    global $dbconn;
    $id = 0;
    $query = "select id 
              from rate 
              where 
              tariff_id = ".$tariff_id." and bill_plan_id = ".$bill_plan_id." and prefix_id = ".$prefix_id."";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }
 
 function get_rate_data($bill_plan_id)
 {
    global $dbconn;
    
    $rates = NULL;
	
	$query = "select rate.id,rate.tariff_id,tariff.name,rate.prefix_id,prefix.prefix
	from rate,prefix,tariff 
	where rate.tariff_id = tariff.id and rate.prefix_id = prefix.id and 
	rate.bill_plan_id = ".$bill_plan_id." order by rate.tariff_id";
	
    $res = pg_query($dbconn,$query);
	
	$i=0;
	while($row = pg_fetch_row($res)) {
		$rates[$i]['id'] = $row[0];
		$rates[$i]['tariff_id'] = $row[1];
		$rates[$i]['tariff']    = $row[2];
		$rates[$i]['prefix_id'] = $row[3];
		$rates[$i]['prefix']    = $row[4];
		$i++;
	}

	return $rates;
 }
 
 function get_tariff_id($bill_plan_id,$tariff)
 {
    global $dbconn;
    $id = 0;
    $query = "select rt.tariff_id 
              from rate as rt,tariff as tr 
              where tr.name = '".$tariff."' and
              rt.tariff_id = tr.id";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    
    if($id == 0)
    {
	$query = "select id from tariff
                  where name = '".$tariff."' and
                  temp_id = ".$bill_plan_id."";
	$res = pg_query($dbconn,$query);
	while($row = pg_fetch_row($res))
	{
	    $id = $row[0];
	}
    }
    return $id;
 }

 function get_cdr_2($process)
 {
    global $dbconn;
    $query = "SELECT 
              call_uid,calling_number,called_number,src_context,dst_context,account_code,billsec,ts,EXTRACT(DOW FROM ts) as dow,id
              from cdrs 
              where
              id = ".$process->cdr_id."";
    $result = pg_query($dbconn,$query);
    
    while($row = pg_fetch_row($result))
    {
	$process->call_uid = $row[0];
	$process->calling_number = $row[1];
	$process->called_number = $row[2];
	$process->src_context = $row[3];
	$process->dst_context = $row[4];
	$process->account_code = $row[5];
	$process->billsec = $row[6];
	$process->timestamp = $row[7];
	$process->dow = $row[8];
	$process->call_id = $row[9];
    }
 }

 function find_billing_account_id($billing_account)
 {
    global $dbconn;
    $billing_account_id = 0;
    if(!strcmp($billing_account,NULL)) return 0;
    
    $query = "select id 
              from billing_account
              where username = '".$billing_account."'";
    $result = pg_query($dbconn,$query);
    //echo $query."\n";
    while($row = pg_fetch_row($result))
    {
	$billing_account_id = $row[0];
    }
    return $billing_account_id;
 }

 function find_billing_account_data($billing_account)
 {
    global $dbconn;
    $bb['id']  = 0;
    $bb['leg'] = "";
    if(!strcmp($billing_account,NULL)) return 0;
    
    $query = "select bacc.id,bacc.leg,cr.name
              from billing_account as bacc,currency as cr
              where bacc.username = '".$billing_account."' and bacc.currency_id = cr.id";
    $result = pg_query($dbconn,$query);
    //echo $query."\n";
    while($row = pg_fetch_row($result))
    {
	$bb['id']  = $row[0];
	$bb['leg'] = $row[1];
	$bb['curr'] = $row[2];
    }
    return $bb;
 }

 function find_billing_account_data_2($bacc)
 {
    global $dbconn;
    $bb['id']  = 0;
    $bb['leg'] = "";
    if($bacc == 0) return 0;
    
    $query = "select bacc.id,bacc.leg,cr.name
              from billing_account as bacc,currency as cr
              where bacc.id = ".$bacc." and bacc.currency_id = cr.id";
    $result = pg_query($dbconn,$query);
    //echo $query."\n";
    while($row = pg_fetch_row($result))
    {
	$bb['id']  = $row[0];
	$bb['leg'] = $row[1];
	$bb['curr'] = $row[2];
    }
    return $bb;
 }

 function find_accountcode($process,$task)
 {
    global $dbconn;
    if(!strcmp($process->account_code,NULL)) return 0;
    
    $query = "select acc_deff.bill_plan_id,acc.billing_account_id 
              from account_code as acc ,account_code_deff as acc_deff
              where acc.account_code = '".$process->account_code."' and acc.id = acc_deff.account_code_id";
    $result = pg_query($dbconn,$query);
    //echo $query."\n";
    while($row = pg_fetch_row($result))
    {
	$task->bill_plan_id = $row[0];
	$task->billing_account_id = $row[1];
    }
 }

 function find_calling_number($process,$task)
 {
    global $dbconn;
    $query = "select deff.bill_plan_id,cn.billing_account_id 
              from 
              calling_number as cn,calling_number_deff as deff
              where 
              cn.calling_number = '".$process->calling_number."' and
              cn.id = deff.calling_number_id ";
    $result = pg_query($dbconn,$query);
    //echo $query."\n";
    while($row = pg_fetch_row($result))
    {
	$task->bill_plan_id = $row[0];
	$task->billing_account_id = $row[1];
    }
 }

 function find_src_context($process,$task)
 {
    global $dbconn;
    if(!strcmp($process->src_context,NULL)) return 0;
    
    $query = "select df.bill_plan_id,src.billing_account_id 
              from src_context as src ,src_context_deff as df
              where src.src_context = '".$process->src_context."'
              and src.id = df.src_context_id";
    $result = pg_query($dbconn,$query);
    //echo $query."\n";
    while($row = pg_fetch_row($result))
    {
	$task->bill_plan_id = $row[0];
	$task->billing_account_id = $row[1];
    }
 }

 function find_dst_context($process,$task)
 {
    global $dbconn;
    if(!strcmp($process->dst_context,NULL)) return 0;
    
    $query = "select df.bill_plan_id,dst.billing_account_id 
              from dst_context as dst,dst_context_deff as df
              where dst.dst_context = '".$process->dst_context."'
              and dst.id = df.dst_context_id";
    $result = pg_query($dbconn,$query);
    //echo $query."\n";
    while($row = pg_fetch_row($result))
    {
	$task->bill_plan_id = $row[0];
	$task->billing_account_id = $row[1];
    }
 }


 function find_prefix($process,$task)
 {
    global $dbconn;
    $query = "select id,prefix from order by prefix desc";
    $result = pg_query($dbconn,$query);

    $k=0;
    while($row = pg_fetch_row($result))
    {
	$prefix[$k]['id'] = $row[0];
	$prefix[$k]['prefix'] = $row[1];
	$k++;
    }
    
    $task->prefix_id = $row[0];
 }


 function find_time_condition($tariff_id)
 {
    global $dbconn;
    $query = "select df.id,df.hours,df.days_week,df.month,df.day_month 
              from time_condition as tc,time_condition_deff as df 
              where tc.tariff_id = ".$tariff_id." and tc.time_condition_id = df.id
              order by tc.id";
    $result = pg_query($dbconn,$query);
    $i = 0;
    $tc[$i]['id'] = 0;
    while($row = pg_fetch_row($result))
    {
	$tc[$i]['id'] = $row[0];
	$tc[$i]['hours'] = $row[1];
	$tc[$i]['days_week'] = $row[2];
	$tc[$i]['month'] = $row[3];
	$tc[$i]['day_month'] = $row[4];
	$i++;
    }
    return $tc;
 }

 function find_bill_plan($bill_plan_id)
 {
    global $dbconn;

/*    $query = "select rt.tariff_id,pr.prefix,pr.id,rt.id,rt.free_billsec_id
              from rate as rt,prefix as pr
              where 
              rt.prefix_id = pr.id and
              rt.bill_plan_id = ".$bill_plan_id."
              order by pr.prefix desc
              ";
    $result = pg_query($dbconn,$query);

    $k=0;$buf[$k]['tariff_id'] = 0;
    while($row = pg_fetch_row($result))
    {
	$buf[$k]['tariff_id'] = $row[0];
	$buf[$k]['prefix']    = $row[1];
	$buf[$k]['prefix_id'] = $row[2];
	$buf[$k]['rate_id']   = $row[3];
	$buf[$k]['free_billsec_id'] = $row[4];
	$k++;
    }*/
    
    $query = "select rt.tariff_id,pr.prefix,pr.id,rt.id,rt.free_billsec_id
              from rate as rt,prefix as pr
              where 
              rt.prefix_id = pr.id and
              rt.bill_plan_id = ".$bill_plan_id."
              order by pr.prefix desc
              ";
    $result = pg_query($dbconn,$query);

    $k=0;$buf[$k]['tariff_id'] = 0;
    while($row = pg_fetch_row($result))
    {
	$buf[$k]['tariff_id'] = $row[0];
	$buf[$k]['prefix']    = $row[1];
	$buf[$k]['prefix_id'] = $row[2];
	$buf[$k]['rate_id']   = $row[3];
	$buf[$k]['free_billsec_id'] = $row[4];
	$k++;
    }
    return $buf;
 }

 function compare_prefix($prefix,$called_number)
 {
    $len = strlen($prefix);
    $buf_str = substr($called_number,0,$len);
    if(!strcmp($prefix,$buf_str))
    {
	echo "called_number = ".$called_number." prefix = ".$prefix." buf_str = ".$buf_str."\n";
	return 1;
    }
    else return 0;
 }

 function find_celebr_days($ts)
 {
    global $dbconn;
    //$dates = NULL;
    $id = 0;
    $dt = $ts['year']."-".$ts['month'];
    $query = "select id from celebration_dates where date like '".$dt."%'";
    $res = pg_query($dbconn,$query);
    //$i = 0;
    while($row = pg_fetch_row($res))
    {
	//$dates[$i]['date'] = $row[0];
	$id = $row[0];
	//$dates[$i]['working'] = $row[1];
//	$i++;
    }
    return $id;
 }



 function find_celebr_dt_deff($dt_id)
 {
    global $dbconn;
    $query = "select id from celebration_dates where date like '".$dt."%'";
    $res = pg_query($dbconn,$query);
    $i = 0;
    while($row = pg_fetch_row($res))
    {
	//$dates[$i]['date'] = $row[0];
	$id = $row[0];
	//$dates[$i]['working'] = $row[1];
	$i++;
    }
    return $tc;
 }


 function find_bill_plan_periods($bill_plan_id)
 {
    global $dbconn;
    $periods = NULL;
    $query = "select start_period,end_period 
              from bill_plan
              where id = ".$bill_plan_id."";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$periods['start'] = $row[0];
	$periods['end'] = $row[1];
    }
    return $periods;
 }

 function get_ts($process)
 {
    $time_1 = explode(" ",$process->timestamp);
    $parts['date'] = $time_1[0];
    
    $time_2 = explode(":",$time_1[1]);
    $parts['hour'] = $time_2[0];
    $parts['min']  = $time_2[1];
    
    $date_buf = explode("-",$date);
    $parts['year']  = $date_buf[0];
    $parts['month'] = $date_buf[1];
    $parts['day'] = $date_buf[2];
    $parts['dow'] = $process->dow;
    if($parts['dow'] == 0) $parts['dow'] = 7;
    
    return $parts;
 }

 function compare_tc_ts($task,$tc,$ts_parts)
 {
    global $DAYS;

    $t=0;
    if($tc[$t]['id'] == 0) 
    {
	$task->time_condition_id = 0;
	return 1;
    }
    //print_r($tc);
    while($tc[$t]['id'])
    {
	$tc_buf = explode("-",$tc[$t]['hours']);
	$hours_1 = explode(":",$tc_buf[0]);
	$tc_hour_1 = "";$tc_hour_1 = $hours_1[0];
	$tc_min_1  = "";$tc_min_1  = $hours_1[1];
	$hours_2 = explode(":",$tc_buf[1]);
	$tc_hour_2 = "";$tc_hour_2 = $hours_2[0];
	$tc_min_2  = "";$tc_min_2  = $hours_2[1];
	$tc_month = "";$tc_month = $tc[$t]['month'];
	$tc_day_month = "";$tc_day_month = $tc[$t]['day_month'];
	$tc_buf_2 = explode("-",$tc[$t]['days_week']);

	$tc_day_1 = $tc_buf_2[0];
	$tc_day_2 = $tc_buf_2[1];
	$tc_day_1_id = $DAYS[$tc_day_1];$tc_day_2_id = $DAYS[$tc_day_2];
	
	//echo $t." ".$tc_hour_1."-".$tc_hour_2." *** \n";
	
	if(($tc_day_1_id <= $ts_parts['dow']) AND ($tc_day_2_id >= $ts_parts['dow']))
	{
	    echo "[".$ts_parts['dow']."]".$tc_day_1_id."-".$tc_day_2_id."\n";
	    /* I.time 1 < time 2 s
	         time1 <= hour <= time2 */
	    if($tc_hour_1 < $tc_hour_2)
	    {
		//if(($tc_hour_1 <= $ts_parts['hour']) AND ($tc_hour_2 >= $ts_parts['hour']))
		// ??? da se sledi , promeneno vyv version 0.3.9 !!!
		if(($tc_hour_1 <= $ts_parts['hour']) AND ($tc_hour_2 > $ts_parts['hour']))
		{
		    echo $tc_hour_1."-".$tc_hour_2."\n";
		    $task->time_condition_id = $tc[$t]['id'];
		    return 1;
		}
	    }
	
	    /* II. without hours */
	    if(($tc_hour_1 == '') AND ($tc_hour_2 == ''))
	    {
		echo "don't have hours!!!\n";
		$task->time_condition_id = $tc[$t]['id'];
		return 1;
	    }
	
	    /* III.time 1 > time 2 
	           (0 <= hour <= time2) or (time1 <= hour <= 24) */
	    if($tc_hour_1 > $tc_hour_2)
	    {
		//if(((0 <= $hour) AND ($hour <= $tc_hour_2)) AND
		//    (($tc_hour_1 <= $hour) AND ($hour <= 24)))
		    
		//if(($tc_hour_1 >= $ts_parts['hour']) AND ($tc_hour_2 <= $ts_parts['hour']))
		echo "hour = ".$ts_parts['hour']."\n";
		if($ts_parts['hour'] <= $tc_hour_2)
		{
		    // between [0 to tc2]
		    if(($ts_parts['hour'] >= 0) AND ($tc_hour_1 >= $ts_parts['hour']))
		    {
			echo $tc_hour_1."-".$tc_hour_2."\n";
			$task->time_condition_id = $tc[$t]['id'];
			return 1;
		    }
		}
		
		if($ts_parts['hour'] >= $tc_hour_2)
		{
		    // between [tc1 to 24]
		    if(($ts_parts['hour'] < 24) AND ($tc_hour_1 <= $ts_parts['hour']))
		    {
			echo $tc_hour_1."-".$tc_hour_2."\n";
			$task->time_condition_id = $tc[$t]['id'];
			return 1;
		    }
		}
	    }
	}
	$t++;
    }
    return 0;
 }

/* function find_rate($process,$task)
 {
    global $dbconn;
    //global $DAYS;

    if($task->bill_plan_id) $buf = find_bill_plan($task->bill_plan_id);
    else return 0;

    echo $task->tid.",".$process->calling_number.",".$process->called_number."\n";

    $p=0;
    while($buf[$p]['tariff_id'])
    {
	if(compare_prefix($buf[$p]['prefix'],$process->called_number))
	{
	    $task->prefix_id = $buf[$p]['prefix_id'];
	    break;
	}
	$p++;
    }
    
    $p = 0;
    while($buf[$p]['rate_id'])
    { 
	if($buf[$p]['prefix_id'] == $task->prefix_id)
	{
	    $ts_parts = get_ts($process);
	
	    /* Time Conditions */
//	    $tc = find_time_condition($buf[$p]['rate_id']);
//	    $tc_flag = compare_tc_ts($task,$tc,$ts_parts);
	    /* Time Conditions End */
/*	
	    if($tc_flag)
	    {
		$task->tariff_id = $buf[$p]['tariff_id'];
		//$task->prefix_id = $buf[$p]['prefix_id'];
		$task->rate_id = $buf[$p]['rate_id'];
		$task->free_billsec_id = $buf[$p]['free_billsec_id'];
		break;
	    }
	}
	$p++;
    }
 }*/

 function get_calc_functions($task)
 {
    global $dbconn;
    $CALC = NULL;
    
    if($task->tariff_id)
    {
	$query = "select id,pos,delta_time,fee,iterations
                  from calc_function
                  where 
                  tariff_id = ".$task->tariff_id."
                  order by pos
                 ";
	$result = pg_query($dbconn,$query);
	$pos=0;
	while($row = pg_fetch_row($result))
	{
	    $pos = $row[1];
	    $CALC[$pos]['id'] = $row[0];
	    $CALC[$pos]['delta_time'] = $row[2];
	    $CALC[$pos]['fee'] = $row[3];
	    $CALC[$pos]['iterations'] = $row[4];
	}
    }
    return $CALC; 
 }
 
 function get_calc_functions_v2($tid)
 {
    global $dbconn;
    
    $arr = NULL;
    
    if($tid) {
		$query = "select id,pos,delta_time,fee,iterations
                  from calc_function where tariff_id = ".$tid." order by pos";
		
		$result = pg_query($dbconn,$query);
	
		$i=0;
		while($row = pg_fetch_row($result)) {
			$arr[$i]['id']  = $row[0];
			$arr[$i]['pos'] = $row[1];
			$arr[$i]['delta_time'] = $row[2];
			$arr[$i]['fee'] = $row[3];
			$arr[$i]['iterations'] = $row[4];
			
			$i++;
		}
    }
    
    return $arr; 
 }
 
function get_free_billsec()
{
    global $dbconn;

	$arr = NULL;

	$query = "select * from free_billsec order by free_billsec";
	$result = pg_query($dbconn,$query);
	
	$i=0;
	while($row = pg_fetch_row($result)) {
	    $arr[$i]['id'] = $row[0];
	    $arr[$i]['free_billsec'] = $row[1];
	    
		$i++;
	}

    return $arr;
}
 
 function get_free_billsec_v2($tid)
 {
    global $dbconn;
    
    $arr = NULL;
    
	$query = "select tariff.free_billsec_id,free_billsec.free_billsec from tariff,free_billsec where tariff.id = ".$tid." and tariff.free_billsec_id = free_billsec.id;";
	$result = pg_query($dbconn,$query);

	while($row = pg_fetch_row($result)) {
	    $arr['free_billsec_id'] = $row[0];
	    $arr['free_billsec'] = $row[1];	    
    }
    
    return $arr;
 }
 
function get_free_billsec_id($free_billsec)
{
    global $dbconn;
    
    $id = 0;
    
    $query = "select id from free_billsec where free_billsec = ".$free_billsec.";";
    $result = pg_query($dbconn,$query);
    while($row = pg_fetch_row($result)) {
		$id = $row[0];
    }
    
    return $id;
}

 function get_free_bill($process,$task)
 {
    global $dbconn;
    $free_billsec = 0;
    $query = "select call_billsec,call_price 
              from rating 
              where 
              billing_account_id = ".$task->billing_account_id." and
              call_price < 0";
    $result = pg_query($dbconn,$query);
    while($row = pg_fetch_row($result))
    {
	//$process->free_billsec = $row[0];
	$free_billsec = $free_billsec + $row[0];
    }
    return $free_billsec;
 }

 function get_calling_number_cdrs()
 {
    global $dbconn;

    $query = "select a.uniqueid 
	      from cdr_ast as a 
	      where a.accountcode =''";
    $result = pg_query($dbconn,$query);
    $k=0;
    while($row = pg_fetch_row($result))
    {
	$cdr_uniqueid = $row[0];
	$query2 = "select q.id 
                   from cdr_queue_calling_number as q,cdr_ast as a 
                   where q.uniqueid = a.uniqueid and 
                   a.uniqueid = '".$cdr_uniqueid."'";
	$result2 = pg_query($dbconn,$query2);
	$buf = 0;
	while($row2 = pg_fetch_row($result2))
	{
	    $buf = $row2[0];
	}
	
	if($buf == 0) 
	{
	    $cdrs[$k] = $cdr_uniqueid;
	    $k++;
	}
    }
    return $cdrs; 
 }

 function get_accountcode_cdrs()
 {
    global $dbconn;
    
    $query = "select a.uniqueid 
	      from cdr_ast as a 
	      where a.accountcode !=''";
    $result = pg_query($dbconn,$query);
    $k=0;
    while($row = pg_fetch_row($result))
    {
	$cdr_uniqueid = $row[0];
	$query2 = "select q.id 
                   from cdr_queue_accountcode as q,cdr_ast as a 
                   where q.uniqueid = a.uniqueid and 
                   a.uniqueid = '".$cdr_uniqueid."'";
	$result2 = pg_query($dbconn,$query2);
	$buf = 0;
	while($row2 = pg_fetch_row($result2))
	{
	    $buf = $row2[0];
	}
	
	if($buf == 0) 
	{
	    $cdrs[$k] = $cdr_uniqueid;
	    $k++;
	}
    }
    return $cdrs; 
 }

/* function get_norating_cdr($rating_mode_id)
 {

    if($rating_mode_id == 1)
    {
	$cdrs = get_calling_number_cdrs();
    }

    if($rating_mode_id == 2)
    {
	$cdrs = get_accountcode_cdrs();
    }

    return $cdrs;
 }*/
 
 function check_bill($bid,$_leg,$time1,$time2)
 {
    global $dbconn;
    $leg = "leg_".$_leg;
    $bill = 0;
    /*$query = "select sum(rt.call_price) 
              from rating as rt,cdrs 
              where rt.billing_account_id = ".$bid." and 
              cdrs.id = rt.call_id and 
              cdrs.ts >= '".$time1." 00:00:00' and cdrs.ts < '".$time2." 00:00:00' and
              cdrs.leg_a = rt.id and
              rt.call_price > 0
             ";*/
    $query = "select sum(rt.call_price) 
              from rating as rt,cdrs 
              where rt.billing_account_id = ".$bid." and 
              cdrs.ts >= '".$time1." 00:00:00' and cdrs.ts < '".$time2." 00:00:00' and
              cdrs.".$leg." = rt.id and
              rt.call_price > 0
             ";
    $result = pg_query($dbconn,$query);
    while($row = pg_fetch_row($result))
    {
	$bill = $row[0];
    }
    return $bill;
 }

 function check_bill_tariff($bid,$_leg,$time1,$time2)
 {
    global $dbconn;
    $leg = "leg_".$_leg;
    //$bill = 0;
    $query = "select sum(rt.call_price) 
              from rating as rt,cdrs 
              where rt.billing_account_id = ".$bid." and 
              cdrs.ts >= '".$time1." 00:00:00' and cdrs.ts < '".$time2." 00:00:00' and
              cdrs.".$leg." = rt.id and
              rt.call_price > 0
             ";
    $result = pg_query($dbconn,$query);
    while($row = pg_fetch_row($result))
    {
	$bill = $row[0];
    }
    //return $bill;
 }


 function check_free_bill($bid,$time1,$time2)
 {
    global $dbconn;
    $bill = 0;
    $query = "select sum(rt.call_price) 
              from rating as rt,cdrs 
              where rt.billing_account_id = ".$bid." and 
              cdrs.id = rt.call_id and 
              cdrs.ts >= '".$time1." 00:00:00' and cdrs.ts < '".$time2." 00:00:00' and
              rt.call_price < 0
             ";
    $result = pg_query($dbconn,$query);
    while($row = pg_fetch_row($result))
    {
	$bill = $row[0];
    }
    return $bill;
 }

 function get_tc_name($tc_id)
 {
    global $dbconn;
    $tc_name = NULL;
    $query = "select tc_df.tc_name
              from time_condition_deff as tc_df,time_condition as tc
              where
              tc_df.id = tc.time_condition_id and
              tc.id = ".$tc_id."";
    echo $query."\n";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$tc_name = $row[0];
    }
    return $tc_name;
 }
 
 function get_tc_name_2($tc_id)
 {
    global $dbconn;
    $tc_name = NULL;
    $query = "select tc_df.tc_name
              from time_condition_deff as tc_df
              where
              tc_df.id = ".$tc_id."";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$tc_name = $row[0];
    }
    return $tc_name;
 }
 
function get_time_condition_deff()
{
    global $dbconn;
    
    $arr = NULL;
    
    $query = "select * from time_condition_deff order by id";
    $res = pg_query($dbconn,$query);
    
    $i = 0;
    while($row = pg_fetch_row($res)) {
		$arr[$i]['id']    = $row[0];
		$arr[$i]['hours'] = $row[1];
		$arr[$i]['days_week'] = $row[2];
		$arr[$i]['tc_name'] = $row[3];
		$arr[$i]['tc_date'] = $row[4];
		$arr[$i]['year']  = $row[5];
		$arr[$i]['month'] = $row[6];
		$arr[$i]['day_month'] = $row[7];
    
    	$i++;
    }

    return $arr;
}
 
 function get_tariff_name($bid)
 {
    global $dbconn;
    $arr[0] = NULL;
    
    $query = "select tr.name 
              from 
              rating as rt,rate,tariff as tr 
              where rt.billing_account_id = ".$bid." and 
              rt.rate_id = rate.id and 
              tr.id = rate.tariff_id 
              group by tr.name";
    $res = pg_query($dbconn,$query);
    $i=0;
    while($row = pg_fetch_row($res))
    {
	$arr[$i] = $row[0];
	$i++;
    }
    return $arr;
 }
 
 function get_tariff_stat($bid,$_leg,$time1,$time2,$tariff)
 {
    global $dbconn;
 
    $query = "select count(rt.id) as calls,sum(rt.call_price) as leva,sum(rt.call_billsec) as mins
              from rating as rt,cdrs as cd,tariff as tr,rate
              where tr.name = '".$tariff."' and 
              rt.billing_account_id = ".$bid." and 
              rate.id = rt.rate_id and 
              rate.tariff_id = tr.id and
              cd.leg_".$_leg." = rt.id and 
              cd.ts < '".$time2." 00:00:00' and 
              cd.ts >= '".$time1." 00:00:00'";

    $result = pg_query($dbconn,$query);
    while($row = pg_fetch_row($result))
    {
	echo $tariff.",".$row[0].",".$row[1].",".round(($row[2]/60),2)."\n";
    }

 }
 
 function get_rating_calls($bid,$_leg,$time1,$time2)
 {
    global $dbconn;
    $leg = "leg_".$_leg;
    /*$query = "select cd.call_uid,cd.ts,cd.calling_number,cd.called_number,rt.call_price,rt.call_billsec,tr.name,
              rate.id,rt.time_condition_id
              from rating as rt,cdrs as cd,tariff as tr,rate
              where rt.billing_account_id = ".$bid." and 
              rate.id = rt.rate_id and
              rate.tariff_id = tr.id and
              cd.id = rt.call_id and 
              cd.ts < '".$time2." 00:00:00' and 
              cd.ts >= '".$time1." 00:00:00' 
              order by cd.ts
             ";*/
    $query = "select cd.call_uid,cd.ts,cd.calling_number,cd.called_number,rt.call_price,rt.call_billsec,tr.name,
              rate.id,rt.time_condition_id
              from rating as rt,cdrs as cd,tariff as tr,rate
              where rt.billing_account_id = ".$bid." and 
              rate.id = rt.rate_id and
              rate.tariff_id = tr.id and
              cd.".$leg." = rt.id and 
              cd.ts < '".$time2." 00:00:00' and 
              cd.ts >= '".$time1." 00:00:00' 
              order by cd.ts
             ";
    $result = pg_query($dbconn,$query);
    while($row = pg_fetch_row($result))
    {
	$tc_name = NULL;
	$tc_name = get_tc_name_2($row[8]);
	echo $row[0].",".$row[1].",".$row[2].",".$row[3].",".$row[4].",".$row[5].",".$row[6].",".$tc_name."[".$row[8]."]"."\n";
    }
 }

 function get_norating_calls($bid,$time1,$time2)
 {
    global $dbconn;
    $query = "select cd.call_uid,cd.ts,cd.calling_number,cd.called_number,rt.call_price,rt.call_billsec,tr.name 
              from rating as rt,cdrs as cd,tariff as tr,rate
              where rt.billing_account_id = ".$bid." and 
              rate.id = rt.rate_id and
              rate.tariff_id = tr.id and
              cd.id = rt.call_id and 
              cd.ts < '".$time2." 00:00:00' and 
              cd.ts >= '".$time1." 00:00:00' 
              order by cd.ts
             ";
    $result = pg_query($dbconn,$query);
    while($row = pg_fetch_row($result))
    {
	echo $row[0].",".$row[1].",".$row[2].",".$row[3].",".$row[4].",".$row[5].",".$row[6]."\n";
    }
 }
 
 function clear_rating($bid,$time_1,$time_2)
 {
    global $dbconn;
    $query = "delete from rating where billing_account_id = ".$bid."";
 }
 
 function get_norating_cdr($leg)
 {
    global $dbconn;
    $query = "SELECT id from cdrs where ".$leg." = 0 order by ts";
    $result = pg_query($dbconn,$query);
    $i=0;
    while($row = pg_fetch_row($result))
    {
	$cdr_id[$i] = $row[0];
	$i++;
    }
    return $cdr_id;
 }
 

 function write_rating_in_db($process,$task)
 {
    global $dbconn;
    $id = 0;
    $query = "insert into rating 
              (rate_id,call_price,call_billsec,call_id,billing_account_id,rating_mode_id,time_condition_id)
              values 
              (".$task->rate_id.",".$task->call_price.",".$task->call_billsec.",".$process->call_id.",
              ".$task->billing_account_id.",".$task->rating_mode_id.",".$task->time_condition_id.")";
    pg_query($dbconn,$query);
    
    $query2 = "select id
               from rating
               where 
               rate_id = ".$task->rate_id." and
               call_price = ".$task->call_price." and 
               call_billsec = ".$task->call_billsec." and 
               call_id = ".$process->call_id." and
               billing_account_id = ".$task->billing_account_id." and
               rating_mode_id = ".$task->rating_mode_id."";
    $res = pg_query($dbconn,$query2);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }
 
 function write_balance($balance)
 {
    global $dbconn;
    $query = "update balance 
              set amount = ".$balance->Amount.",last_update = '".$balance->LastUpdate."'
              where id = ".$balance->BalanceID."";
    if($balance->BalanceID) pg_query($dbconn,$query); 
 }

 function insert_cdrs_into($cdr)
 {
    global $dbconn;
    $i = 0;$c = 0;
    while($cdr[$i]['call_uid'])
    {
	$id = 0;
	$query = "select id from cdrs where call_uid = '".$cdr[$i]['call_uid']."'";
	$res = pg_query($dbconn,$query);
	while($row = pg_fetch_row($res))
	{
	    $id = $row[0];
	}
	
	$query = "insert into cdrs
              (call_uid,calling_number,called_number,dst_context,billsec,account_code,ts,server_name,src_context,src,dst)
              values 
              ('".$cdr[$i]['call_uid']."','".$cdr[$i]['calling_number']."','".$cdr[$i]['called_number']."',
              '".$cdr[$i]['dst_context']."','".$cdr[$i]['billsec']."','".$cdr[$i]['accountcode']."','".$cdr[$i]['ts']."',
              '".$cdr[$i]['server_name']."','".$cdr[$i]['userfield']."','".$cdr[$i]['src']."','".$cdr[$i]['dst']."')";
	if($id == 0) { pg_query($dbconn,$query); $c++;}
	$i++;
    }
    echo "\n\t$c CDRs are inputed in the DB!!!\n\n";
 }

 function insert_cdrs_into_2($cdr)
 {
    global $dbconn;
    $i = 0;$c = 0;
    while($cdr[$i]['call_uid'])
    {
	$id = 0;
	$query = "select id from cdrs where call_uid = '".$cdr[$i]['call_uid']."'";
	$res = pg_query($dbconn,$query);
	while($row = pg_fetch_row($res))
	{
	    $id = $row[0];
	}
	
	$query = "insert into cdrs
              (call_uid,calling_number,called_number,dst_context,billsec,account_code,ts,server_name,src_context,src,dst,cdr_server_id)
              values 
              ('".$cdr[$i]['call_uid']."','".$cdr[$i]['calling_number']."','".$cdr[$i]['called_number']."',
              '".$cdr[$i]['dst_context']."','".$cdr[$i]['billsec']."','".$cdr[$i]['accountcode']."','".$cdr[$i]['ts']."',
              '".$cdr[$i]['server_name']."','".$cdr[$i]['userfield']."','".$cdr[$i]['src']."','".$cdr[$i]['dst']."',".$cdr[$i]['cdr_server_id'].")";
	if($id == 0) { pg_query($dbconn,$query); $c++;}
	$i++;
    }
    echo "\n\t$c CDRs are inputed in the DB!!!\n\n";
 }
 
 
 function mark_cdr($tid)
 {
    global $dbconn;
    $query = "insert into cdr_queue_accountcode 
              (id,uniqueid)
              values (1,'".$tid."')
             ";
    pg_query($dbconn,$query);
 }

 function insert_bill_plan($bill_plan,$start_period,$end_period)
 {
    global $dbconn;
    $query = "insert into bill_plan 
              (name,start_period,end_period)
              values ('".$bill_plan."',".$start_period.",".$end_period.")
             ";
    pg_query($dbconn,$query);
 }

 function insert_bill_plan_v2($bill_plan)
 {
    global $dbconn;
    $query = "insert into bill_plan (name) values ('".$bill_plan."')";
    pg_query($dbconn,$query);
    
    return get_bill_plan_id($bill_plan);
 }

 function insert_bill_tree($bill_plan_id,$root_bplan_id)
 {
    global $dbconn;
    $query = "insert into bill_plan_tree
              (bill_plan_id,root_bplan_id)
              values (".$bill_plan_id.",".$root_bplan_id.")";
    pg_query($dbconn,$query);
 }

 function insert_credit_card($bacc,$amount)
 {
    global $dbconn;
    $query = "insert into credit_card (billing_account_id,amount) values (".$bacc.",".$amount.")";
    pg_query($dbconn,$query);
    return get_credit_card_id($bacc,$amount);
 }

 function get_credit_card_id($bacc,$amount)
 {
    global $dbconn;
    $id = 0;
    $query = "select id 
              from credit_card 
              where 
              billing_account_id = ".$bacc." and
              amount = ".$amount."";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }

 function get_balance_id($bacc,$debit,$credit)
 {
    global $dbconn;
    $id = 0;
    $query = "select id 
              from balance
              where 
              billing_account_id = ".$bacc." and
              debit_card_id = ".$debit." and
              credit_card_id = ".$credit."
              ";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }


 function insert_debit_card($bacc,$amount,$start,$end)
 {
    global $dbconn;
    $query = "insert into debit_card 
              (billing_account_id,amount,start_date,end_date,last_update) 
              values (".$bacc.",".$amount.",'".$start."','".$end."','now()')";
    pg_query($dbconn,$query);
    return get_debit_card_id($bacc,$amount,$start,$end);
 }

 function get_debit_card_id($bacc,$amount,$start,$end)
 {
    global $dbconn;
    $id = 0;
    $query = "select id 
              from debit_card 
              where 
              billing_account_id = ".$bacc." and
              amount = ".$amount." and
              start_date = '".$start."' and end_date = '".$end."'
              ";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    return $id;
 }

 function insert_balance($bacc,$debit,$credit)
 {
    global $dbconn;
    $query = "insert into balance (billing_account_id,debit_card_id,credit_card_id,last_update) 
              values (".$bacc.",".$debit.",".$credit.",'now()')";
    pg_query($dbconn,$query);
 }

function insert_tariff($bill_plan_id,$tariff,$start,$end,$free_billsec_id)
{
    global $dbconn;
    
    $query = "insert into tariff (name,temp_id,start_period,end_period,free_billsec_id) values ('$tariff',$bill_plan_id,$start,$end,$free_billsec_id)";
    pg_query($dbconn,$query);

	return get_tariff_id($bill_plan_id,$tariff);
}

 function insert_billing_account($billing_account,$curr_id,$leg,$cdr_server_id)
 {
    global $dbconn;
    
    $query = "insert into billing_account
              (username,currency_id,leg,cdr_server_id)
              values ('".$billing_account."',".$curr_id.",'".$leg."',".$cdr_server_id.")";
    pg_query($dbconn,$query);
 
 	return get_billing_account_id($billing_account);
 }

function insert_rate($bill_plan_id,$prefix_id,$tariff_id)
{
	global $dbconn;
    
	$query = "insert into rate (bill_plan_id,prefix_id,tariff_id) 
              values (".$bill_plan_id.",".$prefix_id.",".$tariff_id.")";
	pg_query($dbconn,$query);
 
	return get_rate_id($bill_plan_id,$prefix_id,$tariff_id);
}
/*
 function insert_free_billsec($free_billsec,$tariff_id)
 {
    global $dbconn;
    $query = "insert into free_billsec
              (free_billsec,tariff_id)
              values 
              (".$free_billsec.",".$tariff_id.")
             ";
    pg_query($dbconn,$query);
 }
*/
function insert_free_billsec($free_billsec)
{
    global $dbconn;
    
    $query = "insert into free_billsec (free_billsec) values (".$free_billsec.")";
    
    pg_query($dbconn,$query);
    
    return get_free_billsec_id($free_billsec);
}

 function insert_pcard($bacc_id,$pcard_tp,$pcard_sts,$amount,$start_date,$end_date)
 {
    global $dbconn;
    
    $query = "insert into pcard (billing_account_id,pcard_status_id,pcard_type_id,amount,start_date,end_date,last_update) 
              values (".$bacc_id.",".$pcard_sts.",".$pcard_tp.",".$amount.",'".$start_date."','".$end_date."','now()')";
    pg_query($dbconn,$query);
    
    return get_pcard_id($bacc_id,$pcard_tp,$pcard_sts,$amount,$start_date,$end_date);
 }

 function insert_time_condition($tariff_id,$time_zone)
 {
    global $dbconn;
    $id = 0;
    $query = "select id from time_condition_deff where tc_name = '".$time_zone."'";
    $res = pg_query($dbconn,$query);
    while($row = pg_fetch_row($res))
    {
	$id = $row[0];
    }
    if($id)
    {
	$query = "insert into time_condition 
	          (tariff_id,time_condition_id)
	          values (".$tariff_id.",".$id.")";
	pg_query($dbconn,$query);
    }
    else
    {
	echo "Error\n";
	exit;
    }
 }

function insert_time_condition_deff($hours,$days_week,$tc_name,$tc_date,$year,$month,$day_month)
{
	global $dbconn;
	
	/* hours,days_week,tc_name,tc_date,year,month,day_month */ 
	
	$query = "insert into time_condition_deff (hours,days_week,tc_name,tc_date,year,month,day_month) 
			  values('$hours','$days_week','$tc_name','$tc_date','$year','$month','$day_month');";
	pg_query($dbconn,$query);
	
	return get_time_condition_deff_id($tc_name);
}

function insert_prefix($prefix,$desc)
{
	global $dbconn;
    
    if(empty($desc)) {
		$query = "insert into prefix (prefix) values ('".$prefix."')";
    } else {
		$query = "insert into prefix (prefix,comm) values ('".$prefix."','".$desc."')";
    }
    
    pg_query($dbconn,$query);

	return get_prefix_id($prefix);
}

function insert_rating_account($rating_mode,$rating_account,$billing_account_id,$bill_plan_id)
{
	global $dbconn;
    
    $query = "insert into ".$rating_mode." 
              (".$rating_mode.",billing_account_id) 
              values ('".$rating_account."',".$billing_account_id.")";
    pg_query($dbconn,$query);
    
    $id = get_rating_account_id($rating_mode,$rating_account);
    
    $query = "insert into ".$rating_mode."_deff 
              (".$rating_mode."_id,bill_plan_id)
              values
              (".$id.",".$bill_plan_id.")";
    
    if($id) pg_query($dbconn,$query);
    else echo "Don't have rating_account_id ...\n"; 
    
    return $id;
}

function insert_rating_account_2($rating_mode,$rating_account,$billing_account_id,$bill_plan_id,$clg_nadi,$cld_nadi)
{
    global $dbconn;
    
    $query = "insert into ".$rating_mode." 
              (".$rating_mode.",billing_account_id) 
              values ('".$rating_account."',".$billing_account_id.")";
    pg_query($dbconn,$query);
    
    $id = get_rating_account_id($rating_mode,$rating_account);
    
    $query = "insert into ".$rating_mode."_deff 
              (".$rating_mode."_id,bill_plan_id,clg_nadi,cld_nadi)
              values
              (".$id.",".$bill_plan_id.",".$clg_nadi.",".$cld_nadi.")";
    if($id) pg_query($dbconn,$query);
    else echo "Don't have rating_account_id ...\n"; 
 
 	return $id;
}

function insert_calc_func($tariff_id,$pos,$fee,$delta_time,$iterations)
{
    global $dbconn;

    if(empty($iterations)) {
		$query = "insert into calc_function
                  (tariff_id,pos,fee,delta_time)
                  values 
                  (".$tariff_id.",".$pos.",".$fee.",".$delta_time.")";
    } else {
		$query = "insert into calc_function
                  (tariff_id,pos,fee,delta_time,iterations)
                  values 
                  (".$tariff_id.",".$pos.",".$fee.",".$delta_time.",".$iterations.")
                  ";
    }
    
    pg_query($dbconn,$query);

	return get_calc_id($tariff_id,$pos);
}

 function update_rating_id_cdrs($cid,$leg,$rating_id)
 {
    global $dbconn;
    $query = "update cdrs
              set ".$leg." = ".$rating_id."
              where id = ".$cid."
             ";
    pg_query($dbconn,$query);
 }

function update_cdr_server_id($old,$new)
{
	global $dbconn;
	
	return pg_query($dbconn,"update billing_account set cdr_server_id = $new where cdr_server_id = $old");	
}

?>
