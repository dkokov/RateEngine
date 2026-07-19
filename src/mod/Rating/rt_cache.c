#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../mem/mem.h"
#include "../../db/db.h"
#include "../../log/rt_log.h"

#include "rt_cache.h"

static unsigned int rt_cache_hash_int(unsigned int key)
{
	return key % RT_CACHE_BUCKETS;
}

static unsigned int rt_cache_hash_str(const char *key)
{
	unsigned int h = 5381;
	int c;

	while((c = *key++)) h = ((h << 5) + h) + c;

	return h % RT_CACHE_BUCKETS;
}

/* entry is usable if it was (re)filled within the last RT_CACHE_TTL seconds */
static int rt_cache_fresh(time_t ts)
{
	return (time(NULL) - ts) <= RT_CACHE_TTL;
}

/* deep-copy the null-terminated rate array (count real rows + 1 terminator).
 * calc_funcs is attached later per call, so it is not carried in the copy. */
static rate_t *rt_cache_rates_dup(rate_t *src,int count)
{
	int i;
	rate_t *dst;

	if(src == NULL || count <= 0) return NULL;

	dst = (rate_t *)mem_alloc_arr(count + 1,sizeof(rate_t));
	if(dst == NULL) return NULL;

	memcpy(dst,src,count * sizeof(rate_t));
	for(i = 0; i < count; i++) dst[i].calc_funcs = NULL;

	return dst;
}

/* deep-copy the null-terminated calc_function array (count rows + 1 terminator) */
static calc_function_t *rt_cache_calc_dup(calc_function_t *src,int count)
{
	calc_function_t *dst;

	if(src == NULL || count <= 0) return NULL;

	dst = (calc_function_t *)mem_alloc_arr(count + 1,sizeof(calc_function_t));
	if(dst == NULL) return NULL;

	memcpy(dst,src,count * sizeof(calc_function_t));

	return dst;
}

/* deep-copy the pcard array (count rows + 1 terminator, id==0) */
static pcard_t *rt_cache_pcard_dup(pcard_t *src,int count)
{
	pcard_t *dst;

	if(src == NULL || count <= 0) return NULL;

	dst = (pcard_t *)mem_alloc_arr(count + 1,sizeof(pcard_t));
	if(dst == NULL) return NULL;

	memcpy(dst,src,count * sizeof(pcard_t));

	return dst;
}

rt_cache_t *rt_cache_init(void)
{
	rt_cache_t *cache;

	cache = (rt_cache_t *)mem_alloc(sizeof(rt_cache_t));
	if(cache != NULL) {
		memset(cache,0,sizeof(rt_cache_t));
		pthread_rwlock_init(&cache->lock,NULL);
	}

	return cache;
}

void rt_cache_free(rt_cache_t *cache)
{
	int i;
	rt_cache_rate_entry_t *re,*re_next;
	rt_cache_tariff_entry_t *te,*te_next;
	rt_cache_racc_entry_t *ae,*ae_next;
	rt_cache_pcard_entry_t *pe,*pe_next;
	rt_cache_tc_entry_t *tce,*tce_next;
	rt_cache_fbs_entry_t *fe,*fe_next;

	if(cache == NULL) return;

	for(i = 0; i < RT_CACHE_BUCKETS; i++) {
		re = cache->rates[i];
		while(re != NULL) {
			re_next = re->next;
			if(re->rates != NULL) mem_free(re->rates);
			mem_free(re);
			re = re_next;
		}
	}

	for(i = 0; i < RT_CACHE_BUCKETS; i++) {
		te = cache->tariffs[i];
		while(te != NULL) {
			te_next = te->next;
			if(te->calc_funcs != NULL) mem_free(te->calc_funcs);
			mem_free(te);
			te = te_next;
		}
	}

	for(i = 0; i < RT_CACHE_BUCKETS; i++) {
		ae = cache->raccs[i];
		while(ae != NULL) {
			ae_next = ae->next;
			mem_free(ae);
			ae = ae_next;
		}
	}

	for(i = 0; i < RT_CACHE_BUCKETS; i++) {
		pe = cache->pcards[i];
		while(pe != NULL) {
			pe_next = pe->next;
			if(pe->cards != NULL) mem_free(pe->cards);
			mem_free(pe);
			pe = pe_next;
		}
	}

	for(i = 0; i < RT_CACHE_BUCKETS; i++) {
		tce = cache->tcs[i];
		while(tce != NULL) {
			tce_next = tce->next;
			mem_free(tce);
			tce = tce_next;
		}
	}

	for(i = 0; i < RT_CACHE_BUCKETS; i++) {
		fe = cache->fbs[i];
		while(fe != NULL) {
			fe_next = fe->next;
			mem_free(fe);
			fe = fe_next;
		}
	}

	LOG("rt_cache_free()","rates h/m: %d/%d | tariff h/m: %d/%d | racc h/m: %d/%d | pcard h/m: %d/%d",
		cache->rates_hits,cache->rates_misses,
		cache->tariff_hits,cache->tariff_misses,
		cache->racc_hits,cache->racc_misses,
		cache->pcard_hits,cache->pcard_misses);

	pthread_rwlock_destroy(&cache->lock);

	mem_free(cache);
}

/* ---- rates cache ---- */

rate_t *rt_cache_rates_get(rt_cache_t *cache,unsigned int bplan_id)
{
	unsigned int h;
	rt_cache_rate_entry_t *e;
	rate_t *copy = NULL;

	if(cache == NULL) return NULL;

	h = rt_cache_hash_int(bplan_id);

	pthread_rwlock_rdlock(&cache->lock);
	e = cache->rates[h];
	while(e != NULL) {
		if(e->bplan_id == bplan_id) {
			if(rt_cache_fresh(e->ts)) copy = rt_cache_rates_dup(e->rates,e->count);
			break;
		}
		e = e->next;
	}
	pthread_rwlock_unlock(&cache->lock);

	if(copy != NULL) __atomic_fetch_add(&cache->rates_hits,1,__ATOMIC_RELAXED);
	else __atomic_fetch_add(&cache->rates_misses,1,__ATOMIC_RELAXED);

	return copy;
}

void rt_cache_rates_put(rt_cache_t *cache,unsigned int bplan_id,rate_t *rates,int count)
{
	unsigned int h;
	rt_cache_rate_entry_t *e;
	rate_t *copy;

	if(cache == NULL || rates == NULL || count <= 0) return;

	/* copy outside the lock to keep the write section short */
	copy = rt_cache_rates_dup(rates,count);
	if(copy == NULL) return;

	h = rt_cache_hash_int(bplan_id);

	pthread_rwlock_wrlock(&cache->lock);

	/* replace in place if the key already exists (TTL refresh) */
	for(e = cache->rates[h]; e != NULL; e = e->next) {
		if(e->bplan_id == bplan_id) {
			if(e->rates != NULL) mem_free(e->rates);
			e->rates = copy;
			e->count = count;
			e->ts = time(NULL);
			pthread_rwlock_unlock(&cache->lock);
			return;
		}
	}

	e = (rt_cache_rate_entry_t *)mem_alloc(sizeof(rt_cache_rate_entry_t));
	if(e == NULL) {
		pthread_rwlock_unlock(&cache->lock);
		mem_free(copy);
		return;
	}

	e->bplan_id = bplan_id;
	e->rates = copy;
	e->count = count;
	e->ts = time(NULL);
	e->next = cache->rates[h];
	cache->rates[h] = e;

	pthread_rwlock_unlock(&cache->lock);
}

/* ---- tariff cache ---- */

calc_function_t *rt_cache_tariff_get(rt_cache_t *cache,unsigned int tariff_id)
{
	unsigned int h;
	rt_cache_tariff_entry_t *e;
	calc_function_t *copy = NULL;

	if(cache == NULL) return NULL;

	h = rt_cache_hash_int(tariff_id);

	pthread_rwlock_rdlock(&cache->lock);
	e = cache->tariffs[h];
	while(e != NULL) {
		if(e->tariff_id == tariff_id) {
			if(rt_cache_fresh(e->ts)) copy = rt_cache_calc_dup(e->calc_funcs,e->count);
			break;
		}
		e = e->next;
	}
	pthread_rwlock_unlock(&cache->lock);

	if(copy != NULL) __atomic_fetch_add(&cache->tariff_hits,1,__ATOMIC_RELAXED);
	else __atomic_fetch_add(&cache->tariff_misses,1,__ATOMIC_RELAXED);

	return copy;
}

void rt_cache_tariff_put(rt_cache_t *cache,unsigned int tariff_id,calc_function_t *funcs,int count)
{
	unsigned int h;
	rt_cache_tariff_entry_t *e;
	calc_function_t *copy;

	if(cache == NULL || funcs == NULL || count <= 0) return;

	copy = rt_cache_calc_dup(funcs,count);
	if(copy == NULL) return;

	h = rt_cache_hash_int(tariff_id);

	pthread_rwlock_wrlock(&cache->lock);

	for(e = cache->tariffs[h]; e != NULL; e = e->next) {
		if(e->tariff_id == tariff_id) {
			if(e->calc_funcs != NULL) mem_free(e->calc_funcs);
			e->calc_funcs = copy;
			e->count = count;
			e->ts = time(NULL);
			pthread_rwlock_unlock(&cache->lock);
			return;
		}
	}

	e = (rt_cache_tariff_entry_t *)mem_alloc(sizeof(rt_cache_tariff_entry_t));
	if(e == NULL) {
		pthread_rwlock_unlock(&cache->lock);
		mem_free(copy);
		return;
	}

	e->tariff_id = tariff_id;
	e->calc_funcs = copy;
	e->count = count;
	e->ts = time(NULL);
	e->next = cache->tariffs[h];
	cache->tariffs[h] = e;

	pthread_rwlock_unlock(&cache->lock);
}

/* ---- subscriber (racc) cache ---- */

int rt_cache_racc_get(rt_cache_t *cache,const char *key,rt_cache_racc_data_t *out)
{
	unsigned int h;
	rt_cache_racc_entry_t *e;
	int hit = 0;

	if(cache == NULL || key == NULL || out == NULL) return 0;

	h = rt_cache_hash_str(key);

	pthread_rwlock_rdlock(&cache->lock);
	e = cache->raccs[h];
	while(e != NULL) {
		if(strcmp(e->key,key) == 0) {
			if(rt_cache_fresh(e->ts)) {
				memcpy(out,&e->data,sizeof(rt_cache_racc_data_t));
				hit = 1;
			}
			break;
		}
		e = e->next;
	}
	pthread_rwlock_unlock(&cache->lock);

	if(hit) __atomic_fetch_add(&cache->racc_hits,1,__ATOMIC_RELAXED);
	else __atomic_fetch_add(&cache->racc_misses,1,__ATOMIC_RELAXED);

	return hit;
}

void rt_cache_racc_put(rt_cache_t *cache,const char *key,rt_cache_racc_data_t *data)
{
	unsigned int h;
	rt_cache_racc_entry_t *e;

	if(cache == NULL || key == NULL || data == NULL) return;

	h = rt_cache_hash_str(key);

	pthread_rwlock_wrlock(&cache->lock);

	for(e = cache->raccs[h]; e != NULL; e = e->next) {
		if(strcmp(e->key,key) == 0) {
			memcpy(&e->data,data,sizeof(rt_cache_racc_data_t));
			e->ts = time(NULL);
			pthread_rwlock_unlock(&cache->lock);
			return;
		}
	}

	e = (rt_cache_racc_entry_t *)mem_alloc(sizeof(rt_cache_racc_entry_t));
	if(e == NULL) {
		pthread_rwlock_unlock(&cache->lock);
		return;
	}

	strncpy(e->key,key,sizeof(e->key)-1);
	memcpy(&e->data,data,sizeof(rt_cache_racc_data_t));
	e->ts = time(NULL);
	e->next = cache->raccs[h];
	cache->raccs[h] = e;

	pthread_rwlock_unlock(&cache->lock);
}

/* ---- pcard cache ---- */

pcard_t *rt_cache_pcard_get(rt_cache_t *cache,unsigned int bacc_id,int *count)
{
	unsigned int h;
	rt_cache_pcard_entry_t *e;
	pcard_t *copy = NULL;
	int cnt = 0;

	if(cache == NULL) return NULL;

	h = rt_cache_hash_int(bacc_id);

	pthread_rwlock_rdlock(&cache->lock);
	e = cache->pcards[h];
	while(e != NULL) {
		if(e->bacc_id == bacc_id) {
			if(rt_cache_fresh(e->ts)) {
				copy = rt_cache_pcard_dup(e->cards,e->count);
				cnt = e->count;
			}
			break;
		}
		e = e->next;
	}
	pthread_rwlock_unlock(&cache->lock);

	if(copy != NULL) {
		if(count != NULL) *count = cnt;
		__atomic_fetch_add(&cache->pcard_hits,1,__ATOMIC_RELAXED);
	} else __atomic_fetch_add(&cache->pcard_misses,1,__ATOMIC_RELAXED);

	return copy;
}

void rt_cache_pcard_put(rt_cache_t *cache,unsigned int bacc_id,pcard_t *cards,int count)
{
	unsigned int h;
	rt_cache_pcard_entry_t *e;
	pcard_t *copy;

	if(cache == NULL || cards == NULL || count <= 0) return;

	copy = rt_cache_pcard_dup(cards,count);
	if(copy == NULL) return;

	h = rt_cache_hash_int(bacc_id);

	pthread_rwlock_wrlock(&cache->lock);

	for(e = cache->pcards[h]; e != NULL; e = e->next) {
		if(e->bacc_id == bacc_id) {
			if(e->cards != NULL) mem_free(e->cards);
			e->cards = copy;
			e->count = count;
			e->ts = time(NULL);
			pthread_rwlock_unlock(&cache->lock);
			return;
		}
	}

	e = (rt_cache_pcard_entry_t *)mem_alloc(sizeof(rt_cache_pcard_entry_t));
	if(e == NULL) {
		pthread_rwlock_unlock(&cache->lock);
		mem_free(copy);
		return;
	}

	e->bacc_id = bacc_id;
	e->cards = copy;
	e->count = count;
	e->ts = time(NULL);
	e->next = cache->pcards[h];
	cache->pcards[h] = e;

	pthread_rwlock_unlock(&cache->lock);
}

/* ---- time condition cache ---- */

int rt_cache_tc_get(rt_cache_t *cache,unsigned int tariff_id,int *has_tc)
{
	unsigned int h;
	rt_cache_tc_entry_t *e;
	int hit = 0;

	if(cache == NULL) return 0;

	h = rt_cache_hash_int(tariff_id);

	pthread_rwlock_rdlock(&cache->lock);
	e = cache->tcs[h];
	while(e != NULL) {
		if(e->tariff_id == tariff_id) {
			if(rt_cache_fresh(e->ts)) { *has_tc = e->has_tc; hit = 1; }
			break;
		}
		e = e->next;
	}
	pthread_rwlock_unlock(&cache->lock);

	return hit;
}

void rt_cache_tc_put(rt_cache_t *cache,unsigned int tariff_id,int has_tc)
{
	unsigned int h;
	rt_cache_tc_entry_t *e;

	if(cache == NULL) return;

	h = rt_cache_hash_int(tariff_id);

	pthread_rwlock_wrlock(&cache->lock);

	for(e = cache->tcs[h]; e != NULL; e = e->next) {
		if(e->tariff_id == tariff_id) {
			e->has_tc = has_tc;
			e->ts = time(NULL);
			pthread_rwlock_unlock(&cache->lock);
			return;
		}
	}

	e = (rt_cache_tc_entry_t *)mem_alloc(sizeof(rt_cache_tc_entry_t));
	if(e == NULL) {
		pthread_rwlock_unlock(&cache->lock);
		return;
	}

	e->tariff_id = tariff_id;
	e->has_tc = has_tc;
	e->ts = time(NULL);
	e->next = cache->tcs[h];
	cache->tcs[h] = e;

	pthread_rwlock_unlock(&cache->lock);
}

/* ---- free billsec limit cache ---- */

int rt_cache_fbs_get(rt_cache_t *cache,unsigned int tariff_id,int *limit)
{
	unsigned int h;
	rt_cache_fbs_entry_t *e;
	int hit = 0;

	if(cache == NULL) return 0;

	h = rt_cache_hash_int(tariff_id);

	pthread_rwlock_rdlock(&cache->lock);
	e = cache->fbs[h];
	while(e != NULL) {
		if(e->tariff_id == tariff_id) {
			if(rt_cache_fresh(e->ts)) { *limit = e->free_billsec_limit; hit = 1; }
			break;
		}
		e = e->next;
	}
	pthread_rwlock_unlock(&cache->lock);

	return hit;
}

void rt_cache_fbs_put(rt_cache_t *cache,unsigned int tariff_id,int limit)
{
	unsigned int h;
	rt_cache_fbs_entry_t *e;

	if(cache == NULL) return;

	h = rt_cache_hash_int(tariff_id);

	pthread_rwlock_wrlock(&cache->lock);

	for(e = cache->fbs[h]; e != NULL; e = e->next) {
		if(e->tariff_id == tariff_id) {
			e->free_billsec_limit = limit;
			e->queried = 1;
			e->ts = time(NULL);
			pthread_rwlock_unlock(&cache->lock);
			return;
		}
	}

	e = (rt_cache_fbs_entry_t *)mem_alloc(sizeof(rt_cache_fbs_entry_t));
	if(e == NULL) {
		pthread_rwlock_unlock(&cache->lock);
		return;
	}

	e->tariff_id = tariff_id;
	e->free_billsec_limit = limit;
	e->queried = 1;
	e->ts = time(NULL);
	e->next = cache->fbs[h];
	cache->fbs[h] = e;

	pthread_rwlock_unlock(&cache->lock);
}

/* ---- preload: batch load subscribers from CDR batch ---- */

int rt_cache_preload_raccs(rt_cache_t *cache,db_t *dbp,char **numbers,int count,int cdr_server_id)
{
	int i,c,loaded;
	char *in_clause;
	char *sql_buf;
	char safe[CLG_LEN * 2];
	db_sql_result_t *result;
	rt_cache_racc_data_t data;

	if(cache == NULL || dbp == NULL || numbers == NULL || count <= 0) {
		LOG("rt_cache_preload_raccs()","skip: cache=%p,dbp=%p,numbers=%p,count=%d",cache,dbp,numbers,count);
		return 0;
	}

	/* build IN clause from unique calling numbers */
	in_clause = (char *)mem_alloc(count * (CLG_LEN + 4));
	if(in_clause == NULL) return -1;

	in_clause[0] = '\0';
	c = 0;

	for(i = 0; i < count; i++) {
		if(numbers[i] == NULL || strlen(numbers[i]) == 0) continue;

		/* skip if already in cache */
		char key[128];
		rt_cache_racc_data_t seen;
		sprintf(key,"%d:%s",rt_mode_clg,numbers[i]);
		if(rt_cache_racc_get(cache,key,&seen)) continue;

		/* check if already in IN clause (simple dedup) */
		db_sql_escape(numbers[i],safe,sizeof(safe));

		char needle[CLG_LEN + 4];
		sprintf(needle,"'%s'",safe);

		if(strstr(in_clause,needle) != NULL) continue;

		if(c > 0) strcat(in_clause,",");
		strcat(in_clause,needle);
		c++;
	}

	if(c == 0) {
		LOG("rt_cache_preload_raccs()","no unique numbers to preload (count=%d)",count);
		mem_free(in_clause);
		return 0;
	}

	LOG("rt_cache_preload_raccs()","preloading %d unique numbers ...",c);

	/* batch query */
	sql_buf = (char *)mem_alloc(strlen(in_clause) + 512);
	if(sql_buf == NULL) { mem_free(in_clause); return -1; }

	sprintf(sql_buf,
		"SELECT clg.calling_number,clg.billing_account_id,clg_df.bill_plan_id,"
		"bp.start_period,bp.end_period,bacc.billing_day,bacc.round_mode_id,bacc.day_of_payment "
		"FROM calling_number AS clg,calling_number_deff AS clg_df,bill_plan AS bp,billing_account AS bacc "
		"WHERE clg.id = clg_df.calling_number_id AND bp.id = clg_df.bill_plan_id "
		"AND bacc.id = clg.billing_account_id AND bacc.cdr_server_id = %d "
		"AND clg.calling_number IN (%s)",cdr_server_id,in_clause);

	{
//		LOG("debug","sql: %s", sql_buf);
		int sel_ret = db_select(dbp,sql_buf);
		int fetch_ret = db_fetch(dbp);
		LOG("rt_cache_preload_raccs()","select=%d, fetch=%d, result=%p",sel_ret,fetch_ret,dbp->conn->result);
	}

	loaded = 0;

	if(dbp->conn->result != NULL) {
		result = (db_sql_result_t *)dbp->conn->result;

		for(i = 0; i < result->rows; i++) {
			char key[128];
			sprintf(key,"%d:%s",rt_mode_clg,result->cols_list[0].rows_list[i].row);

			memset(&data,0,sizeof(data));
			data.bacc_id = atoi(result->cols_list[1].rows_list[i].row);
			data.bplan_id = atoi(result->cols_list[2].rows_list[i].row);
			data.bplan_start_period = atoi(result->cols_list[3].rows_list[i].row);
			data.bplan_end_period = atoi(result->cols_list[4].rows_list[i].row);
			data.billing_day = atoi(result->cols_list[5].rows_list[i].row);
			data.round_mode_id = atoi(result->cols_list[6].rows_list[i].row);
			data.day_of_payment = atoi(result->cols_list[7].rows_list[i].row);

			rt_cache_racc_put(cache,key,&data);
			loaded++;
		}

		db_sql_result_free(result);
		dbp->conn->result = NULL;
	}

	mem_free(sql_buf);
	mem_free(in_clause);

	LOG("rt_cache_preload_raccs()","unique: %d, loaded: %d",c,loaded);

	return loaded;
}

int rt_cache_preload_pcards(rt_cache_t *cache,db_t *dbp)
{
	int i,loaded;
	char *in_clause;
	char *sql_buf;
	db_sql_result_t *result;

	unsigned int bacc_ids[RT_CACHE_BUCKETS * 4];
	int bacc_count = 0;

	if(cache == NULL || dbp == NULL) return 0;

	/* collect unique bacc_ids from racc cache */
	for(i = 0; i < RT_CACHE_BUCKETS; i++) {
		rt_cache_racc_entry_t *e = cache->raccs[i];
		while(e != NULL) {
			if(e->data.bacc_id > 0) {
				/* simple dedup */
				int dup = 0, j;
				for(j = 0; j < bacc_count; j++) {
					if(bacc_ids[j] == e->data.bacc_id) { dup = 1; break; }
				}
				if(!dup && bacc_count < (RT_CACHE_BUCKETS * 4)) {
					bacc_ids[bacc_count++] = e->data.bacc_id;
				}
			}
			e = e->next;
		}
	}

	if(bacc_count == 0) return 0;

	/* build IN clause */
	in_clause = (char *)mem_alloc(bacc_count * 12);
	if(in_clause == NULL) return -1;

	in_clause[0] = '\0';
	for(i = 0; i < bacc_count; i++) {
		char tmp[16];
		sprintf(tmp,"%s%d",(i > 0 ? "," : ""),bacc_ids[i]);
		strcat(in_clause,tmp);
	}

	/* batch query */
	sql_buf = (char *)mem_alloc(strlen(in_clause) + 256);
	if(sql_buf == NULL) { mem_free(in_clause); return -1; }

	sprintf(sql_buf,
		"SELECT id,amount,start_date,end_date,pcard_status_id,pcard_type_id,call_number,billing_account_id "
		"FROM pcard WHERE billing_account_id IN (%s) AND pcard_status_id = 1 "
		"ORDER BY billing_account_id,start_date DESC",in_clause);

	db_select(dbp,sql_buf);
	db_fetch(dbp);

	loaded = 0;

	if(dbp->conn->result != NULL) {
		result = (db_sql_result_t *)dbp->conn->result;

		if(result->rows > 0) {
			/* group pcards by billing_account_id */
			unsigned int cur_bacc = 0;
			int start = 0;

			for(i = 0; i <= result->rows; i++) {
				unsigned int this_bacc = 0;

				if(i < result->rows)
					this_bacc = atoi(result->cols_list[7].rows_list[i].row);

				if(i > 0 && (this_bacc != cur_bacc || i == result->rows)) {
					/* flush group [start..i-1] for cur_bacc */
					int cnt = i - start;
					pcard_t *cards = (pcard_t *)mem_alloc_arr(cnt + 1,sizeof(pcard_t));

					if(cards != NULL) {
						int k;
						for(k = 0; k < cnt; k++) {
							int r = start + k;
							cards[k].id = atoi(result->cols_list[0].rows_list[r].row);
							cards[k].amount = atof(result->cols_list[1].rows_list[r].row);
							strncpy(cards[k].start,result->cols_list[2].rows_list[r].row,DATE_LEN-1);
							strncpy(cards[k].end,result->cols_list[3].rows_list[r].row,DATE_LEN-1);
							cards[k].status = atoi(result->cols_list[4].rows_list[r].row);
							cards[k].type = atoi(result->cols_list[5].rows_list[r].row);
							cards[k].call_number = atoi(result->cols_list[6].rows_list[r].row);
						}

						rt_cache_pcard_put(cache,cur_bacc,cards,cnt);
						mem_free(cards);   /* put() stores its own copy */
						loaded++;
					}

					start = i;
				}

				cur_bacc = this_bacc;
			}
		}

		db_sql_result_free(result);
		dbp->conn->result = NULL;
	}

	mem_free(sql_buf);
	mem_free(in_clause);

	LOG("rt_cache_preload_pcards()","bacc_ids: %d, loaded: %d",bacc_count,loaded);

	return loaded;
}
