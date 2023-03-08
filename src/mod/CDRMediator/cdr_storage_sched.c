#include <time.h>

#include "../../misc/globals.h"
#include "../../misc/exten/time_funcs.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "prefix_filter.h"
#include "cdr.h"
#include "cdr_storage.h"
#include "cdr_storage_sched.h"
#include "cdrm_json.h"

cdr_storage_sched_t *cdr_storage_sched_init(void)
{	
	return (cdr_storage_sched_t *)mem_alloc(sizeof(cdr_storage_sched_t));
}

int cdr_storage_sched_get(cdr_storage_profile_t *cfg,cdr_storage_sched_t *sched)
{
    int i,num,ret;
    char str[SQL_BUF_LEN];
  
	db_sql_result_t *result;
	db_nosql_result_t *result2;
	cdrm_json_t cdr_json_ptr;
	
	if(cfg->dbp == NULL) return DB_ERR_DBP_NUL;
	if(sched == NULL) return -1;
	
	num = 0;
	
	if(cfg->dbp->t == sql) {
		memset(str,0,SQL_BUF_LEN);
		
		sprintf(str,"select ts,last_chk_ts,start_ts,replies from cdr_storage_sched where cdr_server_id = %d",cfg->cdr_server_id);
		
		db_select(cfg->dbp,str);
		db_fetch(cfg->dbp);
			
		if(cfg->dbp->conn->result != NULL) {
			result = (db_sql_result_t *)cfg->dbp->conn->result;
				
			if(result->rows > 0) {
				for(i = 0; i < result->rows; i++) {
					sched->ts = atoi(result->cols_list[0].rows_list[i].row);
					sched->last = atoi(result->cols_list[1].rows_list[i].row);
					sched->start = atoi(result->cols_list[2].rows_list[i].row);
					sched->replies = atoi(result->cols_list[3].rows_list[i].row);
				}
			}
				
			num = result->rows;
				
			db_sql_result_free(result);
			cfg->dbp->conn->result = NULL;
		}
	} else if(cfg->dbp->t == nosql) {
		memset(&cdr_json_ptr,0,sizeof(cdrm_json_t));
		sprintf(cdr_json_ptr.header,"%s%s",CDR_JSON_CDR_SCHED_HDR_PATTERN,cfg->profile_name);
		
		memset(str,0,SQL_BUF_LEN);
		sprintf(str,"get %s",cdr_json_ptr.header);
		
		ret = db_get(cfg->dbp,str);
		if(ret < 0) {
			db_error(ret);
			//return DB_ERR_NOSQL_RES_NUL;
			return 0;
		}
		
		result2 = (db_nosql_result_t *)cfg->dbp->conn->result;
		
		cdr_json_ptr.msg = result2->str;
		
		cdrm_json_sched_struct(&cdr_json_ptr,sched);

		db_nosql_result_free(result2);
		cfg->dbp->conn->result = NULL;
		
		db_free_result(cfg->dbp);		
	} else return DB_ERR_TYPE_UNK;
	
	return num;	
}

int cdr_storage_sched_nosql_insert(cdr_storage_profile_t *cfg)
{
	int ret;
	char str[CDR_JSON_BUF_LEN];
	
    cdrm_json_t cdr_j;

	memset(&cdr_j,0,sizeof(cdrm_json_t));
	cdrm_sched_struct_json(cfg,&cdr_j);
		
	memset(str,0,SQL_BUF_LEN);
	sprintf(str,"set %s %s",cdr_j.header,cdr_j.msg);
	mem_free(cdr_j.msg);
		
	ret = db_set(cfg->dbp,str,NULL);
	if(ret < 0) {
		db_error(ret);
		return -1;
	}
	
	return ret;
}

int cdr_storage_sched_insert(cdr_storage_profile_t *cfg)
{
	int ret;
    char str[SQL_BUF_LEN];
    
	if(cfg->dbp == NULL) return DB_ERR_DBP_NUL;

	if(cfg->dbp->t == sql) {
		memset(str,0,SQL_BUF_LEN);
		sprintf(str,"insert into cdr_storage_sched (cdr_server_id,start_ts,last_chk_ts,ts) values (%d,%d,%d,%d)",
					 cfg->cdr_server_id,(int)convert_ts_to_epoch(cfg->cdr_sched_ts),cfg->chk_ts,cfg->ts);

		ret = db_insert(cfg->dbp,str);
		if(ret < 0) {
			db_error(ret);
			return DB_ERR_SQLEXEC_NOOK;
		}
	} else if(cfg->dbp->t == nosql) {
		ret = cdr_storage_sched_nosql_insert(cfg);
	} else return DB_ERR_TYPE_UNK;
	
	return ret;
}

int cdr_storage_sched_update(cdr_storage_profile_t *cfg)
{
    int ret;
    char str[512];

	if(cfg->dbp == NULL) return DB_ERR_DBP_NUL;
	
	if(cfg->dbp->t == sql) {
		sprintf(str,"update cdr_storage_sched "
					" set last_chk_ts = %d,ts=%d where cdr_server_id = %d",cfg->chk_ts,cfg->ts,cfg->cdr_server_id);

		ret = db_update(cfg->dbp,str);
	} else if(cfg->dbp->t == nosql) {
		ret = cdr_storage_sched_nosql_insert(cfg);
	} 
	
	return ret;
}

int cdr_storage_sched_set_ts(cdr_storage_profile_t *cfg)
{
	int num;
	cdr_storage_sched_t *sched;
	
    time_t chk,temp;
    int curr_cycle,last_cycle;
            
    sched = cdr_storage_sched_init();
    if(sched == NULL) goto end_func;
    
    num = cdr_storage_sched_get(cfg,sched);
    
    if(sched->start == 0) sched->start = convert_ts_to_epoch(cfg->cdr_sched_ts);
    if(sched->last == 0) sched->last = sched->start;
    
    chk = time(NULL);
    
    if((sched->start) == (sched->last)) {
		cfg->ts = sched->start;
		goto end_func;
	}
    
    temp = (chk - (sched->start));
    curr_cycle = (temp / CDR_STORAGE_SCHED_D);

    if(curr_cycle > 1) {
		sched->curr_ts = ((curr_cycle*CDR_STORAGE_SCHED_D)+(sched->start));
		
		if((sched->curr_ts > sched->last)) {
			temp = (sched->last - sched->start);
		
			last_cycle = (temp / CDR_STORAGE_SCHED_D); 
		
			if( last_cycle > 1 ) {
				sched->last_ts = ((last_cycle*CDR_STORAGE_SCHED_D)+(sched->start));

				cfg->ts = sched->last_ts;
			} else cfg->ts = sched->start;
		} else {
			if(sched->curr_ts <= sched->ts) cfg->ts = (sched->last - (call_maxsec_limit + CDR_STORAGE_SCHED_M));
			else cfg->ts = sched->curr_ts;
		}
		
    } else {
		cfg->ts = sched->start;
    }
    
    end_func:
	if(sched != NULL) mem_free(sched);
	
	cfg->chk_ts = chk;
	
	return num;
}
