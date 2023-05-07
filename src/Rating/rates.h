#ifndef RATES_H
#define RATES_H

#define RATES_BPLAN_TBL_NUM 20
#define RATES_SHMEM_KEY 5768

pthread_mutex_t cache_tbl_lock;

typedef struct rates_cache_tbl
{
	unsigned int bplan;
	rates *rt;
}rates_cache_tbl;

rates *rates_get_bplan_rates(PGconn *conn,rating *pre);
void rates_cache_tbl_free(void);

#endif
