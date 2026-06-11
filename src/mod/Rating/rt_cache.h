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

typedef struct rt_cache {
	rt_cache_rate_entry_t   *rates[RT_CACHE_BUCKETS];
	rt_cache_tariff_entry_t *tariffs[RT_CACHE_BUCKETS];

	unsigned int rates_hits;
	unsigned int rates_misses;
	unsigned int tariff_hits;
	unsigned int tariff_misses;
} rt_cache_t;

rt_cache_t *rt_cache_init(void);
void rt_cache_free(rt_cache_t *cache);

/* rates cache: returns cached rate_t* or NULL */
rate_t *rt_cache_rates_get(rt_cache_t *cache,unsigned int bplan_id);
void rt_cache_rates_put(rt_cache_t *cache,unsigned int bplan_id,rate_t *rates);

/* tariff cache: returns cached calc_function_t* or NULL */
calc_function_t *rt_cache_tariff_get(rt_cache_t *cache,unsigned int tariff_id);
void rt_cache_tariff_put(rt_cache_t *cache,unsigned int tariff_id,calc_function_t *funcs);

#endif
