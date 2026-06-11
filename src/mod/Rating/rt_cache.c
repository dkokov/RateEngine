#include <stdio.h>
#include <string.h>

#include "../../mem/mem.h"
#include "../../log/rt_log.h"

#include "rt_cache.h"

static unsigned int rt_cache_hash(unsigned int key)
{
	return key % RT_CACHE_BUCKETS;
}

rt_cache_t *rt_cache_init(void)
{
	rt_cache_t *cache;

	cache = (rt_cache_t *)mem_alloc(sizeof(rt_cache_t));
	if(cache != NULL) memset(cache,0,sizeof(rt_cache_t));

	return cache;
}

void rt_cache_free(rt_cache_t *cache)
{
	int i;
	rt_cache_rate_entry_t *re,*re_next;
	rt_cache_tariff_entry_t *te,*te_next;

	if(cache == NULL) return;

	/* free rates cache entries + their data */
	for(i = 0; i < RT_CACHE_BUCKETS; i++) {
		re = cache->rates[i];
		while(re != NULL) {
			re_next = re->next;
			if(re->rates != NULL) mem_free(re->rates);
			mem_free(re);
			re = re_next;
		}
	}

	/* free tariff cache entries + their data */
	for(i = 0; i < RT_CACHE_BUCKETS; i++) {
		te = cache->tariffs[i];
		while(te != NULL) {
			te_next = te->next;
			if(te->calc_funcs != NULL) mem_free(te->calc_funcs);
			mem_free(te);
			te = te_next;
		}
	}

	LOG("rt_cache_free()","rates hits: %d, misses: %d | tariff hits: %d, misses: %d",
		cache->rates_hits,cache->rates_misses,cache->tariff_hits,cache->tariff_misses);

	mem_free(cache);
}

rate_t *rt_cache_rates_get(rt_cache_t *cache,unsigned int bplan_id)
{
	unsigned int h;
	rt_cache_rate_entry_t *e;

	if(cache == NULL) return NULL;

	h = rt_cache_hash(bplan_id);
	e = cache->rates[h];

	while(e != NULL) {
		if(e->bplan_id == bplan_id) {
			cache->rates_hits++;
			return e->rates;
		}
		e = e->next;
	}

	cache->rates_misses++;
	return NULL;
}

void rt_cache_rates_put(rt_cache_t *cache,unsigned int bplan_id,rate_t *rates)
{
	unsigned int h;
	rt_cache_rate_entry_t *e;

	if(cache == NULL) return;
	if(rates == NULL) return;

	h = rt_cache_hash(bplan_id);

	e = (rt_cache_rate_entry_t *)mem_alloc(sizeof(rt_cache_rate_entry_t));
	if(e == NULL) return;

	e->bplan_id = bplan_id;
	e->rates = rates;
	e->next = cache->rates[h];
	cache->rates[h] = e;
}

calc_function_t *rt_cache_tariff_get(rt_cache_t *cache,unsigned int tariff_id)
{
	unsigned int h;
	rt_cache_tariff_entry_t *e;

	if(cache == NULL) return NULL;

	h = rt_cache_hash(tariff_id);
	e = cache->tariffs[h];

	while(e != NULL) {
		if(e->tariff_id == tariff_id) {
			cache->tariff_hits++;
			return e->calc_funcs;
		}
		e = e->next;
	}

	cache->tariff_misses++;
	return NULL;
}

void rt_cache_tariff_put(rt_cache_t *cache,unsigned int tariff_id,calc_function_t *funcs)
{
	unsigned int h;
	rt_cache_tariff_entry_t *e;

	if(cache == NULL) return;
	if(funcs == NULL) return;

	h = rt_cache_hash(tariff_id);

	e = (rt_cache_tariff_entry_t *)mem_alloc(sizeof(rt_cache_tariff_entry_t));
	if(e == NULL) return;

	e->tariff_id = tariff_id;
	e->calc_funcs = funcs;
	e->next = cache->tariffs[h];
	cache->tariffs[h] = e;
}
