#include "../misc/globals.h"
#include "../misc/exten/time_funcs.h"
#include "../misc/mem/mem.h"

#include "cdr_storage.h"
#include "cdr_storage_sched.h"

PGconn *cdr_storage_pgsql_conn(cdr_storage_profile_t *profile)
{
    PGconn *conn;
    char   conninfo[255];

	bzero(conninfo,sizeof(conninfo));
	
    sprintf(conninfo,"host=%s dbname=%s user=%s password=%s ",
            profile->dbhost,
            profile->dbname,
            profile->dbuser,
            profile->dbpass);
            
	conn = db_pgsql_conn(conninfo);

    return conn;
}

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
	cdr_storage_profile_t *profile = NULL;
	
	profile = (cdr_storage_profile_t *)mem_alloc(sizeof(cdr_storage_profile_t));
	
	return profile;
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
	
	if(profile->sql_col_t == ts) sprintf(sql_where,"to_timestamp(%d)",profile->ts);
	else sprintf(sql_where,"%d",profile->ts);
	
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

PGresult *cdr_storage_pgsql_cdrs_exec(cdr_storage_profile_t *profile)
{
	PGresult *res;
		
	res = db_pgsql_exec(profile->pgsql,profile->sql_query);
	
	return res;
}

int cdr_storage_pgsql_cols_num(PGresult *res)
{
	return PQntuples(res);
}

int *cdr_storage_pgsql_cols_mem(int num)
{
	int *ptr;
	
	ptr = (int *)mem_alloc(sizeof(int)*num);
	if(ptr != NULL) {
		memset(ptr,0,(sizeof(int)*num));
	}
	
	return ptr;
}

int *cdr_storage_pgsql_cols_parser(PGresult *res,cdr_storage_profile_t *profile)
{
	int i;
	int *fnum;
	
	fnum = cdr_storage_pgsql_cols_mem(profile->cols_num);
	
	if(fnum != NULL) {
		for(i=0;i<profile->cols_num;i++) {
			fnum[i] = PQfnumber(res,profile->cols[i].col_name);
		}
	}
	
	return fnum;
}

/*
 * Get records from PGSQL remote server.
 * Parse columns from a profile schem/CDR fields/.
 * Insert CDR in the local DB.
 * 
 */
int cdr_storage_get_pgsql_cdrs(cdr_storage_profile_t *profile)
{ 
    int i,c,p;
    int rows;
    int *fnum;
    
    char ts[21];
    
    cdr_t the_cdr;
    PGresult *res;
    
    cdr_storage_col_t *cols;

	cols = profile->cols;
	
	bzero(ts,21);
	convert_epoch_to_ts(profile->ts,ts);
	
	p = 0;
	
	res = cdr_storage_pgsql_cdrs_exec(profile);

	if(res != NULL) {
		fnum = cdr_storage_pgsql_cols_parser(res,profile);

		if(fnum != NULL) {
			rows = cdr_storage_pgsql_cols_num(res);

			if(rows) {
				for (i = 0; i < rows ;i++) {
					memset(&the_cdr,0,sizeof(the_cdr));
					
					the_cdr.cdr_server_id = profile->cdr_server_id;
					the_cdr.cdr_rec_type_id = profile->cdr_rec_type_id;
						
					for(c = 0;c < (profile->cols_num - 1);c++) {
						(*cols[c].func)(&the_cdr,PQgetvalue(res,i,fnum[c]));
					}
			
					if(cdr_add_in_db(profile->conn,&the_cdr,profile->filters)) p++;
				}
			}

			LOG("cdr_storage_get_pgsql_cdrs()",
				"get cdrs num: %d,cdr_server_id: %d,inserted cdrs: %d(%.2f%),ts: %s (%d)",
				rows,profile->cdr_server_id,p,(((float)p/(float)rows)*100),ts,profile->ts);
				
			mem_free(fnum);    
		} else {
			LOG("cdr_storage_get_pgsql_cdrs()","fnum is null,cdr_server_id: %d",profile->cdr_server_id);
							  
			p = -1;
		}
		
		PQclear(res);
	} else {
		LOG("cdr_storage_get_pgsql_cdrs()","res is null,cdr_server_id: %d",profile->cdr_server_id);
						  
		p = -1;
	}
	
	return p;
}

int cdr_storage_read_pgsql(cdr_storage_profile_t *profile)
{		
	int num;
	
	num = -1;
	
	profile->pgsql = cdr_storage_pgsql_conn(profile);
		
	if((profile->pgsql) != NULL) num = cdr_storage_get_pgsql_cdrs(profile);
	else {
		LOG("cdr_storage_reader_pgsql()","Can't connect to '%s' host ",profile->dbhost);
	}
				
	PQfinish(profile->pgsql);
	
	return num;
}

void cdr_storage_read_mysql(cdr_storage_profile_t *profile)
{
	
}

void cdr_storage_read_oracle(cdr_storage_profile_t *profile)
{
	
}

void cdr_storage_reader(cdr_storage_profile_t *profile)
{
	int num;

	num = cdr_storage_sched_set_ts(profile);
		
	cdr_storage_sql_query_parser(profile);

	switch(profile->t) {
		case pgsql:
					if(cdr_storage_read_pgsql(profile) == -1) num = -1;
					break;
		case mysql:
					cdr_storage_read_mysql(profile);
					break;
		case oracle:
					cdr_storage_read_oracle(profile);
					break;	
	};
	
	if(num > 0) cdr_storage_sched_update(profile);
	else if(num == 0) cdr_storage_sched_insert(profile);
	else {
		LOG("cdr_storage_reader_pgsql()","Don't change sched_ts for '%s' host !",profile->dbhost);
	}
	
	if(profile->sql_query != NULL) mem_free(profile->sql_query);
}
