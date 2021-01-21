<?php
/*
 * MyCC PHP client - test tool,version 0.1 (2017-08-22)
 * - add 'strest test 1',2017-11-09
 * - add 'status_loop',2017-11-13
 * - add 'balance loop',2017-11-13
 * - edit 'loop' option to use 'end value'(not with end loop) and show 'avg speed/performance',2018-01-16
 * - add 'test2' loop,2019-10-16
 *
 * MyCC PHP client - test tool,version 0.1.1 (2020-02-12)
 * - add 'help()' function and use in main part
 * - add CALL_ID_PATTERN,MIN_BILLSEC,MAX_BILLSEC,MIN_DURSEC,MAX_DURSEC,LOOP_NUMBERS
 * - add 'main_loop()' function as get 'main code part' by this file
 * - define new option 'test_f' in 'main_loop()'
 */

include("lib.mycc.php");

define("CALL_ID_PATTERN","cc-");
define("MIN_BILLSEC",5);
define("MAX_BILLSEC",3600);
define("MIN_DURSEC",2);
define("MAX_DURSEC",35);

define("LOOP_NUMBERS",100000);

function help()
{
    echo "\nHelp:\n";
    echo "\n php myCC.php [options]";
    echo "\n   options:";
    echo "\n           help                                            ,this display";
    echo "\n           status                                          ,send 'MyCC status' to the 'CC Server'";
    echo "\n           loop                                            ,send 'MyCC status' in the loop to the 'CC Server'";
    echo "\n           balance  [PhoneNumber]                          ,send 'MyCC balance' with argument to the 'CC Server'";
    echo "\n           loopb [PhoneNumber]                             ,send 'MyCC balance' with argument in the loop to the 'CC Server'";
    echo "\n           maxsec [CallingNumber] [CalledNumber] [Call-ID] ,send 'MyCC maxsec' with arguments to the 'CC Server'";
    echo "\n           term  [Call-ID] [billsec] [duration]            ,send 'MyCC term' with arguments to the 'CC Server'";
    echo "\n           test_f [file_name]                              ,get CLG by file and send 'MyCC maxsec','MyCC term' with arguments to the 'CC Server'";
}

function main_loop($argv)
{
    $i = 0;
    $n = LOOP_NUMBERS;

    if($argv[1] == "status") {
		echo myCC_status()."\n";
    } else if($argv[1] == "loop") {
		while($i <= $n) {
			if(myCC_status() < 0) break;
			$i++;
		}
    } else if($argv[1] == "loopb") {
		while($i <= $n) {
			if(myCC_balance($argv[2]) < 0) break;
			$i++;
		}
    } else if($argv[1] == "maxsec") {
		echo "ret(maxsec)=".myCC_maxsec($argv[2],$argv[3],$argv[4])."\n";
    } else if($argv[1] == "test_f") {
		if(strlen($argv[2]) == 0) {
			echo "ERROR! FILENAME(argv[2]) is empty!\n";
			return -1;
		}
		
		if(strlen($argv[3]) == 0) {
			echo "ERROR! CLD(argv[3]) is empty!\n";
			return -1;
		}
		
		$fp = fopen($argv[2],"r");
		if($fp) {
			$call_id = NULL;
			
			$i = 0;
			while(($buffer = fgets($fp,1024)) !== false) {
				$buffer = trim($buffer,"\n");
				
				if(strlen($buffer) > 0) {
					$call_id[$i] = CALL_ID_PATTERN.mt_rand()."-".mt_rand();
					echo "ret(maxsec)=".MyCC_maxsec($buffer,$argv[3],$call_id[$i])."\n";
					
					$i++;
				}
			}
			
			$i = 0;
			while(!empty($call_id[$i])) {
				$billsec = mt_rand(MIN_BILLSEC,MAX_BILLSEC);
				$duration = mt_rand(MIN_DURSEC,MAX_DURSEC);;
				
				echo "term[".$i."],".$call_id[$i].",".$billsec.",".$duration."\n";
				echo "ret(term)=".MyCC_term($call_id[$i],$billsec,$duration)."\n";
				
				$i++;
			}
			
			fclose($fp);
		} else {
			echo "ERROR! File is not opened!\n";
			return -1;
		}
		
		return 0;
    } else if($argv[1] == "test1") {
		again:
		$arr = NULL;
		
		for($c=0;$c<10;$c++) {
			$arr[$c] = "cc-".mt_rand()."-".mt_rand();
			echo MyCC_maxsec(CLG_PATTERN.$c,CLD_PATTERN,$arr[$c])."\n";
		}
		
		for($c=0;$c<10;$c++) {
			echo MyCC_term($arr[$c],BILLSEC,DURATION)."\n";
		}
		
		if(!empty($argv[2])) usleep($argv[2]);
		else sleep(1);
			goto again;
		} else if($argv[1] == "test2") {
			again2:
			$arr = NULL;
			
			for($c=0;$c<10;$c++) {
				$arr[$c] = "call_uuid-".mt_rand()."-".mt_rand();
				$res = MyCC_maxsec(CLG_PATTERN.$c,CLD_PATTERN,$arr[$c]);
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
		} else if($argv[1] == "help") {
			help();
		} else {
			echo "\n\tInvalid command!\n\n";
			help();
		}

    return $i;
}


 error_reporting(E_ERROR);

 $tt1 = gettimeofday();

 $i = main_loop($argv);

 $tt2 = gettimeofday();

 $sec = $tt2['sec'] - $tt1['sec'];

 if($tt2['usec'] > $tt1['usec']) $usec = $tt2['usec'] - $tt1['usec'];
 else $usec = $tt1['usec'] - $tt2['usec'];

 $t = $sec*1000 + $usec/1000;
 echo "\n\nt: $t ms\n";

 if($i) {
    $q = 1000000000/$t;
    echo "q: $q req/s\n";
 }
?>
