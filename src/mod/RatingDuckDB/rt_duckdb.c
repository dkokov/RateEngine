#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "rt_duckdb.h"

/* run a statement on the DuckDB engine; clear the native result afterwards */
static int rt_duckdb_exec(rt_duckdb_t *ctx,const char *sql)
{
	if(db_query(ctx->duck,(char *)sql,1) < 0) {
		LOG("rt_duckdb_exec()","error on: %s",sql);
		return -1;
	}

	return 0;
}

int rt_duckdb_init(rt_duckdb_t *ctx,const char *pg_host,const char *pg_dbname,
                   const char *pg_user,const char *pg_pass,int pg_port)
{
	char sql[1024];

	if(ctx == NULL) return -1;

	memset(ctx,0,sizeof(rt_duckdb_t));

	/* open an in-memory DuckDB through the db_duckdb engine (duckdb.so) */
	ctx->duck = db_init();
	if(ctx->duck == NULL) return -1;

	ctx->duck->conn = db_conn_init("duckdb",(char *)pg_host,":memory:",
	                               (unsigned short)pg_port,(char *)pg_user,(char *)pg_pass,0);
	if(ctx->duck->conn == NULL) return -1;

	if(db_engine_bind(ctx->duck) < 0) {
		LOG("rt_duckdb_init()","cannot bind duckdb engine (is duckdb.so loaded?)");
		return -1;
	}

	if(db_connect(ctx->duck) < 0) {
		LOG("rt_duckdb_init()","cannot open DuckDB");
		return -1;
	}

	/* build PostgreSQL connection string */
	snprintf(ctx->pg_connstr,sizeof(ctx->pg_connstr),
		"host=%s dbname=%s user=%s password=%s port=%d",
		pg_host,pg_dbname,pg_user,pg_pass,pg_port);

	/* install and load postgres_scanner */
	rt_duckdb_exec(ctx,"INSTALL postgres;");
	rt_duckdb_exec(ctx,"LOAD postgres;");

	/* attach PostgreSQL database */
	snprintf(sql,sizeof(sql),
		"ATTACH '%s' AS pg (TYPE POSTGRES, READ_ONLY);",ctx->pg_connstr);

	if(rt_duckdb_exec(ctx,sql) < 0) {
		LOG("rt_duckdb_init()","cannot attach PostgreSQL");
		return -2;
	}

	LOG("rt_duckdb_init()","DuckDB initialized, PostgreSQL attached");

	return 0;
}

void rt_duckdb_close(rt_duckdb_t *ctx)
{
	if(ctx == NULL || ctx->duck == NULL) return;

	rt_duckdb_exec(ctx,"DETACH pg;");

	db_close(ctx->duck);
	db_free(ctx->duck);
	ctx->duck = NULL;

	LOG("rt_duckdb_close()","DuckDB closed");
}

/* run a SELECT on DuckDB and hand back the marshaled result (rows of strings).
 * Caller must db_sql_result_free() it and null conn->result when done. */
static db_sql_result_t *rt_duckdb_select(rt_duckdb_t *ctx,const char *sql)
{
	if(db_query(ctx->duck,(char *)sql,0) < 0) return NULL;
	if(db_fetch(ctx->duck) < 0) return NULL;

	return (db_sql_result_t *)ctx->duck->conn->result;
}

/*
 * The big query:
 * 1. Pin a deterministic window of unrated CDRs from PostgreSQL (postgres_scanner)
 * 2. JOIN with subscriber, billing plan (via bill_plan_tree), rates, tariff
 * 3. Calculate prices in DuckDB SQL
 * 4. Write rated rows back to PostgreSQL, mark the rest of the window as -1
 */
int rt_duckdb_rate_batch(rt_duckdb_t *ctx,db_t *pg_dbp,char leg,int limit)
{
	char sql[4096];
	int row_count = 0;
	int rated = 0;
	long long max_window_id = 0;

	db_sql_result_t *rb;

	if(ctx == NULL || pg_dbp == NULL) return 0;

	/*
	 * STEP 0: pin a deterministic batch window (the lowest `limit` unrated CDR
	 * ids). Both the rating JOIN and the unmatched (-1) marking work on exactly
	 * this set, so we never mark a CDR we did not actually evaluate.
	 */
	snprintf(sql,sizeof(sql),
		"CREATE OR REPLACE TEMP TABLE batch_window AS "
		"SELECT id, calling_number, called_number, billsec, start_ts, cdr_server_id "
		"FROM pg.cdrs WHERE leg_%c = 0 ORDER BY id LIMIT %d",
		leg,limit);

	if(rt_duckdb_exec(ctx,sql) < 0) {
		LOG("rt_duckdb_rate_batch()","build batch_window failed");
		return -1;
	}

	/* window bounds: is there work left, and the max id to scope the -1 mark */
	{
		db_sql_result_t *wr =
			rt_duckdb_select(ctx,"SELECT COUNT(*), COALESCE(MAX(id),0) FROM batch_window");
		long long win_count;

		if(wr == NULL || wr->rows < 1) {
			if(wr != NULL) { db_sql_result_free(wr); ctx->duck->conn->result = NULL; }
			return -1;
		}

		win_count     = atoll(wr->cols_list[0].rows_list[0].row);
		max_window_id = atoll(wr->cols_list[1].rows_list[0].row);

		db_sql_result_free(wr);
		ctx->duck->conn->result = NULL;

		if(win_count == 0) return 0;   /* backlog drained */
	}

	/*
	 * STEP 1: Big JOIN over the window - match CDRs to subscribers, rates, tariffs.
	 * Rates are reached through bill_plan_tree (nested plans); COALESCE falls back
	 * to the plan itself for flat (non-nested) plans - mirrors rt_data_q rate sql.
	 * Longest matching prefix wins via
	 * ROW_NUMBER() OVER (PARTITION BY cdr_id ORDER BY LENGTH(prefix) DESC).
	 */
	snprintf(sql,sizeof(sql),
		"CREATE OR REPLACE TEMP TABLE rated_batch AS "
		"WITH matched AS ( "
		"  SELECT "
		"    c.id AS cdr_id, c.calling_number, c.called_number, c.billsec, c.start_ts, "
		"    ba.id AS bacc_id, ba.round_mode_id, "
		"    rt.id AS rate_id, rt.tariff_id, pr.prefix, pr.id AS prefix_id, "
		"    ROW_NUMBER() OVER (PARTITION BY c.id ORDER BY LENGTH(pr.prefix) DESC) AS rn "
		"  FROM batch_window c "
		"  JOIN pg.calling_number clg ON clg.calling_number = c.calling_number "
		"  JOIN pg.calling_number_deff cd ON cd.calling_number_id = clg.id "
		"  JOIN pg.billing_account ba ON ba.id = clg.billing_account_id "
		"    AND ba.cdr_server_id = c.cdr_server_id "
		"  JOIN pg.bill_plan bp ON bp.id = cd.bill_plan_id "
		"  LEFT JOIN pg.bill_plan_tree tree ON tree.root_bplan_id = bp.id "
		"  JOIN pg.rate rt ON rt.bill_plan_id = COALESCE(tree.bill_plan_id, bp.id) "
		"  JOIN pg.prefix pr ON pr.id = rt.prefix_id "
		"    AND c.called_number LIKE pr.prefix || '%%' "
		"  JOIN pg.tariff tr ON tr.id = rt.tariff_id "
		"), "
		"best_match AS ( "
		"  SELECT * FROM matched WHERE rn = 1 "
		"), "
		"with_tariff AS ( "
		"  SELECT bm.*, cf.delta_time, cf.fee, cf.iterations "
		"  FROM best_match bm "
		"  JOIN pg.calc_function cf ON cf.tariff_id = bm.tariff_id "
		"  WHERE cf.pos = 1 "
		") "
		"SELECT CAST(cdr_id AS BIGINT) AS cdr_id, calling_number, called_number, CAST(billsec AS BIGINT) AS billsec, "
		"  CAST(start_ts AS VARCHAR) AS start_ts_str, "
		"  CAST(bacc_id AS BIGINT) AS bacc_id, round_mode_id, CAST(rate_id AS BIGINT) AS rate_id, tariff_id, prefix_id, "
		"  delta_time, fee, iterations, "
		"  CASE "
		"    WHEN iterations = 0 THEN fee * CEIL(CAST(billsec AS DOUBLE) / delta_time) "
		"    ELSE fee * iterations "
		"  END AS cprice "
		"FROM with_tariff");

	if(rt_duckdb_exec(ctx,sql) < 0) {
		LOG("rt_duckdb_rate_batch()","build rated_batch failed");
		return -1;
	}

	rb = rt_duckdb_select(ctx,"SELECT * FROM rated_batch");
	row_count = (rb != NULL) ? rb->rows : 0;

	LOG("rt_duckdb_rate_batch()","matched: %d CDRs (window <= id %lld)",
		row_count,(long long)max_window_id);

	/*
	 * STEP 2: Bulk INSERT rated rows into PostgreSQL and set leg = rating_id.
	 * Skipped when nothing matched - the whole window then falls through to -1.
	 */
	if(row_count > 0) {
		char *insert_sql;
		char row_buf[512];
		char safe_ts[64];
		int first = 1;
		int i;

		insert_sql = (char *)mem_alloc(row_count * 300 + 1024);
		if(insert_sql == NULL) {
			db_sql_result_free(rb);
			ctx->duck->conn->result = NULL;
			return -1;
		}

		strcpy(insert_sql,
			"INSERT INTO rating"
			" (call_price,call_billsec,rate_id,billing_account_id,call_id,"
			"rating_mode_id,pcard_id,time_condition_id,call_ts,last_update,free_billsec_id)"
			" VALUES ");

		for(i = 0; i < row_count; i++) {
			long long cdr_id  = atoll(rb->cols_list[0].rows_list[i].row);  /* cdr_id */
			int billsec       = atoi(rb->cols_list[3].rows_list[i].row);   /* billsec */
			long long bacc_id = atoll(rb->cols_list[5].rows_list[i].row);  /* bacc_id */
			long long rate_id = atoll(rb->cols_list[7].rows_list[i].row);  /* rate_id */
			double cprice     = atof(rb->cols_list[13].rows_list[i].row);  /* cprice */
			char *ts          = rb->cols_list[4].rows_list[i].row;         /* start_ts_str */

			memset(safe_ts,0,sizeof(safe_ts));
			if(ts != NULL && ts[0] != '\0') {
				db_sql_escape(ts,safe_ts,sizeof(safe_ts));
			} else {
				strcpy(safe_ts,"1970-01-01 00:00:00");
			}

			snprintf(row_buf,sizeof(row_buf),
				"%s(%f,%d,%lld,%lld,%lld,1,0,0,'%s','now()',0)",
				(first ? "" : ","),
				cprice,billsec,rate_id,bacc_id,cdr_id,safe_ts);

			strcat(insert_sql,row_buf);
			first = 0;
		}

		strcat(insert_sql," RETURNING id, call_id");

		/* done reading the DuckDB result */
		db_sql_result_free(rb);
		ctx->duck->conn->result = NULL;
		rb = NULL;

		/* execute on PostgreSQL */
		db_query(pg_dbp,insert_sql,0);
		db_fetch(pg_dbp);

		/* build CDR update from RETURNING result */
		if(pg_dbp->conn->result != NULL) {
			db_sql_result_t *pg_result = (db_sql_result_t *)pg_dbp->conn->result;

			if(pg_result->rows > 0) {
				char *update_sql = (char *)mem_alloc(pg_result->rows * 40 + 256);
				if(update_sql != NULL) {
					int j;

					sprintf(update_sql,
						"UPDATE cdrs SET leg_%c = v.rid FROM (VALUES ",leg);

					for(j = 0; j < pg_result->rows; j++) {
						char upd_row[64];
						long long rating_id = atoll(pg_result->cols_list[0].rows_list[j].row);
						long long cdr_id    = atoll(pg_result->cols_list[1].rows_list[j].row);

						snprintf(upd_row,sizeof(upd_row),"%s(%lld,%lld)",
							(j > 0 ? "," : ""),cdr_id,rating_id);
						strcat(update_sql,upd_row);
					}

					strcat(update_sql,") AS v(cid,rid) WHERE cdrs.id = v.cid");

					db_query(pg_dbp,update_sql,1);
					rated = pg_result->rows;

					mem_free(update_sql);
				}
			}

			db_sql_result_free(pg_result);
			pg_dbp->conn->result = NULL;
		}

		mem_free(insert_sql);
	} else if(rb != NULL) {
		db_sql_result_free(rb);
		ctx->duck->conn->result = NULL;
	}

	/*
	 * STEP 3: Mark the evaluated-but-unrated CDRs in this window as leg = -1.
	 * Scoped to id <= max_window_id (the window we just rated against), so we
	 * never mark a CDR that was never evaluated. Matched CDRs are already
	 * leg > 0 and excluded by the leg = 0 predicate.
	 */
	{
		char mark_sql[256];
		snprintf(mark_sql,sizeof(mark_sql),
			"UPDATE cdrs SET leg_%c = -1 WHERE leg_%c = 0 AND id <= %lld",
			leg,leg,(long long)max_window_id);

		db_query(pg_dbp,mark_sql,1);

		LOG("rt_duckdb_rate_batch()","marked unmatched CDRs as -1 (id <= %lld)",
			(long long)max_window_id);
	}

	LOG("rt_duckdb_rate_batch()","batch complete: %d rated",rated);

	return rated;
}
