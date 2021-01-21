#ifndef RATING_H
#define RATING_H

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

#define WAIT_RATING 1500
#define BLOCK_RATING 50000

#define PCARD_SORT_KEY  "start_date"
#define PCARD_SORT_MODE "desc"

#define RT_BAL_NUM 2

typedef unsigned int rtimer_t;

typedef struct rates
{
    char prefix[PREFIX_LEN];
    int rate;
    int tariff;
    int prefix_id;
    int start_period;
    int end_period;
    int free_billsec_id;
}rates;

typedef struct tariff
{
    int pos;
    int delta;
    double fee;
    int iterations;
}tariff;

typedef struct rating
{
    int cdr_server_id;

    char clg[CLG_LEN];
    char cld[CLD_LEN];
    char call_uid[CALL_UID_LEN];

    char timestamp[TS_LEN];

    int epoch;

    int bacc;
    int bplan;
    int bplan_start_period;
    int bplan_end_period;
    char billing_day[3];
    int round_mode_id;
    int day_of_payment;

    int clg_nadi;
    int cld_nadi;

    int rate;
    int tariff;
    int prefix;
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
    struct pcard *card;

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

    tariff *tr;
} rating;
/*
typedef struct node
{
    struct node *prev;
    struct rating *cache;
    struct node *next;
    int ts;
}node;
*/
/*
typedef struct rated
{
    double cprice;
    int billsec;
    int call_id;
    int bacc;
}rated;
*/
typedef struct rate_engine
{
    PGconn *conn;
    int log;
    char active;
    char leg;
    char no_prefix_rating;
    int rating_interval;
    int rating_flag;
//    char rating_mode[RATING_ACCOUNT_LEN];
//    char rating_account[RATING_ACCOUNT_LEN];
    
//    pid_t pid;
//    pthread_t rt_thread;
    
    char pcard_sort_key[12];
    char pcard_sort_mode[5];
    
    unsigned short bal_num;
    
}rate_engine;

pthread_mutex_t sync_bt_thread;

void f_calling_number_query(PGconn *conn,rating *pre);
void f_calling_number_sms_query(PGconn *conn,rating *pre);
void f_account_code_query(PGconn *conn,rating *pre,char *acc);
void f_src_context_query(PGconn *conn,rating *pre,char *src_context);
void f_src_tgroup_query(PGconn *conn,rating *pre,char *src_tgroup,int clg_nadi,int cld_nadi);
void f_dst_context_query(PGconn *conn,rating *pre,char *dst_context);
void f_dst_tgroup_query(PGconn *conn,rating *pre,char *dst_tgroup,int clg_nadi,int cld_nadi);
void f_bacc_query(PGconn *conn,rating *pre);
void f_bal_query(PGconn *conn,rating *pre,char *start,char *end);
void f_credit_query(PGconn *conn,rating *pre);
void f_debit_query(PGconn *conn,rating *pre);
rates *f_rate_query(PGconn *conn,rating *pre);
tariff *f_tariff_query(PGconn *conn,rating *pre);
void balance(PGconn *conn,rating *pre,char *time1,char *time2);
void update_balance(PGconn *conn,int bid,double bill);
//void f_rate_query_2(PGconn *conn,rating *pre);
tariff *rate_searching(PGconn *conn,rating *pre);
void create_balance_pgsql(PGconn *conn,rating *pre,char *start,char *end);
void create_balance_pgsql_v2(PGconn *conn,rating *pre,char *start,char *end);
void calculate(tariff *tr,rating *pre);
void f_rating_id_query(PGconn *conn,rating *pre);
void f_rating_id_query_2(PGconn *conn,rating *pre);
void insert_rating_pgsql(PGconn *conn,rating *pre);
void f_cdr_id_query(PGconn *conn,rating *pre);
void tr_free(tariff *tr);
void copy_cdr_to_rating(cdr_t *the_cdr,rating *pre);
void rating_init(rating *pre);

void balance_exec(PGconn *conn,rating *pre,char *start,char *end);
//int free_billsec_exec(PGconn *conn,rating *pre,char *start,char *end);

void Rating5(PGconn *conn,rating *pre,char leg);
int chk_bplan_periods(rating *pre);

//#if PROC_THREAD_FLAG
void *RateEngine(void *dt);
//#else
//void RateEngine(void);
//#endif

/* New functions , 2016-03-23 */
int rt_get_bacc_id(PGconn *conn,char *table,int cdr_server_id,char *cond);
double rt_get_balances(PGconn *conn,int bacc_id);

/* 2018-03-01 */
int f_bplan_tree_num(PGconn *conn,rating *pre);

#endif
