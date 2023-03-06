#!/usr/bin/sh
#
# JSON-RPC test cases, 2019-08-08
#
# https://linuxconfig.org/how-to-use-arrays-in-bash-script
#

declare -a test_arr

desc_arr[0]="Normal 'JSON-RPC 2.0' request"
test_arr[0]=" recv '{\"jsonrpc\":\"2.0\",\"method\":\"maxsec\",\"params\":{\"call-uid\":\"1234\",\"clg\":\"35924119998\",\"cld\":\"0886893345\"},\"id\":111}'"
		
desc_arr[1]="'params' are different format from the model"
test_arr[1]=" recv '{\"jsonrpc\":\"2.0\",\"method\":\"maxsec\",\"params\":{\"maxsec\":3600},\"id\":111}'"

desc_arr[2]="Version is 1.0 instead 2.0"
test_arr[2]=" recv '{\"jsonrpc\":\"1.0\",\"method\":\"maxsec\",\"params\":{\"call-uid\":\"1234\",\"clg\":\"35924119998\",\"cld\":\"0886893345\"},\"id\":111}'"
		
desc_arr[3]="'jsonr' instead 'jsonrpc' element - not 'JSON-RPC 2.0' format"
test_arr[3]=" recv '{\"jsonr\":\"2.0\",\"method\":\"maxsec\",\"params\":{\"call-uid\":\"1234\",\"clg\":\"35924119998\",\"cld\":\"0886893345\"},\"id\":111}'"
		
desc_arr[4]="Method is not exist as 'proto-type'" 
test_arr[4]=" recv '{\"jsonrpc\":\"2.0\",\"method\":\"maxsec222\",\"params\":{\"call-uid\":\"1234\",\"clg\":\"35924119998\",\"cld\":\"0886893345\"},\"id\":111}'"
		
desc_arr[5]="Without 'id' element" 
test_arr[5]=" recv '{\"jsonrpc\":\"2.0\",\"method\":\"maxsec\",\"params\":{\"call-uid\":\"1234\",\"clg\":\"35924119998\",\"cld\":\"0886893345\"}}'"
		
desc_arr[6]="Without 'params' element"
test_arr[6]=" recv '{\"jsonrpc\":\"2.0\",\"method\":\"maxsec\",\"id\":111}'"

desc_arr[7]="With some 'params',not full elements"		
test_arr[7]=" recv '{\"jsonrpc\":\"2.0\",\"method\":\"maxsec\",\"params\":{\"clg\":\"35924119998\",\"cld\":\"0886893345\"},\"id\":111}'"

desc_arr[8]="No 'JSON-RPC' format string (no JSON format)"
test_arr[8]=" recv 'test Dimitar Kokov]}'"

desc_arr[9]="rpc call Batch, invalid JSON"
test_arr[9]=" recv '[{\"jsonrpc\": \"2.0\", \"method\": \"sum\", \"params\": [1,2,4], \"id\": \"1\"},{\"jsonrpc\": \"2.0\", \"method\"]'"

desc_arr[10]="Send JSON-RPC request - Notification without parameters"	  
test_arr[10]=" send heartbeat"


APP="./json_rpc_test_2" 

for i in "${!test_arr[@]}";
do 
	echo "************************ test $i ************************";
	echo ${desc_arr[$i]};
	eval $APP${test_arr[$i]};
	echo "";
	sleep 1;
done
