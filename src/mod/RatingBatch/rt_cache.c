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

	mem_free(cache);
}

/* ---- rates cache ---- */

rate_t *rt_cache_rates_get(rt_cache_t *cache,unsigned int bplan_id)
{
	unsigned int h;
	rt_cache_rate_entry_t *e;

	if(cache == NULL) return NULL;

	h = rt_cache_hash_int(bplan_id);
	e = cache->rates[h];

	while(e != NULL) {
		if(e->bplan_id == bplan_id) { cache->rates_hits++; return e->rates; }
		e = e->next;
	}

	cache->rates_misses++;
	return NULL;
}

void rt_cache_rates_put(rt_cache_t *cache,unsigned int bplan_id,rate_t *rates)
{
	unsigned int h;
	rt_cache_rate_entry_t *e;

	if(cache == NULL || rates == NULL) return;

	h = rt_cache_hash_int(bplan_id);
	e = (rt_cache_rate_entry_t *)mem_alloc(sizeof(rt_cache_rate_entry_t));
	if(e == NULL) return;

	e->bplan_id = bplan_id;
	e->rates = rates;
	e->next = cache->rates[h];
	cache->rates[h] = e;
}

/* ---- tariff cache ---- */

calc_function_t *rt_cache_tariff_get(rt_cache_t *cache,unsigned int tariff_id)
{
	unsigned int h;
	rt_cache_tariff_entry_t *e;

	if(cache == NULL) return NULL;

	h = rt_cache_hash_int(tariff_id);
	e = cache->tariffs[h];

	while(e != NULL) {
		if(e->tariff_id == tariff_id) { cache->tariff_hits++; return e->calc_funcs; }
		e = e->next;
	}

	cache->tariff_misses++;
	return NULL;
}

void rt_cache_tariff_put(rt_cache_t *cache,unsigned int tariff_id,calc_function_t *funcs)
{
	unsigned int h;
	rt_cache_tariff_entry_t *e;

	if(cache == NULL || funcs == NULL) return;

	h = rt_cache_hash_int(tariff_id);
	e = (rt_cache_tariff_entry_t *)mem_alloc(sizeof(rt_cache_tariff_entry_t));
	if(e == NULL) return;

	e->tariff_id = tariff_id;
	e->calc_funcs = funcs;
	e->next = cache->tariffs[h];
	cache->tariffs[h] = e;
}

/* ---- subscriber (racc) cache ---- */

rt_cache_racc_data_t *rt_cache_racc_get(rt_cache_t *cache,const char *key)
{
	unsigned int h;
	rt_cache_racc_entry_t *e;

	if(cache == NULL || key == NULL) return NULL;

	h = rt_cache_hash_str(key);
	e = cache->raccs[h];

	while(e != NULL) {
		if(strcmp(e->key,key) == 0) { cache->racc_hits++; return &e->data; }
		e = e->next;
	}

	cache->racc_misses++;
	return NULL;
}

void rt_cache_racc_put(rt_cache_t *cache,const char *key,rt_cache_racc_data_t *data)
{
	unsigned int h;
	rt_cache_racc_entry_t *e;

	if(cache == NULL || key == NULL || data == NULL) return;

	h = rt_cache_hash_str(key);
	e = (rt_cache_racc_entry_t *)mem_alloc(sizeof(rt_cache_racc_entry_t));
	if(e == NULL) return;

	strncpy(e->key,key,sizeof(e->key)-1);
	memcpy(&e->data,data,sizeof(rt_cache_racc_data_t));
	e->next = cache->raccs[h];
	cache->raccs[h] = e;
}

/* ---- pcard cache ---- */

rt_cache_pcard_entry_t *rt_cache_pcard_get(rt_cache_t *cache,unsigned int bacc_id)
{
	unsigned int h;
	rt_cache_pcard_entry_t *e;

	if(cache == NULL) return NULL;

	h = rt_cache_hash_int(bacc_id);
	e = cache->pcards[h];

	while(e != NULL) {
		if(e->bacc_id == bacc_id) { cache->pcard_hits++; return e; }
		e = e->next;
	}

	cache->pcard_misses++;
	return NULL;
}

void rt_cache_pcard_put(rt_cache_t *cache,unsigned int bacc_id,pcard_t *cards,int count)
{
	unsigned int h;
	rt_cache_pcard_entry_t *e;

	if(cache == NULL) return;

	h = rt_cache_hash_int(bacc_id);
	e = (rt_cache_pcard_entry_t *)mem_alloc(sizeof(rt_cache_pcard_entry_t));
	if(e == NULL) return;

	e->bacc_id = bacc_id;
	e->cards = cards;
	e->count = count;
	e->next = cache->pcards[h];
	cache->pcards[h] = e;
}

/* ---- time condition cache ---- */

int rt_cache_tc_get(rt_cache_t *cache,unsigned int tariff_id,int *has_tc)
{
	unsigned int h;
	rt_cache_tc_entry_t *e;

	if(cache == NULL) return 0;

	h = rt_cache_hash_int(tariff_id);
	e = cache->tcs[h];

	while(e != NULL) {
		if(e->tariff_id == tariff_id) { *has_tc = e->has_tc; return 1; }
		e = e->next;
	}

	return 0;
}

void rt_cache_tc_put(rt_cache_t *cache,unsigned int tariff_id,int has_tc)
{
	unsigned int h;
	rt_cache_tc_entry_t *e;

	if(cache == NULL) return;

	h = rt_cache_hash_int(tariff_id);
	e = (rt_cache_tc_entry_t *)mem_alloc(sizeof(rt_cache_tc_entry_t));
	if(e == NULL) return;

	e->tariff_id = tariff_id;
	e->has_tc = has_tc;
	e->next = cache->tcs[h];
	cache->tcs[h] = e;
}

/* ---- free billsec limit cache ---- */

int rt_cache_fbs_get(rt_cache_t *cache,unsigned int tariff_id,int *limit)
{
	unsigned int h;
	rt_cache_fbs_entry_t *e;

	if(cache == NULL) return 0;

	h = rt_cache_hash_int(tariff_id);
	e = cache->fbs[h];

	while(e != NULL) {
		if(e->tariff_id == tariff_id) { *limit = e->free_billsec_limit; return 1; }
		e = e->next;
	}

	return 0;
}

void rt_cache_fbs_put(rt_cache_t *cache,unsigned int tariff_id,int limit)
{
	unsigned int h;
	rt_cache_fbs_entry_t *e;

	if(cache == NULL) return;

	h = rt_cache_hash_int(tariff_id);
	e = (rt_cache_fbs_entry_t *)mem_alloc(sizeof(rt_cache_fbs_entry_t));
	if(e == NULL) return;

	e->tariff_id = tariff_id;
	e->free_billsec_limit = limit;
	e->queried = 1;
	e->next = cache->fbs[h];
	cache->fbs[h] = e;
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
		sprintf(key,"%d:%s",rt_mode_clg,numbers[i]);
		if(rt_cache_racc_get(cache,key) != NULL) continue;

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
