<?php
/*
 * MyCC PHP client,version 0.1 (2017-08-22)
 * - add 'strest test 1',2017-11-09
 * - add 'status_loop',2017-11-13
 * - add 'balance loop',2017-11-13
 * - edit 'loop' option to use 'end value'(not with end loop) and show 'avg speed/performance',2018-01-16
 * - add 'test2' loop,2019-10-16
*/

include("lib.mycc.php");

function status_loop()
{
    $pid = pcntl_fork();
    if ($pid == -1) {
	die('could not fork');
    } else if ($pid) {
	pcntl_wait($status);
    } else {
	myCC_status();
	posix_kill(posix_getpid(), SIGTERM); 
    }
}

 error_reporting(E_ERROR);
 
 $n = 1000000;
 
 $tt1 = gettimeofday();

 if($argv[1] == "status") {
      echo myCC_status()."\n";
 } else if($argv[1] == "loop") {
	$i=0;
    while($i<=$n) {
		myCC_status();	
		$i++;
	}
 } else if($argv[1] == "loopb") {

	$i=0;
    while($i<=$n) {
		myCC_balance("359996070791");
		$i++;
	}
 } else if($argv[1] == "maxsec") {
    echo myCC_maxsec($argv[2],$argv[3],$argv[4])."\n";
 } else if($argv[1] == "test1") {
    again:
    $arr = NULL;
    for($c=0;$c<10;$c++) {
		$arr[$c] = "cc-".mt_rand()."-".mt_rand();
		echo MyCC_maxsec("35999606083".$c,"35924119998",$arr[$c])."\n";
    }

    for($c=0;$c<10;$c++) {
		echo MyCC_term($arr[$c],65,75)."\n";
    }
    if(!empty($argv[2])) usleep($argv[2]);
    else sleep(1);
    goto again;
 } else if($argv[1] == "test2") {
    again2:
    $arr = NULL;
    for($c=0;$c<10;$c++) {
        $arr[$c] = "call_uuid-".mt_rand()."-".mt_rand();
        $res = MyCC_maxsec("35999606083".$c,"35924119998",$arr[$c]);
        if(!empty($argv[3])) echo $res."\n";
        if(($res <= -990)&&($res >= -999)) die;
        if(!empty($argv[2])) usleep($argv[2]);
        $res = MyCC_term($arr[$c],0,0);
        if(!empty($argv[2])) echo $res."\n";
        if(($res <= -990)&&($res >= -999)) die;
    }
    if(!empty($argv[3])) usleep($argv[3]);
    goto again2;
 } else if($argv[1] == "term") {
    echo myCC_term($argv[2],$argv[3],$argv[4])."\n";
 } else if($argv[1] == "balance") {
    echo myCC_balance($argv[2])."\n";
 } else {
    echo "Invalid command!\n";
 }

 $tt2 = gettimeofday();

 $sec = $tt2['sec'] - $tt1['sec'];

 if($tt2['usec'] > $tt1['usec']) $usec = $tt2['usec'] - $tt1['usec'];
 else $usec = $tt1['usec'] - $tt2['usec'];

 $t = $sec*1000 + $usec/1000;
 echo "t: $t ms\n";

 if($i) {
	 $q = 1000000000/$t;
	 echo "q: $q req/s\n";
 }
?>
