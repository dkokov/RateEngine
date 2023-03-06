
* status *

format:
cdr_server_id,transaction_id,command,timestamp

example:
./2cclient 192.168.12.56 9090 42,1,status,1234

* term *

format:
cdr_server_id,transaction_id,command,timestamp,status,call_uid,billsec,duration

example:

Normal clear: ./2cclient 192.168.12.56 9090 42,1,term,1234,nc,1111,36,60


* maxsec *

format:
cdr_server_id,transaction_id,command,timestamp,clg,cld,call_uid

example:
./2cclient 192.168.12.56 9090 42,1,maxsec,1234,35924119460,359884119998,1111

* rate *

format:
cdr_server_id,transaction_id,timestamp,

example:
3,1,maxsec,1234,

* balance *

format:
cdr_server_id,transaction_id,timestamp,

example:
3,1,maxsec,1234,

* cprice *

format:
cdr_server_id,transaction_id,timestamp,

example:
3,1,maxsec,1234,



