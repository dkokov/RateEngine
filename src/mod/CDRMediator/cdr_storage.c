#include <unistd.h>

#include "../../misc/globals.h"
#include "../../misc/exten/time_funcs.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "prefix_filter.h"
#include "cdr.h"
#include "cdr_storage.h"
#include "cdr_storage_sched.h"

cdr_storage_col_t *cdr_storage_col_init(int num)
{
	size_t mem;

	if(num <= 0) return NULL;

	mem = sizeof(cdr_storage_col_t)*(num + 1);

	return (cdr_storage_col_t *)mem_alloc(mem);
}

cdr_storage_profile_t *cdr_storage_profile_init(void)
{	
	return (cdr_storage_profile_t *)mem_alloc(sizeof(cdr_storage_profile_t));	
}

void cdr_storage_col_put(cdr_storage_col_t *cols,cdr_table_t *tbl,char *col_name,int col_id) 
{
	if((tbl != NULL)&&(cols != NULL)) {
		cols->func = tbl->func;
		strcpy(cols->col_name,col_name);
		cols->col_id = col_id;
	}
}

void cdr_storage_sql_query_parser(cdr_storage_profile_t *profile)
{
	int i;
	
	char *buf;
	
	char columns[2048];
	char sql_query[2048];
	char sql_where[2048];
	
	bzero(columns,sizeof(columns));
	bzero(sql_query,sizeof(sql_query));
	bzero(sql_where,sizeof(sql_where));

	for(i=0;i<profile->cols_num;i++) {
		if(i==0) sprintf(columns,"%s",profile->cols[i].col_name);
		else {
			if(strcmp(profile->cols[i].col_name,"")) {
				buf = strdup(columns);
				bzero(columns,2048);
				sprintf(columns,"%s,%s",buf,profile->cols[i].col_name);
				free(buf);
			}
		}
	}
	
	if(profile->sql_col_t == ts) {
		char ts[DT_LEN];
		convert_epoch_to_ts(profile->ts,ts);
		//sprintf(sql_where,"to_timestamp(%d)",profile->ts);
		sprintf(sql_where,"'%s'",ts);
	} else sprintf(sql_where,"%d",profile->ts);
	
	sprintf(sql_query,"select %s from %s where %s >= %s",
			columns,profile->cdr_table,profile->sql_col_where,sql_where);
	
	if(strcmp(profile->sql_where_const,"")) {
		buf = strdup(sql_query);
		bzero(sql_query,2048);
		sprintf(sql_query,"%s and %s",buf,profile->sql_where_const);
		free(buf);
	}
	
	profile->sql_query = strdup(sql_query);
}

/*
 * Get records from REMOTE DB server.
 * Parse columns from a profile schem/CDR fields/.
 * Insert CDR in the local DB.
 * 
 */
/* Map one remote result row -> cdr_t and insert it locally. Returns 1 if the
 * insert reported success (incl. an ON CONFLICT no-op), 0 otherwise. */
static int cdr_storage_insert_row(cdr_storage_profile_t *profile,db_sql_result_t *result,int i)
{
	int c;
	cdr_t the_cdr;
	cdr_storage_col_t *cols = profile->cols;

	memset(&the_cdr,0,sizeof(the_cdr));

	the_cdr.cdr_server_id   = profile->cdr_server_id;
	the_cdr.cdr_rec_type_id = profile->cdr_rec_type_id;

	for(c = 0;c < (profile->cols_num - 1);c++) {
		(*cols[c].func)(&the_cdr,result->cols_list[c].rows_list[i].row);
	}

	strcpy(the_cdr.profile_name,profile->profile_name);

	return (cdr_add_in_db(profile->dbp,&the_cdr,profile->filters) == 0) ? 1 : 0;
}

int cdr_storage_get_remote_cdrs(cdr_storage_profile_t *profile)
{
    int i,p,fetched,rows;
    char ts[21];

	db_sql_result_t *result;

	if(profile->rem_dbp == NULL) return -1;
	if(profile->rem_dbp->t != sql) return 0;

	p = 0;
	fetched = 0;

	bzero(ts,21);
	convert_epoch_to_ts(profile->ts,ts);

	/* Rows are inserted locally with autocommit (each visible+durable at once,
	 * so rating runs concurrently and a restart loses only the last row). Dedup
	 * is via cdrs.call_uid UNIQUE + ON CONFLICT. Only the REMOTE read is wrapped
	 * (read-only) below when a server-side cursor is used. */

	if(strcmp(profile->rem_dbp->conn->enginename,"pgsql") == 0) {
		/* Bounded-memory path: stream the remote result through a server-side
		 * cursor in chunks instead of materializing millions of rows at once. */
		int chunk = (profile->fetch_chunk > 0) ? profile->fetch_chunk : CDR_FETCH_CHUNK;
		char decl[SQL_BUF_LEN + 128];
		char fetchq[128];

		snprintf(decl,sizeof(decl),"DECLARE %s CURSOR FOR %s",CDR_CURSOR_NAME,profile->sql_query);
		snprintf(fetchq,sizeof(fetchq),"FETCH %d FROM %s",chunk,CDR_CURSOR_NAME);

		db_query(profile->rem_dbp,"BEGIN",1);            /* cursor needs a tx (read-only) */
		db_query(profile->rem_dbp,decl,1);

		for(;;) {
			db_query(profile->rem_dbp,fetchq,0);         /* keep result for db_fetch */
			db_fetch(profile->rem_dbp);

			if(profile->rem_dbp->conn->result == NULL) break;
			result = (db_sql_result_t *)profile->rem_dbp->conn->result;
			rows = result->rows;

			for(i = 0;i < rows;i++) {
				p += cdr_storage_insert_row(profile,result,i);

				if((p > 0) && (p % CDR_PROGRESS_STEP == 0))
					LOG("cdr_storage_get_romote_cdrs()",
						"progress: inserted %d,cdr_server_id: %d",p,profile->cdr_server_id);
			}

			fetched += rows;

			db_sql_result_free(result);
			profile->rem_dbp->conn->result = NULL;

			if(rows < chunk) break;                      /* last (partial) chunk */
		}

		db_query(profile->rem_dbp,"CLOSE " CDR_CURSOR_NAME,1);
		db_query(profile->rem_dbp,"COMMIT",1);
	} else {
		/* Non-pgsql engines: single fetch (cursor/FETCH syntax is pg-specific). */
		db_select(profile->rem_dbp,profile->sql_query);
		db_fetch(profile->rem_dbp);

		if(profile->rem_dbp->conn->result != NULL) {
			result = (db_sql_result_t *)profile->rem_dbp->conn->result;
			rows = result->rows;

			for(i = 0;i < rows;i++) {
				p += cdr_storage_insert_row(profile,result,i);

				if((p > 0) && (p % CDR_PROGRESS_STEP == 0))
					LOG("cdr_storage_get_romote_cdrs()",
						"progress: inserted %d,cdr_server_id: %d",p,profile->cdr_server_id);
			}

			fetched = rows;

			db_sql_result_free(result);
			profile->rem_dbp->conn->result = NULL;
		}
	}

	LOG("cdr_storage_get_romote_cdrs()",
		"get cdrs num: %d,cdr_server_id: %d,inserted cdrs: %d(%.2f%),ts: %s (%d)",
		fetched,profile->cdr_server_id,p,(fetched > 0 ? (((float)p/(float)fetched)*100) : 0),ts,profile->ts);

	return p;
}

void cdr_storage_reader(cdr_storage_profile_t *profile)
{
	int num,ret;

	num = cdr_storage_sched_set_ts(profile);
		
	cdr_storage_sql_query_parser(profile);

	ret = db_connect(profile->rem_dbp);
	if(ret < 0) {
		LOG("cdr_storage_reader()","Cannot connect with '%s' host /engine: %s/!",profile->rem_dbp->conn->hostname,profile->rem_dbp->conn->enginename);
		goto _end;
	}
	
	ret = cdr_storage_get_remote_cdrs(profile);
	if(ret == -1) {
		LOG("cdr_storage_reader()","Cannot get CDRs from '%s' host !",profile->rem_dbp->conn->hostname);
		num = -1;
	}
	
	db_close(profile->rem_dbp);
	
	if(num > 0) cdr_storage_sched_update(profile);
	else if(num == 0) cdr_storage_sched_insert(profile);
	else {
		LOG("cdr_storage_reader()","Don't change sched_ts for '%s' host !",profile->rem_dbp->conn->hostname);
	}
	
_end:
	if(profile->sql_query != NULL) mem_free(profile->sql_query);
}
