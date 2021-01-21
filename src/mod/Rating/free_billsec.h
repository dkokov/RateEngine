#ifndef FREE_BILLSEC_H
#define FREE_BILLSEC_H

int f_free_billsec_query(db_t *dbp,rating_t *pre);
int f_free_billsec_query_2(db_t *dbp,rating_t *pre);
int f_free_billsec_bal_query(db_t *dbp,rating_t *pre);
int f_free_billsec_bal_query_2(db_t *dbp,rating_t *pre);
int create_free_billsec_balance(db_t *dbp,rating_t *pre);
int update_free_billsec_balance(db_t *dbp,rating_t *pre);
int free_billsec_sms_balance(db_t *dbp,racc_t *rtp,char *time1,char *time2);
int free_billsec_balance(db_t *dbp,racc_t *rtp,char *time1,char *time2);
int free_billsec_balance_v2(db_t *dbp,racc_t *rtp,char *time1,char *time2);
int free_billsec_exec(db_t *dbp,racc_t *rtp,char *start,char *end);

#endif
