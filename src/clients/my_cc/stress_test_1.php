#!/usr/bin/php -q
<?php 

$ENGINE_HOST = "localhost";
$ENGINE_PORT = "9090";
$ENGINE_COMM = "maxsec";

$CALL_CONTROL_CLIENT = "./2cclient";

$CDR_SERVER_ID = 3;

$CLG = $argv[1];
$CLD = $argv[2];
$CALL_UID = $argv[3];
$INT = $argv[4];
$NUM = $argv[5];

//if($argc != 5) die;

$conn = "host=localhost port=5432 dbname=hpvp user=global password=_cfg.access ";
$dbconn = pg_connect($conn);
if (!$dbconn) 
{
    echo "An error occured.\n";
    exit;
}

$newCALL_UID = $CALL_UID;
$i = 0;
while($i <= $NUM)
{
    $newCALL_UID;

    $command = "$CALL_CONTROL_CLIENT $ENGINE_HOST $ENGINE_PORT $CDR_SERVER_ID,$ENGINE_COMM,$CLG,$CLD,$newCALL_UID";
    $ms = shell_exec($command);
    echo time()."\n";
    echo "SET VARIABLE maxsec ".$ms;
    usleep($INT);

    if($ms > 0)
    {
	$billsec = mt_rand(5,120);
	$sql = "insert into cdrs (call_uid,ts,src,dst,billsec) 
                values('".$newCALL_UID."','".date(DATE_W3C)."','".$CLG."','".$CLD."',".$billsec.")";
	pg_query($dbconn,$sql);
	usleep($INT);
    }

    $newCALL_UID++;

    $i++;
}
?>