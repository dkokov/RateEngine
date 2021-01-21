#ifndef RT_DATA_Q_H
#define RT_DATA_Q_H

#define RT_SQL_CLG_TBL_STR   "calling_number"
#define RT_SQL_ACC_TBL_STR   "account_code"
#define RT_SQL_SRCC_TBL_STR  "src_context"
#define RT_SQL_DSTC_TBL_STR  "dst_context"
#define RT_SQL_SRCTG_TBL_STR "src_tgroup"
#define RT_SQL_DSTTG_TBL_STR "dst_tgroup"

racc_t *rt_data_q_racc(db_t *dbp,rating_t *pre);
int rt_data_q_rate(db_t *dbp,racc_t *rtp);
int rt_data_q_rating_id(db_t *dbp,racc_t *rtp);
int rt_data_q_rating_add(db_t *dbp,racc_t *rtp);
int rt_data_q_bal(db_t *dbp,racc_t *rtp,char *time1,char *time2);
int rt_data_q_bal_count(db_t *dbp,racc_t *rtp,char flag);
int rt_data_q_bal_add(db_t *dbp,racc_t *rtp,char *start,char *end);
int rt_data_q_pcard(db_t *dbp,bacc_t *bpt,int status);
int rt_data_q_pcard_set(db_t *dbp,bacc_t *bpt,int status);

#endif
