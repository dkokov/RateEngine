#ifndef RATING_H
#define RATING_H

#include "../CDRMediator/cdr_bind_api.h"

#include "rt_data.h"

typedef struct rate_engine
{
    db_t *dbp;
    
    int  log;
    char active;
    char leg;
    char no_prefix_rating;
    int  rating_interval;
    unsigned wait_rating;
    int  rating_flag;
    char rating_mode[RATING_ACCOUNT_LEN];
    char rating_account[RATING_ACCOUNT_LEN];
    
    pid_t pid;
    pthread_t rt_thread;
    
    char pcard_sort_key[12];
    char pcard_sort_mode[5];
    
    unsigned short bal_num;
 	char rt_cfg_json_dir[256];   
} rate_engine_t;

rate_engine_t rt_eng;

void *RateEngine(void *dt);
void rt_exec(db_t *dbp,racc_t *rtp,char leg);
void rt_chk_bplan_periods(bplan_t *bpt,int ts);
int rt_double_rating(db_t *dbp,racc_t *rtp,char leg);
void rt_balance_exec(db_t *dbp,racc_t *rtp,char *start,char *end);

/* cdrm export functions */
cdr_funcs_t *cdrm_api;

#endif
