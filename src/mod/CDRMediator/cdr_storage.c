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
	cdr_storage_col_t *cols = NULL;
	
	if(num > 0) mem = sizeof(cdr_storage_col_t)*(num + 1);
	
	cols = (cdr_storage_col_t *)mem_alloc(mem);
	
	return cols;
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
int cdr_storage_get_remote_cdrs(cdr_storage_profile_t *profile)
{ 
    int i,c,p;    
    char ts[21];
    
    cdr_t the_cdr;    
    cdr_storage_col_t *cols;
	db_sql_result_t *result;

	if(profile->rem_dbp == NULL) return -1;

	if(profile->rem_dbp->t == sql) {
		p = 0;
		cols = profile->cols;
	
		bzero(ts,21);
		convert_epoch_to_ts(profile->ts,ts);
	
		db_select(profile->rem_dbp,profile->sql_query);
		db_fetch(profile->rem_dbp);
	
		if(profile->rem_dbp->conn->result != NULL) {
			result = (db_sql_result_t *)profile->rem_dbp->conn->result;
			
			if(result->rows > 0) {
				for (i = 0; i < result->rows ;i++) {
					memset(&the_cdr,0,sizeof(the_cdr));
					
					the_cdr.cdr_server_id = profile->cdr_server_id;
					the_cdr.cdr_rec_type_id = profile->cdr_rec_type_id;
						
					for(c = 0;c < (profile->cols_num - 1);c++) {
						(*cols[c].func)(&the_cdr,result->cols_list[c].rows_list[i].row);
					}
			
					strcpy(the_cdr.profile_name,profile->profile_name);
					if(cdr_add_in_db(profile->dbp,&the_cdr,profile->filters) == 0) p++;
				}
			}
			
			LOG("cdr_storage_get_romote_cdrs()",
				"get cdrs num: %d,cdr_server_id: %d,inserted cdrs: %d(%.2f%),ts: %s (%d)",
				result->rows,profile->cdr_server_id,p,(((float)p/(float)result->rows)*100),ts,profile->ts);
		
			db_sql_result_free(result);
			profile->rem_dbp->conn->result = NULL;
		}
	}
	
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
