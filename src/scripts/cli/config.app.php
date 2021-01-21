<?php
 
 include('lib/lib.db.php');
 include('lib/lib.importing.php');
 include('lib/lib.dumping.php');
  
 /* ******************************************************************** */
 /* RateEngine - global configuration */
 $RE['billing_day'] = "01"; // every first day from the month !
 $RE['credit_limit'] = 50.00; // default value for credit limit !!!
 $RE['rounding']  = 0;

 /* DataBase of the RateEngine */
 $RE['dbhost'] = "127.0.0.1";
 $RE['dbname'] = "re7";
 $RE['dbuser'] = "global";
 $RE['dbpass'] = "_cfg.access";
 
 $dbconn = conn();
 
 /* MyCC server params */
 $RE['cc_host'] = "127.0.0.1";
 $RE['cc_port'] = 9090;
 
 /* ******************************************************************** */ 
 /* App info */
 $APP['app'] = "RateEngine PHPLIB";
 $APP['creator'] = "Dimitar Kokov";
 $APP['version'] = "0.6.2 (2018-04-16)";
?>
