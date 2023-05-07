<?php

/*
--------------060801080207060105090107
Content-Type: text/csv;
 name="portin_ranges.csv"
Content-Transfer-Encoding: 7bit
Content-Disposition: attachment;
 filename="portin_ranges.csv"
 */

$theFile="interbild.csv";

$boundary = '-----=' . md5( uniqid ( rand() ) );

$message .= "Content-Type: text/csv; name=\"$theFile\"\n";
$message .= "Content-Transfer-Encoding: 7bit\n";
$message .= "Content-Disposition: attachment\n\n";

$headers  = "From: \"RateEngine5\"<d.kokov@bulsat.com>\n";
$headers .= "MIME-Version: 1.0\n";

mail('d.kokov@bulsat.com', 'd.kokov@bulsat.com', $message, $headers);


//$Name = "RateEngine5"; //senders name
//$email = "d.kokov@bulsat.com"; //senders e-mail adress
//$recipient = "d.kokov@bulsat.com"; //recipient
//$mail_body = "The text for the mail..."; //mail body
//$subject = "Subject for reviever"; //subject
//$header = "From: ". $Name . " <" . $email . ">\r\n"; //optional headerfields

//mail($recipient, $subject, $mail_body, $header); //mail command :) 

?>