#ifndef CC_SERVER_H
#define CC_SERVER_H

typedef struct cc_server {
	
	db_t *dbp;
	// db_t *dbp2; ??
	
	unsigned int ccserver_usleep;
	rating *pre;
}cc_server_t;

cc_server_t ccserver;

typedef struct cc_server_tbl {
	
	cc_t *cc_ptr;
	rating *pre;
	
} cc_server_tbl_t;

cc_server_tbl_t *cc_tbl;
pthread_mutex_t cc_tbl_lock;
//pthread_t ccserver_proc;

void *cc_server_main(void *dt);
int cc_server_call_init(cc_t *cc_ptr,rating *pre);
int cc_server_call_search_call_uid(cc_t *cc_ptr);
int cc_server_call_search_racc(char *racc);
void cc_server_call_term(cc_server_tbl_t *tbl,cc_t *cc_ptr);

#define CC_SERVER_USLEEP 50000

unsigned short cc_server_sim;

#define CALL_MAXSEC_LIMIT 3660
#define CALL_MAXSEC_LIMIT_WAIT 90
#define SIM_CALLS 50

rt_funcs_t rt_api;

#endif 
