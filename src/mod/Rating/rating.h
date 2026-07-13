#ifndef RATING_H
#define RATING_H

#include "../CDRMediator/cdr_bind_api.h"

#include "rt_data.h"
#include "rt_cache.h"

#define RT_MAX_THREADS 16
#define RT_DEFAULT_THREADS 1

/* worker thread context */
typedef struct rt_worker {
    int id;
    db_t *dbp;
    cdr_t *cdrs;
    int *indices;
    int count;
    char leg;
    int rated_ok;
    int rated_err;
} rt_worker_t;

typedef struct rate_engine
{
    db_t *dbp;
    rt_cache_t *cache;

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
    unsigned short num_threads;

    /* worker threads */
    rt_worker_t workers[RT_MAX_THREADS];
    pthread_t worker_threads[RT_MAX_THREADS];
    db_t *worker_dbp[RT_MAX_THREADS];

 	char rt_cfg_json_dir[256];
} rate_engine_t;

extern rate_engine_t rt_eng;

void *RateEngine(void *dt);
void rt_exec(db_t *dbp,racc_t *rtp,char leg);
racc_t *rt_racc_voip_av_a(db_t *dbp,rating_t *pre);
void rt_rate_searching(db_t *dbp,racc_t *rtp);
void rt_chk_bplan_periods(bplan_t *bpt,int ts);
void rt_chk_tr_opt(db_t *dbp,rating_t *pre);
int rt_double_rating(db_t *dbp,racc_t *rtp,char leg);
void rt_balance_exec(db_t *dbp,racc_t *rtp,char *start,char *end);

/* cdrm export functions */
extern cdr_funcs_t *cdrm_api;

#endif
