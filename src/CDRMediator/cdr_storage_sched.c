#include "../misc/globals.h"
#include "../misc/mem/mem.h"
#include <time.h>
#include "../misc/exten/time_funcs.h"

#include "cdr_storage.h"
#include "cdr_storage_sched.h"

cdr_storage_sched_t *cdr_storage_sched_init(void)
{
	cdr_storage_sched_t *sched;
	
	sched = NULL;
	
	sched = (cdr_storage_sched_t *)mem_alloc(sizeof(cdr_storage_sched_t));
	
	return sched;
}

int cdr_storage_sched_get(cdr_storage_profile_t *cfg,cdr_storage_sched_t *sched)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[5];
        
    num = 0;
    
    sprintf(str,
			"select ts,last_chk_ts,start_ts,replies from cdr_storage_sched where cdr_server_id = %d",
			cfg->cdr_server_id);
	res = db_pgsql_exec(cfg->conn,str);
	
	if(res) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"ts");
			fnum[1] = PQfnumber(res,"last_chk_ts");
			fnum[2] = PQfnumber(res,"start_ts");
			fnum[3] = PQfnumber(res,"replies");
	
			for(i = 0; i < num; i++) {
				sched->ts = atoi(PQgetvalue(res,i,fnum[0]));
				sched->last = atoi(PQgetvalue(res,i,fnum[1]));
				sched->start = atoi(PQgetvalue(res,i,fnum[2]));
				sched->replies = atoi(PQgetvalue(res,i,fnum[3]));
			}
		}
		
		PQclear(res);
	}
	
	return num;	
}

void cdr_storage_sched_insert(cdr_storage_profile_t *cfg)
{
    PGresult *res;
    char str[512];

    sprintf(str,"insert into cdr_storage_sched "
                "(cdr_server_id,start_ts,last_chk_ts,ts)"
                "values (%d,%d,%d,%d)",
                cfg->cdr_server_id,(int)convert_ts_to_epoch(cfg->cdr_sched_ts),cfg->chk_ts,cfg->ts);

	res = db_pgsql_exec(cfg->conn,str);
    if(res != NULL) PQclear(res);
}

void cdr_storage_sched_update(cdr_storage_profile_t *cfg)
{
    PGresult *res;
    char str[512];

    sprintf(str,
    "update cdr_storage_sched "
    " set last_chk_ts = %d,"
    "ts=%d "
    " where cdr_server_id = %d",
    cfg->chk_ts,cfg->ts,cfg->cdr_server_id);

	res = db_pgsql_exec(cfg->conn,str);
    if(res != NULL) PQclear(res);
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
