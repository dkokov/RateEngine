<?php
/*
 * MyCC PHP client,version 0.1 (2017-08-22)
 * 
 * - add 'myCC_test1()',version 0.1.1 (2018-04-24)
*/

define("DEBUG",true);

define("PORT1",9999);
define("IP1",'127.0.0.1');
//define("IP1",'::1');


define("PORT2",9090);
define("IP2",'127.0.0.2');

define("ID",1);

define("MYCC_GEN_ERR",-999);
define("MYCC_NO_RESPONSE",-998);

define("TCP_SOCKET_ERR",-997);
define("TCP_CONN_ERR",-996);

//function myCC_client($comm,$clg,$cld,$uuid,$billsec,$duration)
//{
    $write_buf = "";
    $tid = 1;
    $ts = time();

    $ip = IP1;
    $port = PORT1;

/*    $write_buf .= ID.",".$tid.",".$comm.",".$ts;

    if($comm == "maxsec") $write_buf .= ",".$clg.",".$cld.",".$uuid;
    else if($comm == "term") $write_buf .= ",nc,".$uuid.",".$billsec.",".$duration;
    else if($comm == "balance") $write_buf .= ",".$clg;
*/

    $write_buf = '{"jsonrpc": "2.0","method":"maxsec","params":{"call-uid":"1112","clg":"359996666199","cld":"35924119460"},"id":1}';


//    $socket = socket_create(AF_UNIX, SOCK_STREAM, 0);
//    $socket = socket_create(AF_INET6, SOCK_STREAM, SOL_TCP);
    $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
//    $socket = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
    if ($socket === false) {
	echo "create socket is not unsiccessful\n";
	
//	return TCP_SOCKET_ERR;
    }

    socket_set_option($socket, SOL_SOCKET, SO_RCVTIMEO, array('sec'=>1, 'usec'=>500000));
    socket_set_option($socket, SOL_SOCKET, SO_SNDTIMEO, array('sec'=>1, 'usec'=>500000));

//    $reply = 1;

//$server_side_sock = "/tmp/test_unix_socket.server";
//if (!socket_bind($socket, $server_side_sock))
//        die("Unable to bind to $server_side_sock");

//    loop_conn:
    $result = socket_connect($socket, $ip, $port);
    if ($result === false) {
	echo "connect to $ip:$port is not unsuccessful\n";
	
/*	if($reply == 1) {
	    $ip = IP2;
	    $port = PORT2;
	
	    $reply++;
	
	    goto loop_conn;
	} else {
	    socket_close($socket);
	    return TCP_CONN_ERR;
	} */
    }

    $reply = 1;
//    loop_write_read:
    if(DEBUG) echo "[$reply]write buffer: $write_buf\n";
    socket_write($socket, $write_buf, strlen($write_buf));

    $read_buf = "";
    $read_buf = socket_read($socket, 256);
    if(DEBUG) echo "[$reply]read buffer: $read_buf\n";

    socket_close($socket);

/*    if(strlen($read_buf)) {
	$arr = explode(",",$read_buf);
	$ret = $arr[2];
    } else {
	if($reply > 1) return MYCC_NO_RESPONSE;
	else {
	    $reply++;
	    goto loop_write_read;
	}
    }
*/
//    if($ret == 'ok') return 0;
//    else if(($ret == 'nok')OR($ret == 'error')OR($ret == 'empty')) return -9;
//    else return $ret;
//}


?>
