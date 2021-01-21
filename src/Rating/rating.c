#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "../misc/globals.h"
#include "../misc/mem/mem.h"
#include "../misc/exten/time_funcs.h"

#include "../misc/mem/shm_mem.h"
#include "../misc/re5_manager.h"

#include "rt_cfg.h"
#include "time_cond.h"
#include "free_billsec.h"
#include "pcard.h"
#include "Rating5.h"
#include "calc_functions.h"
#include "round_billsec.h"
#include "rates.h"
#include "rt_stat.h"

void rating_init(rating *pre)
{
	memset(pre,0,sizeof(rating));
}

int rt_get_bacc_id(PGconn *conn,char *table,int cdr_server_id,char *cond)
{
	int num,bacc_id,fnum;
	PGresult *res;
	char str[512];
	
	num = 0;
	bacc_id = 0;
	
	sprintf(str,
			"select bacc.id from %s as clg,billing_account as bacc "
			"where bacc.id = clg.billing_account_id and "
			"bacc.cdr_server_id = %d and clg.%s = '%s'",
			table,cdr_server_id,table,cond);
	
	res = db_pgsql_exec(conn,str);
	if(res != NULL) {
		num = PQntuples(res);
	
		if(num == 1) {
			fnum = PQfnumber(res,"id");
			bacc_id = atoi(PQgetvalue(res,0,fnum));
		}
	
		PQclear(res);
	}
	
	return bacc_id;
}

double rt_get_balances(PGconn *conn,int bacc_id)
{
    PGresult *res;

    char str[512];
    int num,fnum;
    double balance;
    
    num = 0;
    balance = 0;
    
    sprintf(str,"select sum(amount) as bal from balance where billing_account_id = %d and active = 't'",bacc_id);

	res = db_pgsql_exec(conn,str);
    
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum = PQfnumber(res,"bal");
			balance = atof(PQgetvalue(res,0,fnum));
		}
		
		PQclear(res);
	}
	
	return balance;
}

void f_bacc_sql_query(PGconn *conn,rating *pre,char *table,char *cond)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[7];
    
    num = 0;
    
    sprintf(str,"SELECT clg.billing_account_id,clg_df.bill_plan_id,bp.start_period,bp.end_period,"
                "bacc.billing_day,bacc.round_mode_id,bacc.day_of_payment "
                " from %s as clg,%s_deff as clg_df,bill_plan as bp,billing_account as bacc "
                " where clg.id = clg_df.%s_id and "
                " bp.id = clg_df.bill_plan_id and "
                " clg.%s = '%s' and "
                " bacc.id = clg.billing_account_id and"
                " bacc.cdr_server_id = %d ",
                table,table,table,table,cond,pre->cdr_server_id);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"billing_account_id");
			fnum[1] = PQfnumber(res,"bill_plan_id");
			fnum[2] = PQfnumber(res,"start_period");
			fnum[3] = PQfnumber(res,"end_period");
			fnum[4] = PQfnumber(res,"billing_day");
			fnum[5] = PQfnumber(res,"round_mode_id");
			fnum[6] = PQfnumber(res,"day_of_payment");
	
			for(i = 0; i < num; i++) {
				pre->bacc = atoi(PQgetvalue(res,i,fnum[0]));
				pre->bplan = atoi(PQgetvalue(res,i,fnum[1]));
				pre->bplan_start_period = atoi(PQgetvalue(res,i,fnum[2]));
				pre->bplan_end_period = atoi(PQgetvalue(res,i,fnum[3]));
				strcpy(pre->billing_day,PQgetvalue(res,i,fnum[4]));
				pre->round_mode_id = atoi(PQgetvalue(res,i,fnum[5]));
				pre->day_of_payment = atoi(PQgetvalue(res,i,fnum[6]));
			}
		}
		
		PQclear(res);
	}
}

void f_bacc_sql_query_2(PGconn *conn,rating *pre,char *table,char *cond,int clg_nadi,int cld_nadi)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[8];
    
    num = 0;
    
    sprintf(str,"SELECT src.billing_account_id,src_df.bill_plan_id,src_df.clg_nadi,src_df.cld_nadi,bp.start_period,bp.end_period,bacc.billing_day,bacc.round_mode_id"
                " from %s as src,%s_deff as src_df,bill_plan as bp,billing_account as bacc  "
                " where src.id = src_df.%s_id and "
                " bp.id = src_df.bill_plan_id and "
                " src.%s = '%s' and "
                " bacc.id = src.billing_account_id and "
                " bacc.cdr_server_id = %d and src_df.clg_nadi = %d and src_df.cld_nadi = %d",
                table,table,table,table,cond,pre->cdr_server_id,clg_nadi,cld_nadi);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"billing_account_id");
			fnum[1] = PQfnumber(res,"bill_plan_id");
			fnum[2] = PQfnumber(res,"clg_nadi");
			fnum[3] = PQfnumber(res,"cld_nadi");
			fnum[4] = PQfnumber(res,"start_period");
			fnum[5] = PQfnumber(res,"end_period");
			fnum[6] = PQfnumber(res,"billing_day");
			fnum[7] = PQfnumber(res,"round_mode_id");

			for(i = 0; i < num; i++) {
				pre->bacc = atoi(PQgetvalue(res,i,fnum[0]));
				pre->bplan = atoi(PQgetvalue(res,i,fnum[1]));
				pre->clg_nadi = atoi(PQgetvalue(res,i,fnum[2]));
				pre->cld_nadi = atoi(PQgetvalue(res,i,fnum[3]));
				pre->bplan_start_period = atoi(PQgetvalue(res,i,fnum[4]));
				pre->bplan_end_period = atoi(PQgetvalue(res,i,fnum[5]));
				strcpy(pre->billing_day,PQgetvalue(res,i,fnum[6]));
				pre->round_mode_id = atoi(PQgetvalue(res,i,fnum[7]));
			}
		}
		
		PQclear(res);
	}
}

void f_bacc_sms_sql_query(PGconn *conn,rating *pre,char *table,char *cond)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[7];
    
    num = 0;
    
    sprintf(str,"SELECT clg.billing_account_id,clg_df.sm_bill_plan_id,bp.start_period,bp.end_period,"
                "bacc.billing_day,bacc.round_mode_id,bacc.day_of_payment "
                " from %s as clg,%s_deff as clg_df,bill_plan as bp,billing_account as bacc "
                " where clg.id = clg_df.%s_id and "
                " bp.id = clg_df.sm_bill_plan_id and "
                " clg.%s = '%s' and "
                " bacc.id = clg.billing_account_id and"
                " bacc.cdr_server_id = %d ",
                table,table,table,table,cond,pre->cdr_server_id);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"billing_account_id");
			fnum[1] = PQfnumber(res,"sm_bill_plan_id");
			fnum[2] = PQfnumber(res,"start_period");
			fnum[3] = PQfnumber(res,"end_period");
			fnum[4] = PQfnumber(res,"billing_day");
			fnum[5] = PQfnumber(res,"round_mode_id");
			fnum[6] = PQfnumber(res,"day_of_payment");
	
			for(i = 0; i < num; i++) {
				pre->bacc = atoi(PQgetvalue(res,i,fnum[0]));
				pre->bplan = atoi(PQgetvalue(res,i,fnum[1]));
				pre->bplan_start_period = atoi(PQgetvalue(res,i,fnum[2]));
				pre->bplan_end_period = atoi(PQgetvalue(res,i,fnum[3]));
				strcpy(pre->billing_day,PQgetvalue(res,i,fnum[4]));
				pre->round_mode_id = atoi(PQgetvalue(res,i,fnum[5]));
				pre->day_of_payment = atoi(PQgetvalue(res,i,fnum[6]));
			}
		}
		
		PQclear(res);
	}
}

void f_calling_number_sms_query(PGconn *conn,rating *pre)
{
    f_bacc_sms_sql_query(conn,pre,"calling_number",pre->clg);
}

void f_calling_number_query(PGconn *conn,rating *pre)
{
    f_bacc_sql_query(conn,pre,"calling_number",pre->clg);
}

void f_account_code_query(PGconn *conn,rating *pre,char *acc)
{
    f_bacc_sql_query(conn,pre,"account_code",acc);
}

void f_src_context_query(PGconn *conn,rating *pre,char *src_context)
{
    f_bacc_sql_query(conn,pre,"src_context",src_context);
}

void f_src_tgroup_query(PGconn *conn,rating *pre,char *src_tgroup,int clg_nadi,int cld_nadi)
{
    f_bacc_sql_query_2(conn,pre,"src_tgroup",src_tgroup,clg_nadi,cld_nadi);
}

void f_dst_context_query(PGconn *conn,rating *pre,char *dst_context)
{
    f_bacc_sql_query(conn,pre,"dst_context",dst_context);
}

void f_dst_tgroup_query(PGconn *conn,rating *pre,char *dst_tgroup,int clg_nadi,int cld_nadi)
{
    f_bacc_sql_query_2(conn,pre,"dst_tgroup",dst_tgroup,clg_nadi,cld_nadi);
}

void f_bacc_query(PGconn *conn,rating *pre)
{
    f_bacc_sql_query(conn,pre,"calling_number",pre->clg);
}

int chk_bplan_periods(rating *pre)
{
    if(pre->bplan_start_period)
    {
        if(pre->bplan_start_period <= pre->ts)
        {
            if(pre->bplan_end_period)
            {
                if(pre->bplan_end_period >= pre->ts) return 1;
                else pre->bplan = 0;
            }
        }
        else pre->bplan = 0;
    }
    return 0;
}

void f_bal_query(PGconn *conn,rating *pre,char *start,char *end)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[2];
    
    num = 0;
    
    sprintf(str,"select amount,id from balance "
                "where billing_account_id = %d and "
                "start_date = '%s' and end_date='%s' and active = 't'",
                pre->bacc,start,end);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num)
		{
			fnum[0] = PQfnumber(res,"amount");
			fnum[1] = PQfnumber(res,"id");

			for (i = 0; i < num; i++) {
				pre->balanse = atoi(PQgetvalue(res,i,fnum[0])); // atoi() ? ne trqbwa li da e atof() ????
				pre->bal_id = atoi(PQgetvalue(res,i,fnum[1]));
			}
		}
		
		PQclear(res);
	}
}

int f_bal_query_2(PGconn *conn,rating *pre,char *start,char *end,char flag)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[2];
    
    num = 0;
    
    sprintf(str,"select amount,id,last_update from balance "
                "where billing_account_id = %d and "
                "start_date = '%s' and end_date='%s' and active = '%c'"
                " order by last_update",
                pre->bacc,start,end,flag);

	res = db_pgsql_exec(conn,str);
    
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"amount");
			fnum[1] = PQfnumber(res,"id");
			fnum[2] = PQfnumber(res,"last_update");

			for (i = 0; i < num; i++) {
				pre->balanse = atoi(PQgetvalue(res,i,fnum[0]));
				pre->bal_id = atoi(PQgetvalue(res,i,fnum[1]));
				strcpy(pre->bal_lupdate,PQgetvalue(res,i,fnum[2]));
			}
		}
		
		PQclear(res);
	}
	
	return num;
}

void f_bal_query_v3(PGconn *conn,rating *pre,char *start,char *end)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[1];
    
    num = 0;
    
    sprintf(str,"select id from balance "
                "where billing_account_id = %d and "
                "start_date = '%s' and end_date='%s' and active = 't'",
                pre->bacc,start,end);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"id");

			for (i = 0; i < num; i++) {
				pre->bal_id = atoi(PQgetvalue(res,i,fnum[0]));
			}
		}
		
		PQclear(res);
	}
}

int f_bal_count_query(PGconn *conn,rating *pre,char flag)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[2];
    int number;
    
    num = 0;
    number = 0;
    
    sprintf(str,"select count(*) as number from balance "
                "where billing_account_id = %d and "
                "active = '%c'",
                pre->bacc,flag);

	res = db_pgsql_exec(conn,str);
    
    if(res != NULL) {
		num = PQntuples(res);
	
		if(num) {
			fnum[0] = PQfnumber(res,"number");

			for (i = 0; i < num; i++) {
				number = atoi(PQgetvalue(res,i,fnum[0]));
			}
		}
    
		PQclear(res);
    }
    
    return number;
}

int f_bplan_tree_num(PGconn *conn,rating *pre)
{
    PGresult *res;

    char str[512];
    int num;

    num = 0;

    sprintf(str,"select id from bill_plan_tree where root_bplan_id = %d",pre->bplan);

	res = db_pgsql_exec(conn,str);
	if(res != NULL) {
		num = PQntuples(res);
	
		PQclear(res);
	}
    
    return num;
}

rates *f_rate_query(PGconn *conn,rating *pre)
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
        
			//rt = (rates *)malloc((num+1)*sizeof(rates));
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

int chk_rt_tperiods(int start,int end,int ts)
{
    if(start == 0) return 1;
    else
    {
		if(start > ts) return 0;
		else
		{
			if(end == 0) return 1;
			else
			{
				if((end < ts)) return 0;
				else return 1;
			}
		}
    }
}

void chk_tr_opt(PGconn *conn,rating *pre)
{
	if(f_time_cond_query_id(conn,pre)) {
		f_time_cond_query_v2(conn,pre);
		if(!tc_ts_cmp(pre)) pre->tariff = 0;
		free(pre->tc);
	}
	
    f_free_billsec_query(conn,pre);
//    if(pre->free_billsec_limit == 0) f_free_billsec_query_2(conn,pre);
}

void balance_exec(PGconn *conn,rating *pre,char *start,char *end)
{
	balance(conn,pre,start,end);

	f_bal_query_v3(conn,pre,start,end);

	if((pre->bal_id > 0)) {
		update_balance(conn,pre->bal_id,pre->balanse);
	} else {
		create_balance_pgsql_v2(conn,pre,start,end);
		
		LOG("balance_exec()","create new balance,bacc %d,start %s,end %s,balance_id %d",
			pre->bacc,start,end,pre->bal_id);
	}
}

void prefix_searching(PGconn *conn,rating *pre)
{
	rates *rt;
	int p,len;
	
	rt = 0;
	
#if RATES_NOCACHE
	rt = f_rate_query(conn,pre);
#else
	rt = rates_get_bplan_rates(conn,pre);
#endif
	
	if(rt > 0) {
		p=0;
		while((rt[p].tariff) > 0)  {
			if(chk_rt_tperiods(rt[p].start_period,rt[p].end_period,pre->ts) == 0) {
				p++;
				continue;
			}
	
			if(!strcmp(rt[p].prefix,"&")) goto without_prefix;
			
			len = strlen(rt[p].prefix);
			
			if(strncmp(pre->cld,rt[p].prefix,len) == 0) {
				without_prefix:
				pre->tariff = rt[p].tariff;
				pre->rate = rt[p].rate;
				pre->prefix = rt[p].prefix_id;
				pre->free_billsec_id = rt[p].free_billsec_id;
				chk_tr_opt(conn,pre);
				if(pre->tariff) break;
			}
			p++;
		}
#if RATES_NOCACHE
		mem_free(rt); 
#endif
	}
}

int prerating_process(PGconn *conn,rating *pre)
{
	int bal_num;
	int online_rating;
    pcard *card;
	time_t tt;
	struct tm  *tm;
	int day,bday,pp;

	bal_num = 0;
    card = pre->card;
    
    if(pre->epoch) {
		/* offline rating process */
		tt = pre->epoch;
		online_rating = 0;
	} else {
		/* online rating process */
		tt = pre->ts;
		online_rating = 1;
	}
						
	/* blocking card,when have more balance with flag 't' than 2 */
    if((card->id)&&(online_rating)) {	
		/* day of payment */
		bal_num = f_bal_count_query(conn,pre,'t');
		
		if(bal_num > 1) {
			tm   = localtime(&tt);
			day  = tm->tm_mday;
					
			bday = atoi(pre->billing_day);
			
			if(bday == 0) bday = atoi(billing_day);

			if(bal_num > rt_eng.bal_num) {
				set_pcard_status(conn,card->id,2);
				LOG("prerating_process()","Blocking PCard,card_id %d",card->id);
				return 1;				
			}
			
//			if(pre->day_of_payment == 0) pp = day_of_payment;
			if(pre->day_of_payment == 0) pp = 0;
			else pp = pre->day_of_payment;
								
			/* DayOfPayment checking,if 'dday > 0' */
			if(pp > 0) {
				if(bday > pp) {
					if(day < bday) {
						if(day >= pp) {
							set_pcard_status(conn,card->id,2);
							LOG("prerating_process()","Blocking PCard,card_id %d",card->id);
							return 1;
						}					
					}
				} else {
					if(day >= pp) {
						set_pcard_status(conn,card->id,2);
						LOG("prerating_process()","Blocking PCard,card_id %d",card->id);
						return 1;
					}
				}
			}
		}
	}

	if((strlen(card->start))&&(strlen(card->end))) {
		/* Check date validation */
		if((check_date_valid(card->start) == 0)||(check_date_valid(card->end) == 0)) {
			set_pcard_status(conn,card->id,PCARD_BLOCK);
			
			LOG("prerating_process()","start_date(%s) or end date(%s) is not valid!The pcard(%d) is blocked!",
			card->start,card->end,card->id);

			return 1;
		}
		
		/* generate new balance in the same time period */
		if((f_bal_query_2(conn,pre,card->start,card->end,'f'))&&(card->type == CREDIT_CARD)) {	
			if((strcmp(pre->bal_lupdate,card->end) < 0)) {
				pre->bal_id = 0;
				pre->balanse = 0;
				strcpy(card->start,pre->bal_lupdate);
					
				if(log_debug_level >= LOG_LEVEL_INFO) {
					LOG("prerating_process()","changing of the start_date - start: %s,last_update: %s,end: %s",
						card->start,pre->bal_lupdate,card->end);
				}
			}
		}
/*		
		balance_exec(conn,pre,card->start,card->end);
		if(pre->bal_id = 0) {
			LOG("prerating_process()","create new balance ERROR(balance_id is NULL),bacc %d,start %s,end %s",pre->bacc,card->start,card->end);
			return 1;
		}
	
		if((pre->free_billsec_limit)) {
			free_billsec_exec(conn,pre,card->start,card->end);
			if(pre->free_id == 0) {
				LOG("prerating process()","ERROR,no free_billsec balance,bacc: %d,start: %s,end: %s",pre->bacc,card->start,card->end);
				return 1;
			}
		} */
		
		balance(conn,pre,card->start,card->end);
		f_bal_query_v3(conn,pre,card->start,card->end);
		
		if(pre->free_billsec_limit) {
			free_billsec_balance_v2(conn,pre,card->start,card->end);
			f_free_billsec_bal_query_2(conn,pre);
		}
    } else {
		LOG("prerating process()","ERROR,no start or end date,bacc: %d,start: %s,end: %s",pre->bacc,card->start,card->end);
		
		return 1;
    }
    
    return 0;
}

tariff *rate_searching(PGconn *conn,rating *pre)
{
	tariff *tr = 0;
	
	if(pre->tr != NULL) {
		LOG("rate_searching()","get tariff from 'cc_tbl->pre->tr'");
		return pre->tr;
	}
	
	if(pre->bplan) prefix_searching(conn,pre);
	else { 
		LOG("rate_searching()","bill_plan_id is null,call_uid %s",pre->call_uid);
		
		goto end_func;
	}
	
	if(pre->card) { 
		if(prerating_process(conn,pre)) {
			LOG("rate_searching()","A error from prerating_process,call_uid %s",pre->call_uid);
			 
			goto end_func;
		}
	}
	
	if(pre->tariff) tr = f_tariff_query(conn,pre);
	else LOG("rate_searching()","The tariff is not found!");
	
	end_func:
	return tr;
}

tariff *f_tariff_query(PGconn *conn,rating *pre)
{    
    PGresult *res;
    tariff *tr;

    char str[512];
    int i,num;
    int fnum[5];
    
    tr = 0;
    num = 0;

	if(pre->tariff <= 0) goto end;
	
    sprintf(str,"select pos,delta_time,fee,iterations from calc_function where tariff_id = %d order by pos ",pre->tariff);

	res = db_pgsql_exec(conn,str);
    
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"pos");
			fnum[1] = PQfnumber(res,"delta_time");
			fnum[2] = PQfnumber(res,"fee");
			fnum[3] = PQfnumber(res,"iterations");

//			tr = (tariff *)malloc((num+1)*sizeof(tariff));
			tr = (tariff *)mem_alloc_arr((num+1),sizeof(tariff));

			for (i = 0; i < num; i++) {
				tr[i].pos = atoi(PQgetvalue(res,i,fnum[0]));
				tr[i].delta = atoi(PQgetvalue(res,i,fnum[1]));
				tr[i].fee = atof(PQgetvalue(res,i,fnum[2]));
				tr[i].iterations = atoi(PQgetvalue(res,i,fnum[3]));
			}
        
			tr[i].pos = 0;
		}
		
		PQclear(res);
    }
    
    end:
    return tr;
}

void tr_free(tariff *tr)
{
    mem_free(tr);
}

void f_cdr_id_query(PGconn *conn,rating *pre)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[3];

    num = 0;
    
    sprintf(str,"select id from cdrs where call_uid = '%s'",pre->call_uid);

	res = db_pgsql_exec(conn,str);
    
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"id");

			for (i = 0; i < num; i++) {
				pre->cdr_id = atoi(PQgetvalue(res,i,fnum[0]));
			}
		}
    
		PQclear(res);
	}
}

void f_rating_id_query(PGconn *conn,rating *pre)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[3];

    num = 0;
    pre->rating_id = 0;
    
    sprintf(str,"select id from rating where call_id = %d and billing_account_id = %d and rate_id = %d and call_price = %f",
            pre->cdr_id,pre->bacc,pre->rate,pre->cprice);

	res = db_pgsql_exec(conn,str);
    
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"id");

			for (i = 0; i < num; i++) {
				pre->rating_id = atoi(PQgetvalue(res,i,fnum[0]));
			}
		}
    
		PQclear(res);
	}
}

void f_rating_id_query_2(PGconn *conn,rating *pre)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[3];

    num = 0;
    
    sprintf(str,"select id from rating where call_id = %d",pre->cdr_id);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"id");

			for (i = 0; i < num; i++) {
				pre->rating_id = atoi(PQgetvalue(res,i,fnum[0]));
			}
		}
    
		PQclear(res);
	}
}

void insert_rating_pgsql(PGconn *conn,rating *pre)
{
	int card_id;
    PGresult *res;
    char str[512];

	if(pre->card == 0) card_id = 0;
	else card_id = pre->card->id ;

    sprintf(str,
			"insert into rating"
			" (call_price,call_billsec,rate_id,billing_account_id,call_id,rating_mode_id,pcard_id,time_condition_id,"
			"call_ts,last_update,free_billsec_id)"
			" values (%f,%d,%d,%d,%d,%d,%d,%d,'%s','now()',%d)",
			pre->cprice,pre->billsec,pre->rate,pre->bacc,pre->cdr_id,pre->rating_mode_id,card_id,pre->tc_id,pre->timestamp,pre->free_billsec_id);
    
    res = db_pgsql_exec(conn,str);
    
    if(res != NULL) PQclear(res);
}

void create_balance_pgsql(PGconn *conn,rating *pre,char *start,char *end)
{
    PGresult *res;
    char str[512];

    sprintf(str,
    "insert into balance (billing_account_id,start_date,end_date,active) values (%d,'%s','%s','t')",
    pre->bacc,start,end);

	res = db_pgsql_exec(conn,str);

    if(res != NULL) PQclear(res);
}

void create_balance_pgsql_v2(PGconn *conn,rating *pre,char *start,char *end)
{
    PGresult *res;
    char str[512];

    sprintf(str,
    "insert into balance (billing_account_id,start_date,end_date,active,amount,last_update) values (%d,'%s','%s','t',%f,'now()')",
    pre->bacc,start,end,pre->balanse);

	res = db_pgsql_exec(conn,str);

    if(res != NULL) PQclear(res);
    
    f_bal_query_v3(conn,pre,start,end);
}

/*
void update_cdr_pgsql(PGconn *conn,rating *pre,char leg)
{
    PGresult *res;
    char str[512];

	if((pre->cdr_id > 0)&&((leg == 'a')||(leg == 'b')))
	{
		sprintf(str,"update cdrs set leg_%c = %d where id = %d",leg,pre->rating_id,pre->cdr_id);

		res = db_pgsql_exec(conn,str);
		
		PQclear(res);
	}
}
*/

void update_balance(PGconn *conn,int bid,double bill)
{
    PGresult *res;
    char str[512];

    sprintf(str,"update balance set amount = %f,last_update = now() where id = %d",bill,bid);

	res = db_pgsql_exec(conn,str);
    
    if(res != NULL) PQclear(res);
}

void update_balance_2(PGconn *conn,int bid,char *end)
{
    PGresult *res;
    char str[512];

    sprintf(str,"update balance set end_date = '%s',last_update = now() where id = %d",end,bid);
    
    res = db_pgsql_exec(conn,str);
    
    if(res != NULL) PQclear(res);
}

void calculate(tariff *tr,rating *pre)
{
    int p,iterations;
    int checksec,billsec;
    int free_billsec_limit;
    int free_billsec;
    double cprice;

    checksec = 0 ;
    cprice = 0;
    free_billsec_limit = 0;
    free_billsec = 0;

    checksec = pre->billsec;
    billsec = checksec;
    free_billsec_limit = pre->free_billsec_limit;

    p=0;
    while(tr[p].pos)
    {
	if((tr[p].iterations) == 0)
	{
	    iterations = ((checksec / tr[p].delta));
	    cprice = (cprice + (tr[p].fee * iterations));
	}
	else
	{
	    cprice = (cprice + (tr[p].fee * tr[p].iterations));
	}
	
	if(checksec < (tr[p].delta * tr[p].iterations))
	{
	    billsec = (tr[p].delta * tr[p].iterations);
	    break;
	}
	
	checksec = (checksec - (tr[p].delta * tr[p].iterations));
	p++;
    }
    
    if(free_billsec_limit)
    {
	free_billsec = ((free_billsec_limit)-(pre->free_billsec));
	if(checksec <= free_billsec) cprice = cprice * (-1);
	else 
        {
        //    $check_billsec_2 = ($check_billsec - $process->free_billsec);
        //    echo "check_billsec_2=".$check_billsec_2."\n";
            /* Pri tazi situaciq trqbva 'billsec'-a da se razdeli na dve:
                => check_billsec_1 = free_billsec
                => check_billsec_2 = (check_billsec - free_billsec)
                Razgovoryt trqbva da se preiz4isli s novite vremena!!!
                ??? 
            */ 
        }
    }
    
// proverka za checksec,dali e nula ?! ako e , zna4i vsi4ko e smetnato ...ako ne ...ostanala e nesmetnata 4ast !!!
    pre->cprice = cprice;
    pre->billsec = billsec;
}

void balance(PGconn *conn,rating *pre,char *time1,char *time2)
{
    double bill;
    PGresult *res;

    char str[512];
    int n,i;
    
    bill = 0;
    				
    sprintf(str,"select sum(call_price) "
                "from rating "
                "where billing_account_id = %d and "
                "call_ts >= '%s' and "
                "call_ts <= '%s' and "
                "call_price > 0",
                 pre->bacc,time1,time2);
    
	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		n = PQfnumber(res, "sum");
		
		for (i = 0; i < PQntuples(res); i++) {
			bill = atof(PQgetvalue(res, i, n));
		}
	 
		PQclear(res);
	}

    pre->balanse = bill;  
}

void copy_cdr_to_rating(cdr_t *the_cdr,rating *pre)
{
	int tmp_ts = convert_ts_to_epoch(the_cdr->start_ts);
	
    pre->cdr_server_id = the_cdr->cdr_server_id;
    //pre->cdr_rec_type_id = the_cdr->cdr_rec_type_id;
    strcpy(pre->call_uid,the_cdr->call_uid);
    strcpy(pre->clg,the_cdr->calling_number);
    strcpy(pre->cld,the_cdr->called_number);
    pre->billsec = the_cdr->billsec;
    pre->cdr_id = the_cdr->id;
    strcpy(pre->timestamp,the_cdr->start_ts);
    pre->dow = get_weekday_from_epoch(tmp_ts);
    pre->ts = the_cdr->start_epoch;
    strcpy(pre->account_code,the_cdr->account_code);
    strcpy(pre->src_context,the_cdr->src_context);
    strcpy(pre->dst_context,the_cdr->dst_context);
    strcpy(pre->src_tgroup,the_cdr->src_tgroup);
    strcpy(pre->dst_tgroup,the_cdr->dst_tgroup);
    pre->epoch = tmp_ts;
    pre->clg_nadi = the_cdr->clg_nadi;
    pre->cld_nadi = the_cdr->cld_nadi;
    pre->billusec = the_cdr->billusec;
}

void rating_loop(rate_engine *rt_eng,rt_stat_t *stat)
{
	rating rt;
	rating *pre;
	struct timeval tim;
	double t1,t2;
	char msg[512];
	struct timeval times;
	
	cdr_t *cdrs = 0;
	
	unsigned int total   = 0;
	unsigned int success = 0;
	unsigned int error   = 0;
	unsigned int minutes = 0;
	
	pre = &rt;
	
	if(log_debug_level >= LOG_LEVEL_INFO) {
		gettimeofday(&tim, NULL);
		t1 = tim.tv_sec+(tim.tv_usec/1000000.0);
	}
	
	cdrs = cdr_get_cdrs(rt_eng->conn,rt_eng->leg,0);
	if(cdrs) {
		while(cdrs[total].id > 0) {
			rating_init(pre);
			
			copy_cdr_to_rating(&cdrs[total],pre);
			
			gettimeofday(&times, NULL);
			pre->start_timer = (((times.tv_sec)*1000000)+(times.tv_usec));
			
			rating_main(rt_eng->conn,pre,rt_eng->leg,cdrs[total].cdr_rec_type_id);
			
			gettimeofday(&times, NULL);
			pre->current_timer = (((times.tv_sec)*1000000)+(times.tv_usec));
			
			char msg4[1024];
			sprintf(msg4,"rating call times: %f,cdr_id: %d",
			(double)((pre->current_timer)-(pre->start_timer))/1000000,cdrs[total].id);
			
			LOG("rating_loop()",msg4);
			
			if((pre->rating_id) > 0) {
				success++;
				minutes += (pre->billsec / 60);
			} else error++;
			
			usleep(WAIT_RATING);
			
			total++;
		}
		
		sprintf(msg,"Number of rating cdrs (leg %c) = [%d/%d]",rt_eng->leg,total,error);
		
		LOG("rating_loop",msg);
		
		mem_free(cdrs);
    }
    
	if(stat != NULL) {
		rt_stat_put(stat,total,success,error,minutes);
	}
	
    if(log_debug_level >= LOG_LEVEL_INFO) {
		gettimeofday(&tim, NULL);
		t2 = tim.tv_sec+(tim.tv_usec/1000000.0);
				
		char msg3[1024];
		sprintf(msg3,"times: %f %f",(t2-t1),((t2-t1)/success));
//		sprintf(msg3,"times: %f %f",(t2-t1),((t2-t1)/pp));
	
		LOG("rating_loop()",msg3);
	}
}

void *RateEngine(void *dt)
{
	int n,cycles;
	
	shm_mem_t rt_shm;
	rt_stat_t *stat;
	re5_mgr_mem_t *ptr;
	
	ptr = re5_mgr_mem_init(&re5_mgr_shm,2);
	
	rt_stat_init(&rt_shm,1);
	stat = (rt_stat_t *)rt_shm.ptr;
	
	pthread_mutex_init(&cache_tbl_lock, NULL);
	pthread_mutex_init(&sync_bt_thread,NULL);
	
	cycles = rt_eng.rating_interval / LOOP_PAUSE;
	
	loop:
	{
		LOG("RateEngine","RateEngine is started ...PID:%d",getpid());
		
		rates_cache_tbl_free();
		
		if(ptr) ptr->rt_ts = time(NULL);
		
		/* daemon,without leg value */
		if(rt_eng.leg == '\0') {
			rt_eng.leg = 'a';
			LOG("RateEngine","leg_a ...");
			rating_loop(&rt_eng,stat);
			
			LOG("RateEngine","leg_b ...");
			rt_eng.leg = 'b';
			rating_loop(&rt_eng,stat);
			
			rt_eng.leg = '\0';
		} else {
			LOG("RateEngine","leg_%c ...",rt_eng.leg);
			rating_loop(&rt_eng,stat);
		}
		
		LOG("RateEngine","RateEngine is finished!");
		
		if(rt_eng.active == 't') {
			n = 0;
			while(loop_flag == 't') {
				if(n == cycles) break;
				
				n++;
				
				sleep(LOOP_PAUSE);
				
				if(ptr) {
					loop_flag = ptr->rt_flag;
				}
			}
			
			if(loop_flag == 't') {
				goto loop;
			}
		} else {
			loop_flag = 'f';
		}
	}
	
	pthread_mutex_destroy(&sync_bt_thread);
	pthread_mutex_destroy(&cache_tbl_lock);
	
	rt_stat_free(&rt_shm,1);
	
	re5_mgr_mem_free(&re5_mgr_shm,0);
	
	PQfinish(rt_eng.conn);
	
	LOG("RateEngine","Rating thread is finished!");
	
	pthread_exit(NULL);
}
