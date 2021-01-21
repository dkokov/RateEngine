/* 
 * My CallControl protocol version 1.1, definitions & declarations 
 * 
 * */

#ifndef MY_CC_H
#define MY_CC_H

#include "../cc.h"
#include "../../misc/proc/proc_thread.h"

/* MyCC fields separator */
#define MY_CC_SEPARATOR ","

/* MyCC comands */
#define MY_CC_STATUS  "status"
#define MY_CC_MAXSEC  "maxsec"
#define MY_CC_RATE    "rate"
#define MY_CC_BALANCE "balance"
#define MY_CC_CPRICE  "cprice"
#define MY_CC_TERM    "term"

/* MyCC comand length - max */
#define MY_CC_COMM_LEN 16

#define MY_CC_TERM_NC "nc"
#define MY_CC_TERM_U  "u"
#define MY_CC_TERM_B  "b"
#define MY_CC_TERM_E  "e"
#define MY_CC_TERM_C  "c"

/* Transaction timer */
typedef unsigned short ttimer_t;

typedef struct my_cc {
	
	unsigned short cdr_server_id;
	
	unsigned int transaction_id;
	
	char command[MY_CC_COMM_LEN];
	
	time_t timestamp;
	
	cc_t *cc_ptr;
	
	char *ack;
}my_cc_t;

#define MY_CC_OK  "ok"
#define MY_CC_NOK "nok"
#define MY_CC_ERR "empty"

typedef struct my_cc_thread_dt {
	
	proc_thread_t proc;
	
	int server_sockfd;
		
}my_cc_thread_dt_t;

#define TCP_SOCKETS_MAX 30

#define MY_CC_THREAD_SLEEP 15000

int my_cc_main(cc_cfg_int_t *_int);

#endif 
