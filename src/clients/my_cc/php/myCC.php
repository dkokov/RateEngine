<?php

 include("lib.mycc.php");
 
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
		myCC_balance($argv[2]);
		$i++;
	}
 } else if($argv[1] == "maxsec") {
    echo myCC_maxsec($argv[2],$argv[3],$argv[4])."\n";

 } else if(strncmp($argv[1],"test1",5) == 0) {

	$clg = NULL;
	$cld = NULL;

	$mode = 1;

	if($argv[1] == "test1b") $mode = 2;
	
	if(!empty($argv[4])) $n = $argv[4];
	
	$fp = fopen($argv[2],"r");
	if ($fp) {
		$i=0;
		while(($buffer = fgets($fp, 4096)) !== false) {
			if($buffer != NULL) {
				$clg[$i] = trim($buffer,"\n");
				$i++;
			}
		}
	}
	fclose($fp);

	$fp = fopen($argv[3],"r");
	if ($fp) {
		$i=0;
		while(($buffer = fgets($fp, 4096)) !== false) {
			$buffer = trim($buffer,"\n");
			if(strlen($buffer) > 0) {
				$cld[$i] = $buffer;
				$i++;
			}
		}
	}
	fclose($fp);
	
	if((!empty($clg))AND(!empty($cld))) myCC_test1($mode,$clg,$cld,$n);
	
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
	 $q = $n/$t;
	 echo "q: $q req/s\n";
 }
?>
