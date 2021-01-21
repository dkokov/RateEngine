<?php
/*
 * MyCC PHP client,version 0.1 (2017-08-22)
 * 
 * - add 'myCC_test1()',version 0.1.1 (2018-04-24)
*/

define("DEBUG",false);

define("PORT1",9091);
define("IP1",'127.0.0.1');

define("PORT2",9090);
define("IP2",'127.0.0.2');

define("ID",1);

define("MYCC_GEN_ERR",-999);
define("MYCC_NO_RESPONSE",-998);

define("TCP_SOCKET_ERR",-997);
define("TCP_CONN_ERR",-996);

function myCC_client($comm,$clg,$cld,$uuid,$billsec,$duration)
{
    $write_buf = "";
    $tid = 1;
    $ts = time();

    $ip = IP1;
    $port = PORT1;

    $write_buf .= ID.",".$tid.",".$comm.",".$ts;

    if($comm == "maxsec") $write_buf .= ",".$clg.",".$cld.",".$uuid;
    else if($comm == "term") $write_buf .= ",nc,".$uuid.",".$billsec.",".$duration;
    else if($comm == "balance") $write_buf .= ",".$clg;

    $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    if ($socket === false) {
	echo "create socket is not unsiccessful\n";
	
	return TCP_SOCKET_ERR;
    }

    socket_set_option($socket, SOL_SOCKET, SO_RCVTIMEO, array('sec'=>1, 'usec'=>500000));
    socket_set_option($socket, SOL_SOCKET, SO_SNDTIMEO, array('sec'=>1, 'usec'=>500000));

    $reply = 1;

    loop_conn:
    $result = socket_connect($socket, $ip, $port);
    if ($result === false) {
	echo "connect to $ip:$port is not unsuccessful\n";
	
	if($reply == 1) {
	    $ip = IP2;
	    $port = PORT2;
	
	    $reply++;
	
	    goto loop_conn;
	} else {
	    socket_close($socket);
	    return TCP_CONN_ERR;
	}
    }

    $reply = 1;
    loop_write_read:
    if(DEBUG) echo "[$reply]write buffer: $write_buf\n";
    socket_write($socket, $write_buf, strlen($write_buf));

    $read_buf = "";
    $read_buf = socket_read($socket, 256);
    if(DEBUG) echo "[$reply]read buffer: $read_buf\n";

    socket_close($socket);

    if(strlen($read_buf)) {
	$arr = explode(",",$read_buf);
	$ret = $arr[2];
    } else {
	if($reply > 1) return MYCC_NO_RESPONSE;
	else {
	    $reply++;
	    goto loop_write_read;
	}
    }

    if($ret == 'ok') return 0;
    else if(($ret == 'nok')OR($ret == 'error')OR($ret == 'empty')) return -9;
    else return $ret;
}

function myCC_maxsec($clg,$cld,$uuid)
{
    return myCC_client("maxsec",$clg,$cld,$uuid,0,0);
}

function myCC_status()
{
    return myCC_client("status","","","",0,0);
}

function myCC_balance($clg)
{
    return myCC_client("balance",$clg,"","",0,0);
}

function myCC_term($uuid,$billsec,$duration)
{
    return myCC_client("term","","",$uuid,$billsec,$duration);
}

function myCC_test1($mode,$clg,$cld,$n)
{
	$clg_num = count($clg); 
	$cld_num = count($cld);

	$i = 0;
	while($i < $n) {
		$uuid = mt_rand(1000,9999).'-test-'.mt_rand(100000,999999).'-dddd-'.mt_rand(1000,9999);
		
		if(($mode == 2)AND($i > 0)) goto next_step;
		$c = mt_rand(0,$clg_num);
		
		next_step:
		$d = mt_rand(0,$cld_num);

		$maxsec = 0;
		
		if((!empty($clg[$c]))AND(!empty($cld[$d]))) { 
			$tt1 = gettimeofday();

			$maxsec = myCC_maxsec($clg[$c],$cld[$d],$uuid);
			
			$tt2 = gettimeofday();

			$sec = $tt2['sec'] - $tt1['sec'];

			if($tt2['usec'] > $tt1['usec']) $usec = $tt2['usec'] - $tt1['usec'];
			else $usec = $tt1['usec'] - $tt2['usec'];

			$t = $sec*1000 + $usec/1000;
			echo "$i,t: $t ms\n";
		}
		
		if($maxsec > 0) {
			if($maxsec >= 5) {
				$dur  = mt_rand(5,$maxsec);
				$bill = $dur - 5;
			} else {
				$dur  = $maxsec;
				$bill = $dur;
			}
						
			myCC_term($uuid,$bill,$dur);
			
			echo $i.",".$uuid.",".$clg[$c].",".$cld[$d].",".$maxsec."\n";
		}
		
		if($mode == 2) sleep(1);

		$i++;
	}
}
?>
