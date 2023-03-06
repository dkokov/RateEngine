#ifndef RT_DATA_H
#define RT_DATA_H

//#include "rating.h"
//#include "pcard.h"

#define NUMBER_RATES 20
#define NUMBER_TARIFF 5

//#define CALL_MAXSEC_LIMIT 3600

#define CLG_LEN 80
#define CLD_LEN 80
#define CALL_UID_LEN 128
#define COMM_LEN 4
#define PREFIX_LEN 32
#define RATING_ACCOUNT_LEN 80

#define TS_LUPDATE 32

#define WAIT_RATING  1500
#define BLOCK_RATING 50000

#define PCARD_SORT_KEY  "start_date"
#define PCARD_SORT_MODE "desc"

#define RT_BAL_NUM 2

#define BACC_USERNAME_LEN   64
#define BACC_CURRENCY_LEN   12
#define BACC_LEG_LEN        3
#define BACC_ROUND_MODE_LEN 32

#define BPLAN_NAME_LEN 64

#define RACC_RATING_ACCOUNT  64

#define DATE_LEN 32

typedef struct pcard
{
    int id;
    double amount;
    char start[DATE_LEN];
    char end[DATE_LEN];
    int status;
//    int bacc;
    int type;
    int call_number;
    
//    struct pcard *next;
} pcard_t;

typedef struct bacc {
	unsigned int id;
	char username[BACC_USERNAME_LEN];
	
	unsigned int currency_id;
	char currency[BACC_CURRENCY_LEN];
	
	unsigned int cdr_server_id;
	char profile_name[32];

	char leg[BACC_LEG_LEN];
	
	unsigned int round_mode_id;
	char round_mode[BACC_ROUND_MODE_LEN];
	
	unsigned short billing_day;
	unsigned short day_of_payment;

//	int pcard_i;
	pcard_t *pcard_ptr;
} bacc_t;

typedef struct calc_function
{
    int pos;
    int delta;
    double fee;
    int iterations;
} calc_function_t;

typedef struct rate {
	unsigned int id;
	
	unsigned int prefix_id;
	char prefix[32];
	
	unsigned int tariff_id;
	char tariff_name[32];
	
	unsigned int start_period;
	unsigned int end_period;
	
	calc_function_t *calc_funcs;
} rate_t;

typedef struct bplan {
	unsigned int id;
	char bplan_name[BPLAN_NAME_LEN];
	
	unsigned int bplan_start_period;
	unsigned int bplan_end_period;
		
	int rates_i;
	rate_t *rates_ptr;
} bplan_t;

typedef struct bal {
	unsigned int id;
	
	double amount;
	
	unsigned int free_billsec;

	char start_period[32];
	char end_period[32];
} bal_t;

typedef struct __rating {
    int cdr_server_id;

    char clg[CLG_LEN];
    char cld[CLD_LEN];
    char call_uid[CALL_UID_LEN];

    char timestamp[32];

    int epoch;

    int clg_nadi;
    int cld_nadi;

//    int rate;
//    int tariff;
//    int prefix;
    int maxsec;
    int billsec;
    long billusec;

    int free_id;
    int free_billsec;
    int free_billsec_id;
    int free_billsec_sum;
    int free_billsec_limit;

    int dow;
    
    //int dcard;
    //int ccard;
    
    //int pcard;
//    struct pcard *card;

    double balanse;
    int bal_id;
    double limit;
    double cprice;
	char bal_lupdate[TS_LUPDATE];

    int ts;

    struct time_cond *tc;
    int tc_id;
    int cdr_id;

    int rating_id;
    int rating_mode_id;
    //int cdr_rec_type_id;
    
    char account_code[RATING_ACCOUNT_LEN];
    char src_context[RATING_ACCOUNT_LEN];
    char dst_context[RATING_ACCOUNT_LEN];
    char src_tgroup[RATING_ACCOUNT_LEN];
    char dst_tgroup[RATING_ACCOUNT_LEN];
    
    unsigned int start_timer;
    unsigned int current_timer;
    unsigned int stop_timer;
    
//    tariff *tr;
	int tariff_id;
	int rate_id;
	int prefix_id;
} rating_t;

typedef enum rt_mode {
	rt_mode_clg   = 1, /* calling_number */
	rt_mode_acode = 2, /* account_code */
	rt_mode_srcc  = 3, /* src_context */
	rt_mode_dstc  = 4, /* dst_context */
	rt_mode_srctg = 5, /* src_tgroup */
	rt_mode_dsttg = 6, /* dst_tgroup */
	rt_mode_sms   = 7  /* sms */
} rt_mode_t;

typedef struct racc {
	rt_mode_t rtm;
	
	unsigned int id;
	char rating_account[RACC_RATING_ACCOUNT];
	
	bal_t *bal_ptr;
	
	bacc_t *bacc_ptr;

	bplan_t *bplan_ptr;
	
	rating_t *pre;
} racc_t;

rating_t *rt_data_rating_init(void);
bacc_t *rt_data_bacc_init(void);
bplan_t *rt_data_bplan_init(void);
racc_t *rt_data_racc_init(void);
void rt_data_racc_free(racc_t *racc_pt);
//void rt_data_racc_clean(racc_t *racc_pt);


#endif
