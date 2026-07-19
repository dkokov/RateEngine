#ifndef RT_CACHE_H
#define RT_CACHE_H

#include <pthread.h>
#include <time.h>

#include "rt_data.h"

/* forward declaration - defined in db/db.h */
typedef struct db db_t;

#define RT_CACHE_BUCKETS 256

/* reference-data time-to-live (seconds). Entries older than this are treated as
 * a miss on get and re-queried; a stale rate/tariff edit thus takes effect
 * within RT_CACHE_TTL seconds. Only the static reference tables are cached -
 * live balance/consumed-spend is always queried per call. */
#define RT_CACHE_TTL 60

/* cache entry for rates (key = bplan_id) */
typedef struct rt_cache_rate_entry {
	unsigned int bplan_id;
	int count;                 /* number of real rate rows (array is count+1, null-terminated) */
	time_t ts;
	rate_t *rates;
	struct rt_cache_rate_entry *next;
} rt_cache_rate_entry_t;

/* cache entry for tariffs (key = tariff_id) */
typedef struct rt_cache_tariff_entry {
	unsigned int tariff_id;
	int count;                 /* number of real calc_function rows (array is count+1) */
	time_t ts;
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
	time_t ts;
	rt_cache_racc_data_t data;
	struct rt_cache_racc_entry *next;
} rt_cache_racc_entry_t;

/* cache entry for pcards (key = bacc_id) */
typedef struct rt_cache_pcard_entry {
	unsigned int bacc_id;
	int count;
	time_t ts;
	pcard_t *cards;
	struct rt_cache_pcard_entry *next;
} rt_cache_pcard_entry_t;

/* cache entry for time condition check (key = tariff_id) */
typedef struct rt_cache_tc_entry {
	unsigned int tariff_id;
	int has_tc;
	time_t ts;
	struct rt_cache_tc_entry *next;
} rt_cache_tc_entry_t;

/* cache entry for free_billsec limit (key = tariff_id) */
typedef struct rt_cache_fbs_entry {
	unsigned int tariff_id;
	int free_billsec_limit;
	int queried;
	time_t ts;
	struct rt_cache_fbs_entry *next;
} rt_cache_fbs_entry_t;

typedef struct rt_cache {
	rt_cache_rate_entry_t   *rates[RT_CACHE_BUCKETS];
	rt_cache_tariff_entry_t *tariffs[RT_CACHE_BUCKETS];
	rt_cache_racc_entry_t   *raccs[RT_CACHE_BUCKETS];
	rt_cache_pcard_entry_t  *pcards[RT_CACHE_BUCKETS];
	rt_cache_tc_entry_t     *tcs[RT_CACHE_BUCKETS];
	rt_cache_fbs_entry_t    *fbs[RT_CACHE_BUCKETS];

	/* one rwlock guards the whole cache: many concurrent readers (get, the hot
	 * path) OR one writer (put). Required because the online CallControl worker
	 * pool and the offline rating worker threads share a single cache. */
	pthread_rwlock_t lock;

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

/* rates cache. get() returns a freshly allocated COPY the caller owns (or NULL
 * on miss/expired); put() stores its own copy of 'rates'. Copy-on-hit keeps the
 * cache decoupled from racc_t lifetime (racc_t is created by a worker but freed
 * by the janitor), so TTL eviction can never dangle a live call's rates_ptr. */
rate_t *rt_cache_rates_get(rt_cache_t *cache,unsigned int bplan_id);
void rt_cache_rates_put(rt_cache_t *cache,unsigned int bplan_id,rate_t *rates,int count);

/* tariff cache (calc_function array). get() returns an owned copy; put() copies. */
calc_function_t *rt_cache_tariff_get(rt_cache_t *cache,unsigned int tariff_id);
void rt_cache_tariff_put(rt_cache_t *cache,unsigned int tariff_id,calc_function_t *funcs,int count);

/* subscriber (racc) cache. get() copies the record into *out (returns 1 hit / 0 miss). */
int rt_cache_racc_get(rt_cache_t *cache,const char *key,rt_cache_racc_data_t *out);
void rt_cache_racc_put(rt_cache_t *cache,const char *key,rt_cache_racc_data_t *data);

/* pcard cache. get() returns an owned copy of the card array (+count) or NULL; put() copies. */
pcard_t *rt_cache_pcard_get(rt_cache_t *cache,unsigned int bacc_id,int *count);
void rt_cache_pcard_put(rt_cache_t *cache,unsigned int bacc_id,pcard_t *cards,int count);

/* time condition cache (just the has_tc flag) */
int rt_cache_tc_get(rt_cache_t *cache,unsigned int tariff_id,int *has_tc);
void rt_cache_tc_put(rt_cache_t *cache,unsigned int tariff_id,int has_tc);

/* free billsec limit cache */
int rt_cache_fbs_get(rt_cache_t *cache,unsigned int tariff_id,int *limit);
void rt_cache_fbs_put(rt_cache_t *cache,unsigned int tariff_id,int limit);

/* preload subscribers - pass array of calling numbers */
int rt_cache_preload_raccs(rt_cache_t *cache,db_t *dbp,char **numbers,int count,int cdr_server_id);
int rt_cache_preload_pcards(rt_cache_t *cache,db_t *dbp);

#endif
