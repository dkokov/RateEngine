#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../db/db.h"
#include "../../mem/mem.h"
#include "../../log/rt_log.h"

#include "rt_batch_q.h"

/* max SQL buffer for batch operations */
#define BATCH_SQL_BUF (1024 * 256)

int rt_batch_insert_ratings(db_t *dbp,rt_rated_t *results,int count)
{
	int i,inserted;
	char *sql;
	char safe_ts[64];
	char row[512];
	db_sql_result_t *result;

	if(dbp == NULL || results == NULL || count <= 0) return 0;

	/* count how many need inserting (status == 0) */
	inserted = 0;
	for(i = 0; i < count; i++) {
		if(results[i].status == 0 && results[i].bacc_id > 0) inserted++;
	}

	if(inserted == 0) return 0;

	sql = (char *)mem_alloc(BATCH_SQL_BUF);
	if(sql == NULL) return -1;

	/* build multi-row INSERT ... RETURNING id, call_id */
	strcpy(sql,
		"INSERT INTO rating"
		" (call_price,call_billsec,rate_id,billing_account_id,call_id,"
		"rating_mode_id,pcard_id,time_condition_id,call_ts,last_update,free_billsec_id)"
		" VALUES ");

	int first = 1;
	for(i = 0; i < count; i++) {
		if(results[i].status != 0 || results[i].bacc_id == 0) continue;

		db_sql_escape(results[i].timestamp,safe_ts,sizeof(safe_ts));

		snprintf(row,sizeof(row),
			"%s(%f,%d,%d,%d,%d,%d,%d,%d,'%s','now()',%d)",
			(first ? "" : ","),
			results[i].cprice,results[i].billsec,results[i].rate_id,
			results[i].bacc_id,results[i].cdr_id,results[i].rating_mode_id,
			results[i].pcard_id,results[i].tc_id,safe_ts,results[i].free_billsec_id);

		if(strlen(sql) + strlen(row) >= BATCH_SQL_BUF - 100) {
			LOG("rt_batch_insert_ratings()","SQL buffer full at %d/%d rows",i,inserted);
			break;
		}

		strcat(sql,row);
		first = 0;
	}

	strcat(sql," RETURNING id, call_id");

	/* execute */
	db_query(dbp,sql,0);
	db_fetch(dbp);

	if(dbp->conn->result != NULL) {
		result = (db_sql_result_t *)dbp->conn->result;

		for(i = 0; i < result->rows; i++) {
			int new_id = atoi(result->cols_list[0].rows_list[i].row);
			int cdr_id = atoi(result->cols_list[1].rows_list[i].row);

			/* map back to results array */
			int j;
			for(j = 0; j < count; j++) {
				if(results[j].cdr_id == cdr_id && results[j].status == 0) {
					results[j].rating_id = new_id;
					break;
				}
			}
		}

		inserted = result->rows;

		db_sql_result_free(result);
		dbp->conn->result = NULL;
	}

	mem_free(sql);

	return inserted;
}

int rt_batch_update_cdrs(db_t *dbp,rt_rated_t *results,int count,char leg)
{
	int i,updated;
	char *sql;
	char row[64];

	if(dbp == NULL || results == NULL || count <= 0) return 0;

	sql = (char *)mem_alloc(BATCH_SQL_BUF);
	if(sql == NULL) return -1;

	/* build: UPDATE cdrs SET leg_X = v.rid FROM (VALUES ...) AS v(cid,rid) WHERE cdrs.id = v.cid */
	sprintf(sql,
		"UPDATE cdrs SET leg_%c = v.rid FROM (VALUES ",leg);

	updated = 0;
	for(i = 0; i < count; i++) {
		int rid;

		if(results[i].rating_id > 0) rid = results[i].rating_id;
		else rid = -1;

		snprintf(row,sizeof(row),"%s(%d,%d)",
			(updated > 0 ? "," : ""),
			results[i].cdr_id,rid);

		if(strlen(sql) + strlen(row) >= BATCH_SQL_BUF - 100) break;

		strcat(sql,row);
		updated++;
	}

	strcat(sql,") AS v(cid,rid) WHERE cdrs.id = v.cid");

	db_query(dbp,sql,1);

	mem_free(sql);

	return updated;
}

int rt_batch_update_balances(db_t *dbp,rt_rated_t *results,int count)
{
	int i,j;
	char sql[2048];

	/* aggregate cprice per bacc_id */
	typedef struct { unsigned int bacc_id; double total; } bacc_sum_t;

	bacc_sum_t *sums = (bacc_sum_t *)mem_alloc(count * sizeof(bacc_sum_t));
	if(sums == NULL) return -1;

	int num_sums = 0;

	for(i = 0; i < count; i++) {
		if(results[i].rating_id <= 0 || results[i].bacc_id == 0) continue;

		/* find existing or add new */
		int found = 0;
		for(j = 0; j < num_sums; j++) {
			if(sums[j].bacc_id == results[i].bacc_id) {
				sums[j].total += results[i].cprice;
				found = 1;
				break;
			}
		}

		if(!found) {
			sums[num_sums].bacc_id = results[i].bacc_id;
			sums[num_sums].total = results[i].cprice;
			num_sums++;
		}
	}

	/* update each balance */
	for(i = 0; i < num_sums; i++) {
		sprintf(sql,
			"UPDATE balance SET amount = amount + %f, last_update = now() "
			"WHERE billing_account_id = %d AND active = 't'",
			sums[i].total,sums[i].bacc_id);

		db_query(dbp,sql,1);
	}

	mem_free(sums);

	LOG("rt_batch_update_balances()","updated %d balances",num_sums);

	return num_sums;
}
