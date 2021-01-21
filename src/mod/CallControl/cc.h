#ifndef CC_H
#define CC_H

#define CC_MAXSEC_NO_BACC   -1
#define CC_MAXSEC_NO_BPLAN  -2
#define CC_MAXSEC_NO_PCARD  -3
#define CC_MAXSEC_NO_CLIMIT -4
#define CC_MAXSEC_CRESTICT  -6
#define CC_MAXSEC_NO_TARIFF -7
#define CC_MAXSEC_NO_CREDIT -8
#define CC_MAXSEC_NO_PRE    -9
#define CC_MAXSEC_NO_CCTBL  -10

typedef enum cc_events {
	unkn_event,
	status_event,
	maxsec_event,
	balance_event,
	rate_event,
	cprice_event,
	term_event
} cc_events_t;

#define CC_CLG_LEN 32
#define CC_CLD_LEN 32
#define CC_CALL_UID 128

typedef struct cc_maxsec {
	
	char clg[CC_CLG_LEN];
	char cld[CC_CLD_LEN];
	char call_uid[CC_CALL_UID];
	
	int ts;
	
	int maxsec;	
	
} cc_maxsec_t;

typedef struct cc_balance {

	char clg[CC_CLG_LEN];
	
	double amount;

}cc_balance_t;

typedef struct cc_rate {
		
	char clg[CC_CLG_LEN];
	char cld[CC_CLD_LEN];
	
	float rate_price;
	
} cc_rate_t;

typedef struct cc_cprice {
	
	char clg[CC_CLG_LEN];
	char cld[CC_CLD_LEN];
	unsigned int billsec;
	
	float cprice;
	
} cc_cprice_t;

typedef enum cc_term_status {
	unkn_status,
	normal_clear,
	busy,
	cancel,
	error
} cc_term_status_t;

typedef struct cc_term {
	
	cc_term_status_t status;
	char call_uid[CC_CALL_UID];
	
	unsigned int billsec;
	unsigned int duration;
	
	int tbl_idx;
	
} cc_term_t;

typedef struct cc {
	
	/* CDR Server ID */
	unsigned short cdr_server_id;
	
	/* Call Control Event */
	cc_events_t t;
	
	/* Call Control struct for maxsec event */
	cc_maxsec_t *max;
	
	/* Call Control struct for balance event */
	cc_balance_t *bal;
	
	/* Call Control struct for rate event */
	cc_rate_t *rate;
	
	/* Call Control struct for cprice event */
	cc_cprice_t *cprice;
	
	/*Call Control struct for term (terminate) event */
	cc_term_t *term;
	
	unsigned short cc_status;
} cc_t;

#define CC_STATUS_ACTIVE   1
#define CC_STATUS_DEACTIVE 0

#define CC_SLEEP 5000

void cc_maxsec_f(cc_t *cc_ptr);
void cc_term_f(cc_t *cc_ptr);
void cc_balance_f(cc_t *cc_ptr);

void cc_event_manager(cc_t *cc_ptr);
void cc_call_rating(cc_t *ptr,rating *pre);
cc_t *cc_alloc(void);
void cc_free(cc_t *cc_ptr);

#endif 
