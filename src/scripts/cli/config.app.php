<?php
 
 include('lib/lib.db.php');
 include('lib/lib.importing.php');
 include('lib/lib.dumping.php');
 include('lib/lib.re5.php');
 include('lib/lib.testing.php');
  
 /* ******************************************************************** */
 /* RateEngine - global configuration */
 
 $RE['billing_day'] = "01"; // every first day from the month !
 $RE['credit_limit'] = 50.00; // default value for credit limit !!!
 $RE['rounding']  = 0;
 
 /* ******************************************************************** */
 /* DataBase of the RateEngine */
 
 $RE['dbhost'] = "your hostname";
 $RE['dbname'] = "your database";
 $RE['dbuser'] = "your username";
 $RE['dbpass'] = "your password";
 
 $dbconn = conn();

 /* ******************************************************************** */
 /* DataBase of the FreeSWITCH - CDRs */
 
 $FS['dbhost'] = "your hostname";
 $FS['dbname'] = "your database";
 $FS['dbuser'] = "your username";
 $FS['dbpass'] = "your password";
 
 /* ******************************************************************** */ 
 /* App info */
 
 $APP['app'] = "RateEngine PHPLIB";
 $APP['creator'] = "Dimitar Kokov";
 $APP['version'] = "0.6.3 (2020-02-25)";
?>
