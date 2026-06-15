#ifndef RT_BATCH_Q_H
#define RT_BATCH_Q_H

#include "rt_data.h"

/* rated CDR result - ready for bulk insert */
typedef struct rt_rated {
	int cdr_id;
	int cdr_rec_type_id;
	double cprice;
	int billsec;
	int rate_id;
	unsigned int bacc_id;
	int rating_mode_id;
	int pcard_id;
	int tc_id;
	char timestamp[32];
	int free_billsec_id;

	int rating_id;    /* set after bulk insert */
	int status;       /* 0=ok, -1=error, -2=already rated */
} rt_rated_t;

/* bulk insert ratings and get back IDs */
int rt_batch_insert_ratings(db_t *dbp,rt_rated_t *results,int count);

/* bulk update cdrs leg_a */
int rt_batch_update_cdrs(db_t *dbp,rt_rated_t *results,int count,char leg);

/* bulk update balances - aggregate per billing_account */
int rt_batch_update_balances(db_t *dbp,rt_rated_t *results,int count);

#endif
