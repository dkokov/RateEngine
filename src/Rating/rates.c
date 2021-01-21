#include <pthread.h>

#include "../misc/globals.h"
#include "../misc/mem/mem.h"

#include "rates.h"

rates_cache_tbl cache_bplan_rates_tbl[RATES_BPLAN_TBL_NUM];

void rates_cache_tbl_free(void)
{
	int i,bplan;
	
	pthread_mutex_lock(&cache_tbl_lock);
	
	for(i=0;i<RATES_BPLAN_TBL_NUM;i++) {
		bplan = cache_bplan_rates_tbl[i].bplan;
		
		if(bplan > 0) {
			mem_free(cache_bplan_rates_tbl[i].rt);
		
			cache_bplan_rates_tbl[i].rt = NULL;
			cache_bplan_rates_tbl[i].bplan = 0;
		
			LOG("rates_cache_tbl_free()","free bplan: %d from cache tbl",bplan);
		}
	}
	
	pthread_mutex_unlock(&cache_tbl_lock);
}

int rates_put_in_cache_tbl(int bplan,rates *rt)
{
	int i;
	
//	pthread_mutex_lock(&cache_tbl_lock);

	for(i=0;i<RATES_BPLAN_TBL_NUM;i++) {
		if(cache_bplan_rates_tbl[i].bplan == 0) {
			cache_bplan_rates_tbl[i].bplan = bplan;
			cache_bplan_rates_tbl[i].rt = rt;
			
			LOG("rates_put_in_cache_tbl()","insert bplan: %d in cache tbl",bplan);
			
			return 0;
		}
	}
	
//	pthread_mutex_unlock(&cache_tbl_lock);

	return 1;
}

rates *rates_get_from_cache_tbl(int bplan)
{
	int i;
	
	for(i=0;i<RATES_BPLAN_TBL_NUM;i++) {
		if(cache_bplan_rates_tbl[i].bplan == bplan) {
			LOG("rates_put_in_cache_tbl()","get bplan: %d from cache tbl",bplan);

			return cache_bplan_rates_tbl[i].rt;						
		}
	}
	
	return 0;
}

int rates_get_rate_db_screenshot(PGconn *conn)
{
	int num,rates_ts,fnum;
	PGresult *res;
	char str[128];
	
	num = 0;
	rates_ts = 0;
	
	sprintf(str,"select last_update_ts from db_screenshot where tbl_name = 'rate'");
	
	res = db_pgsql_exec(conn,str);
	if(res != NULL) {
		num = PQntuples(res);
	
		if(num == 1) {
			fnum = PQfnumber(res,"last_update_ts");
			rates_ts = atoi(PQgetvalue(res,0,fnum));
		}
	
		PQclear(res);
	}
	
	return rates_ts;
}

int rates_cache_tbl_refresh(PGconn *conn,int old_rates_ts)
{
	int rates_ts;
	
	rates_ts = 0;
	
	rates_ts = rates_get_rate_db_screenshot(conn);
	
	if(rates_ts > old_rates_ts) {
		LOG("rates_cache_tbl_refresh()","RATES CACHE - START BPLANs REFRESH(cache tbl clear)");
		rates_cache_tbl_free();
	}
	
	return rates_ts;
}

rates *rates_get_bplan_rates_pgsql(PGconn *conn,rating *pre)
{
    PGresult *res;
    rates *rt;

    char str[512];
    int i,num;
	int *fnum;
    int bplan_num;

    rt = 0 ;
    num = 0;

    bplan_num = f_bplan_tree_num(conn,pre);

    if(bplan_num == 0) {
      sprintf(str,"select rt.id,rt.tariff_id,pr.prefix,tr.start_period,tr.end_period,rt.prefix_id,tr.free_billsec_id "
                     " from rate as rt,prefix as pr,tariff as tr "
                     " where rt.bill_plan_id = %d and pr.id = rt.prefix_id and "
                     " rt.tariff_id = tr.id "
                     " order by pr.prefix desc",
                     pre->bplan);
    } else {
      sprintf(str,"select rt.id,rt.tariff_id,pr.prefix,tr.start_period,tr.end_period,rt.prefix_id,tr.free_billsec_id "
                     " from rate as rt,prefix as pr,tariff as tr,bill_plan_tree as tree "
                     " where tree.root_bplan_id = %d and "
                     " rt.bill_plan_id = tree.bill_plan_id and "
                     " pr.id = rt.prefix_id and "
                     " rt.tariff_id = tr.id "
                     " order by pr.prefix desc",
                     pre->bplan);
    }

	res = db_pgsql_exec(conn,str);
    
    if(res != NULL) {
		num = PQntuples(res);
    		    
		if(num) {
			char *col[] = {"id","tariff_id","prefix","start_period","end_period","prefix_id","free_billsec_id",""};
		
			fnum = db_pgsql_fnum(res,col);
			if(fnum == 0) goto end;
        
			rt = (rates *)mem_alloc_arr((num+1),sizeof(rates));

			for (i = 0; i < num; i++) {
				rt[i].rate = atoi(PQgetvalue(res,i,fnum[0]));
				rt[i].tariff = atoi(PQgetvalue(res,i,fnum[1]));
				strcpy(rt[i].prefix,PQgetvalue(res,i,fnum[2]));
				rt[i].start_period = atoi(PQgetvalue(res,i,fnum[3]));
				rt[i].end_period = atoi(PQgetvalue(res,i,fnum[4]));
				rt[i].prefix_id = atoi(PQgetvalue(res,i,fnum[5]));
				rt[i].free_billsec_id = atoi(PQgetvalue(res,i,fnum[6]));
			}
			
			db_pgsql_fnum_free(fnum);
		}
		
		PQclear(res);
	}
    
    end:
    return rt;	
}

rates *rates_get_bplan_rates(PGconn *conn,rating *pre)
{
    rates *rt;

	rt = rates_get_from_cache_tbl(pre->bplan);

	if(rt == 0) {
		rt = rates_get_bplan_rates_pgsql(conn,pre);
		rates_put_in_cache_tbl(pre->bplan,rt);		
	}
	
    return rt;
}
