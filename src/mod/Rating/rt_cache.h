#ifndef RT_CACHE_H
#define RT_CACHE_H

#include "rt_data.h"

#define RT_CACHE_BUCKETS 256

/* cache entry for rates (key = bplan_id) */
typedef struct rt_cache_rate_entry {
	unsigned int bplan_id;
	rate_t *rates;
	struct rt_cache_rate_entry *next;
} rt_cache_rate_entry_t;

/* cache entry for tariffs (key = tariff_id) */
typedef struct rt_cache_tariff_entry {
	unsigned int tariff_id;
	calc_function_t *calc_funcs;
	struct rt_cache_tariff_entry *next;
} rt_cache_tariff_entry_t;

/* cached subscriber data (from racc query) */
typedef struct rt_cache_racc_data {
	unsigned int bacc_id;
	unsigned int bplan_id;
	unsigned int bplan_start_period;
	unsigned int bplan_end_period;
	unsigned short billing_day;
	unsigned int round_mode_id;
	unsigned int day_of_payment;
} rt_cache_racc_data_t;

/* cache entry for subscribers (key = string: calling_number etc.) */
typedef struct rt_cache_racc_entry {
	char key[128];
	rt_cache_racc_data_t data;
	struct rt_cache_racc_entry *next;
} rt_cache_racc_entry_t;

/* cache entry for pcards (key = bacc_id) */
typedef struct rt_cache_pcard_entry {
	unsigned int bacc_id;
	int count;
	pcard_t *cards;
	struct rt_cache_pcard_entry *next;
} rt_cache_pcard_entry_t;

/* cache entry for time condition check (key = tariff_id) */
typedef struct rt_cache_tc_entry {
	unsigned int tariff_id;
	int has_tc;
	struct rt_cache_tc_entry *next;
} rt_cache_tc_entry_t;

/* cache entry for free_billsec limit (key = tariff_id) */
typedef struct rt_cache_fbs_entry {
	unsigned int tariff_id;
	int free_billsec_limit;
	int queried;
	struct rt_cache_fbs_entry *next;
} rt_cache_fbs_entry_t;

typedef struct rt_cache {
	rt_cache_rate_entry_t   *rates[RT_CACHE_BUCKETS];
	rt_cache_tariff_entry_t *tariffs[RT_CACHE_BUCKETS];
	rt_cache_racc_entry_t   *raccs[RT_CACHE_BUCKETS];
	rt_cache_pcard_entry_t  *pcards[RT_CACHE_BUCKETS];
	rt_cache_tc_entry_t     *tcs[RT_CACHE_BUCKETS];
	rt_cache_fbs_entry_t    *fbs[RT_CACHE_BUCKETS];

	unsigned int rates_hits;
	unsigned int rates_misses;
	unsigned int tariff_hits;
	unsigned int tariff_misses;
	unsigned int racc_hits;
	unsigned int racc_misses;
	unsigned int pcard_hits;
	unsigned int pcard_misses;
} rt_cache_t;

rt_cache_t *rt_cache_init(void);
void rt_cache_free(rt_cache_t *cache);

/* rates cache */
rate_t *rt_cache_rates_get(rt_cache_t *cache,unsigned int bplan_id);
void rt_cache_rates_put(rt_cache_t *cache,unsigned int bplan_id,rate_t *rates);

/* tariff cache */
calc_function_t *rt_cache_tariff_get(rt_cache_t *cache,unsigned int tariff_id);
void rt_cache_tariff_put(rt_cache_t *cache,unsigned int tariff_id,calc_function_t *funcs);

/* subscriber (racc) cache */
rt_cache_racc_data_t *rt_cache_racc_get(rt_cache_t *cache,const char *key);
void rt_cache_racc_put(rt_cache_t *cache,const char *key,rt_cache_racc_data_t *data);

/* pcard cache */
rt_cache_pcard_entry_t *rt_cache_pcard_get(rt_cache_t *cache,unsigned int bacc_id);
void rt_cache_pcard_put(rt_cache_t *cache,unsigned int bacc_id,pcard_t *cards,int count);

/* time condition cache (just the has_tc flag) */
int rt_cache_tc_get(rt_cache_t *cache,unsigned int tariff_id,int *has_tc);
void rt_cache_tc_put(rt_cache_t *cache,unsigned int tariff_id,int has_tc);

/* free billsec limit cache */
int rt_cache_fbs_get(rt_cache_t *cache,unsigned int tariff_id,int *limit);
void rt_cache_fbs_put(rt_cache_t *cache,unsigned int tariff_id,int limit);

/* preload subscribers from CDR batch */
struct cdr;
int rt_cache_preload_raccs(rt_cache_t *cache,db_t *dbp,struct cdr *cdrs,int count,int cdr_server_id);
int rt_cache_preload_pcards(rt_cache_t *cache,db_t *dbp);

#endif
