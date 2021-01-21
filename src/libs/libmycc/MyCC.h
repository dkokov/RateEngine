#ifndef MyCC_H
#define MyCC_H

#include <MyCC-PDU.h>

#define MyCCVersion 2

typedef struct balance {
	long ts;
	long cdr_server_id;
	long tid;
	char clg[32];
	char host[32];
	
	char amount[16];
}balance_t;

typedef struct maxsec {
	long ts;
	long cdr_server_id;
	long tid;
	char host[32];

	char call_uid[128];
	char clg[32];
	char cld[32];
	
	int maxsec;
}maxsec_t;

typedef struct empty {
	long ts;
	long cdr_server_id;
	long tid;
	char host[32];
	
	int type;
}empty_t;

typedef struct term {
	long ts;
	long cdr_server_id;
	long tid;
	char host[32];

	char call_uid[128];
	char clg[32];
	char cld[32];
	int billsec;
}term_t;

typedef struct rate {
	long ts;
	long cdr_server_id;
	long tid;
	char host[32];

	char clg[32];
	char cld[32];

	char cprice[16];
}rate_t;

MyCC_PDU_t *my_request_balance(balance_t *bal);
MyCC_PDU_t *my_response_balance(balance_t *bal);

MyCC_PDU_t *my_request_maxsec(maxsec_t *max);
MyCC_PDU_t *my_response_maxsec(maxsec_t *max);

MyCC_PDU_t *my_request_empty(empty_t *emp);
MyCC_PDU_t *my_response_empty(empty_t *emp);

MyCC_PDU_t *my_request_term(term_t *tr);

MyCC_PDU_t *my_request_rate(rate_t *rt);
MyCC_PDU_t *my_response_rate(rate_t *rt);

void free_MyCC_PDU(MyCC_PDU_t *myCC);
int recognize_buffer_size(char *buf,size_t max);

int encode_MyCC_PDU(MyCC_PDU_t *myCC,char *buffer,size_t buffer_size,FILE *fp);
int decode_MyCC_PDU(MyCC_PDU_t *myCC_Dec,char *buf,size_t buf_size,FILE *fp);

#endif
