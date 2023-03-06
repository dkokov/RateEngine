#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../db/db.h"
#include "../../mem/mem.h"
#include "../../log/rt_log.h"

#include "rt_data.h"
#include "rt_json.h"
#include "rt_data_q.h"
/*
int f_bacc_sms_sql_query(db_t *dbp,rating *pre,char *table,char *cond)
{
	int i;
    char str[SQL_BUF_LEN];
	
 	db_sql_result_t *result;

	if(dbp == NULL) return -1;
	
	if(dbp->t == sql) {		
		sprintf(str,"SELECT clg.billing_account_id,clg_df.sm_bill_plan_id,bp.start_period,bp.end_period,"
					"bacc.billing_day,bacc.round_mode_id,bacc.day_of_payment "
					" from %s as clg,%s_deff as clg_df,bill_plan as bp,billing_account as bacc "
					" where clg.id = clg_df.%s_id and "
					" bp.id = clg_df.sm_bill_plan_id and "
					" clg.%s = '%s' and "
					" bacc.id = clg.billing_account_id and"
					" bacc.cdr_server_id = %d ",table,table,table,table,cond,pre->cdr_server_id);
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;

			for(i = 0;i < result->rows;i++) {
				pre->bacc = atoi(result->cols_list[0].rows_list[i].row);							
				pre->bplan = atoi(result->cols_list[1].rows_list[i].row);
				pre->bplan_start_period = atoi(result->cols_list[2].rows_list[i].row);
				pre->bplan_end_period = atoi(result->cols_list[3].rows_list[i].row);
				strcpy(pre->billing_day,result->cols_list[4].rows_list[i].row);
				pre->round_mode_id = atoi(result->cols_list[5].rows_list[i].row);
				pre->day_of_payment = atoi(result->cols_list[6].rows_list[i].row);
			}
						
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -2;
	
	return 0;
}

int f_bal_query_2(db_t *dbp,racc_t *rtp,char *start,char *end,char flag)
{	
	int num;
    char str[DB_BUF_LEN];
	
	rating_t *pre;
 	db_sql_result_t *result;

	if(dbp == NULL) return 0;
	
	pre = rtp->pre;
	
	if(dbp->t == sql) {		
		sprintf(str,"select amount,id,last_update from balance "
					"where billing_account_id = %d and "
					"start_date = '%s' and end_date='%s' and active = '%c'"
					" order by last_update", rtp->bacc_ptr->id,start,end,flag);
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;

			num = result->rows;

			if(result->rows == 1) {
				pre->balanse = atoi(result->cols_list[0].rows_list[0].row); // atoi() ? ne trqbwa li da e atof() ????					
				pre->bal_id  = atoi(result->cols_list[1].rows_list[0].row);
				strcpy(pre->bal_lupdate,result->cols_list[2].rows_list[0].row);
			}
						
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return 0;
	
	return num;
}
*/

int rt_data_q_chk_rate_periods(int start,int end,int ts)
{
    if(start == 0) return 1;
    else {
		if(start > ts) return 0;
		else {
			if(end == 0) return 1;
			else {
				if((end < ts)) return 0;
				else return 1;
			}
		}
    }
}

int rt_data_q_bplan_tree_num(db_t *dbp,racc_t *rtp)
{
    int number;
    char str[DB_BUF_LEN];
	
 	db_sql_result_t *result;

	if(dbp == NULL) return 0;
	
	if(dbp->t == sql) {		
		sprintf(str,"select id from bill_plan_tree where root_bplan_id = %d",rtp->bplan_ptr->id);

		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;

			number  = result->rows;
						
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return 0;
	
	return number;
}

int rt_data_q_bal_count(db_t *dbp,racc_t *rtp,char flag)
{
    int number,ret;
    char str[DB_BUF_LEN];
	
 	db_sql_result_t *result;
	db_nosql_result_t *tmp;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	if(rtp == NULL) return -999;
	if(rtp->bacc_ptr == NULL) return -998;
		
	number = 0;
	
	if(dbp->t == sql) {		
		sprintf(str,"select count(*) as number from balance where billing_account_id = %d and active = '%c'",rtp->bacc_ptr->id,flag);
                		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;

			if(result->rows == 1) number  = atoi(result->cols_list[0].rows_list[0].row);
						
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else if(dbp->t == nosql) {
		sprintf(str,"keys %s%s*",RT_JSON_HDR_PATTERN_BAL,rtp->bacc_ptr->username);
		
		ret = db_command(dbp,str);
		if(ret < 0) db_error(ret);
		else {
			if(dbp->conn->result != NULL) {
				tmp = (db_nosql_result_t *)dbp->conn->result;
				number = tmp->elements;
			
				db_nosql_result_free(tmp);
				dbp->conn->result = NULL;
			}
			
			db_free_result(dbp);
			dbp->conn->res = NULL;
		}
		
	} else return 0;
	
	return number;
}

int rt_data_q_pcard_sql(db_t *dbp,bacc_t *bpt,int pcard_status_id)
{
    int i;
    char str[DB_BUF_LEN];

    pcard_t *card;
 	db_sql_result_t *result;
 			
	sprintf(str,"SELECT id,amount,start_date,end_date,pcard_status_id,pcard_type_id,call_number"
				" from pcard where billing_account_id = %d and pcard_status_id = %d "
				"order by start_date desc",
				bpt->id,pcard_status_id);
		
	db_select(dbp,str);
	db_fetch(dbp);
				
	if(dbp->conn->result != NULL) {
		result = (db_sql_result_t *)dbp->conn->result;
			
		if(result->rows > 0) {
			card = (pcard_t *)mem_alloc_arr(result->rows,sizeof(pcard_t));
			if(card == NULL) {
				//bpt->pcard_i = -1;
				return -1;
			}
			
			for(i = 0;i < result->rows;i++) {	
				card[i].id = atoi(result->cols_list[0].rows_list[i].row);
				card[i].amount = atof(result->cols_list[1].rows_list[i].row);
				strcpy(card[i].start,result->cols_list[2].rows_list[i].row);
				strcpy(card[i].end,result->cols_list[3].rows_list[i].row);
				card[i].status = atoi(result->cols_list[4].rows_list[i].row);
				card[i].type = atoi(result->cols_list[5].rows_list[i].row);
				card[i].call_number = atoi(result->cols_list[6].rows_list[i].row);
			}
		
			bpt->pcard_ptr = card;
			//bpt->pcard_i = i;
		} //else bpt->pcard_i = -1;
			
		db_sql_result_free(result);
		dbp->conn->result = NULL;
	}
	
	return 0;      
}

int rt_data_q_pcard_nosql(db_t *dbp,bacc_t *bpt,int pcard_status_id)
{
	return 0;
}

int rt_data_q_pcard(db_t *dbp,bacc_t *bpt,int status)
{
	int ret;
	
	if(bpt == NULL) return -999;
	
	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	if(dbp->t == sql) {
		ret = rt_data_q_pcard_sql(dbp,bpt,status);
	} else if(dbp->t == nosql) {
		ret = rt_data_q_pcard_nosql(dbp,bpt,status);
	} else return DB_ERR_TYPE_UNK;
	
	return ret;
}

int rt_data_q_pcard_set(db_t *dbp,bacc_t *bpt,int status)
{
	int ret;
    char str[DB_BUF_LEN];
    
	if(dbp == NULL) return DB_ERR_DBP_NUL;

	if(dbp->t == sql) {
		sprintf(str,"update pcard set pcard_status_id = %d,last_update = 'now()' where id = %d ", status, bpt->pcard_ptr->id);
		
		ret = db_update(dbp,str);
	} else if(dbp->t == nosql) {
		
	} else return DB_ERR_TYPE_UNK;
	
	return ret;
}
/*
int rt_data_q_bal_id(db_t *dbp,racc_t *rtp,char *start,char *end)
{
    char str[DB_BUF_LEN];
	
 	db_sql_result_t *result;

	if(dbp == NULL) return -1;
	
	if(dbp->t == sql) {		
		sprintf(str,"select id from balance where billing_account_id = %d and start_date = '%s' and end_date='%s' and active = 't'", rtp->bacc_ptr->id,start,end);
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;

			if(result->rows == 1) rtp->bal_ptr->id  = atoi(result->cols_list[0].rows_list[0].row);
						
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -2;
	
	return 0;	
}*/
/*
int rt_data_q_bal_sql(db_t *dbp,racc_t *rtp,char *time1,char *time2)
{
    char str[DB_BUF_LEN];
	
	db_sql_result_t *result;

	sprintf(str,"select sum(call_price) "
				"from rating "
				"where billing_account_id = %d and "
				"call_ts >= '%s' and "
				"call_ts <= '%s' and "
				"call_price > 0", rtp->bacc_ptr->id,time1,time2);		
		
	db_select(dbp,str);
	db_fetch(dbp);
				
	if(dbp->conn->result != NULL) {
		result = (db_sql_result_t *)dbp->conn->result;

		if(result->rows == 1) rtp->bal_ptr->amount  = atof(result->cols_list[0].rows_list[0].row);
						
		db_sql_result_free(result);
		dbp->conn->result = NULL;
	}

	return 0;
}*/

int rt_data_q_bal_sql(db_t *dbp,racc_t *rtp,char *time1,char *time2)
{
    char str[DB_BUF_LEN];
	
 	db_sql_result_t *result;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	sprintf(str,"select amount,id from balance "
				"where billing_account_id = %d and start_date = '%s' and end_date='%s' and active = 't'",rtp->bacc_ptr->id,time1,time2);
		
	db_select(dbp,str);
	db_fetch(dbp);
				
	if(dbp->conn->result != NULL) {
		result = (db_sql_result_t *)dbp->conn->result;

		if(result->rows == 1) {
			rtp->bal_ptr->amount = atof(result->cols_list[0].rows_list[0].row); 					
			rtp->bal_ptr->id  = atoi(result->cols_list[1].rows_list[0].row);
		}
						
		db_sql_result_free(result);
		dbp->conn->result = NULL;
	}
	
	return DB_OK;
}

int rt_data_q_bal_nosql(db_t *dbp,racc_t *rtp,char *time1,char *time2)
{
	int ret;
	char str[DB_BUF_LEN];
	
	rt_json_t rt_ptr;
	db_nosql_result_t *result;	
	
	sprintf(str,"get %s%s_%s",RT_JSON_HDR_PATTERN_BAL,rtp->bacc_ptr->username,time1);
	
	ret = db_get(dbp,str);
	if(ret == DB_OK) {
		result = (db_nosql_result_t *)dbp->conn->result;
		
		if(result->len > 0) {			
			rt_ptr.msg = result->str;
			
			ret = rt_json_balance_get(&rt_ptr,rtp);
		}
	}
		
	db_free_result(dbp);
		
	db_nosql_result_free((db_nosql_result_t *)dbp->conn->result);
	dbp->conn->result = NULL;

	return ret;
}

/* Get the current balance from the db/mem */
int rt_data_q_bal(db_t *dbp,racc_t *rtp,char *time1,char *time2)
{
//    int ret;

	if(rtp == NULL) return -999;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	if(dbp->t == sql) {
		//rt_data_q_bal_id(dbp,rtp,time1,time2);
		rt_data_q_bal_sql(dbp,rtp,time1,time2);
	} else if(dbp->t == nosql) {
		rt_data_q_bal_nosql(dbp,rtp,time1,time2);
	} else return DB_ERR_TYPE_UNK;
	
	return 0;
}

int rt_data_q_bal_add_sql(db_t *dbp,racc_t *rtp,char *start,char *end)
{
	int ret;
	char str[DB_BUF_LEN];

	
	if(rtp->bal_ptr->id > 0) {
		sprintf(str,"update balance set amount = %f,last_update = now() where id = %d",rtp->bal_ptr->amount,rtp->bal_ptr->id);
		
		ret = db_update(dbp,str);
	} else {
		sprintf(str,"insert into balance (billing_account_id,start_date,end_date,active,amount,last_update) values (%d,'%s','%s','t',%f,'now()')",
				rtp->bacc_ptr->id,start,end,rtp->bal_ptr->amount);
		
		ret = db_insert(dbp,str);	
	}

	return ret;
}

int rt_data_q_bal_add_nosql(db_t *dbp,racc_t *rtp,char *start,char *end)
{
	int ret;
	char str[DB_BUF_LEN];
    
    rt_json_t rt_ptr;

	rt_json_balance_put(rtp,&rt_ptr);
		
	sprintf(str,"%s_%s",rt_ptr.header,start);
		
	ret = db_set(dbp,str,rt_ptr.msg);
	
	return ret;
}

int rt_data_q_bal_add(db_t *dbp,racc_t *rtp,char *start,char *end)
{ 
    int ret;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	if(dbp->t == sql) {		
		ret = rt_data_q_bal_add_sql(dbp,rtp,start,end);
	} else if(dbp->t == nosql) {
		ret = rt_data_q_bal_add_nosql(dbp,rtp,start,end);
	} else return DB_ERR_TYPE_UNK;
	
	return ret;
}

int rt_data_q_rating_id(db_t *dbp,racc_t *rtp)
{
    char str[DB_BUF_LEN];
	
	rating_t *pre;
 	db_sql_result_t *result;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	pre = rtp->pre;
	
	if(dbp->t == sql) {		
		sprintf(str,"select id from rating where call_id = %d and billing_account_id = %d and rate_id = %d and call_price = %f",
					pre->cdr_id,rtp->bacc_ptr->id,pre->rate_id,pre->cprice);		
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;

			if(result->rows == 1) pre->rating_id  = atoi(result->cols_list[0].rows_list[0].row);
						
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -2;
	
	return 0;	
}

int rt_data_q_rating_add_sql(db_t *dbp,racc_t *rtp)
{
 	int card_id;
    char str[SQL_BUF_LEN];
    
    rating_t *pre;
    
	if(dbp == NULL) return -1;
	
	pre = rtp->pre;
	
	if(rtp->bacc_ptr->pcard_ptr->id == 0) card_id = 0;
	else card_id = rtp->bacc_ptr->pcard_ptr->id ;
	
	if(dbp->t == sql) {
		sprintf(str,"insert into rating"
					" (call_price,call_billsec,rate_id,billing_account_id,call_id,rating_mode_id,pcard_id,time_condition_id,"
					"call_ts,last_update,free_billsec_id)"
					" values (%f,%d,%d,%d,%d,%d,%d,%d,'%s','now()',%d)",
					pre->cprice,pre->billsec,pre->rate_id,rtp->bacc_ptr->id,pre->cdr_id,pre->rating_mode_id,card_id,pre->tc_id,pre->timestamp,pre->free_billsec_id);	
		
		db_insert(dbp,str);		
	} else return -2;
	
	return 0;   
}

int rt_data_q_rating_add_nosql(db_t *dbp,racc_t *rtp)
{	
	int ret;
//	char buf[DB_BUF_LEN];
	
	rt_json_t rt_ptr;

	rt_json_rated_put(rtp->pre,&rt_ptr);

//	sprintf(buf,"set %s '%s'",rt_ptr.header,rt_ptr.msg);
	
	ret = db_set(dbp,rt_ptr.header,rt_ptr.msg);
	if(ret < 0) {
		db_error(ret);
	}
		
	return ret;
}

int rt_data_q_rating_add(db_t *dbp,racc_t *rtp)
{
	int ret;
	
	rating_t *pre;
	
	if(rtp == NULL) return -999;
	if(rtp->pre == NULL) return -998;
	
	pre = rtp->pre;
	
	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	if(dbp->t == sql) {
		rt_data_q_rating_id(dbp,rtp);
			
		if(pre->rating_id > 0) pre->rating_id = 0; 
		else { 
			ret = rt_data_q_rating_add_sql(dbp,rtp);
			rt_data_q_rating_id(dbp,rtp);
		}
	} else if(dbp->t == nosql) {
		ret = rt_data_q_rating_add_nosql(dbp,rtp);
		pre->rating_id = 1;
	} else return DB_ERR_TYPE_UNK;

	return ret;
}

//int f_bacc_sql_query(db_t *dbp,rating *pre,char *table,char *cond)
//int f_bacc_sql_query(db_t *dbp,racc_t *rtp,char *table,char *cond)
int rt_data_q_racc_sql(db_t *dbp,racc_t *rtp)
{
	int i;
    char str[DB_BUF_LEN];
	char *table,*cond;
	int clg_nadi,cld_nadi;
	
 	db_sql_result_t *result;

	if(rtp == NULL) return -999;
	
	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	clg_nadi = -1;
	cld_nadi = -1;
		
	switch(rtp->rtm) {
		case rt_mode_clg:
			table = RT_SQL_CLG_TBL_STR;
			cond = rtp->pre->clg;
			break;
		case rt_mode_acode:
			table = RT_SQL_ACC_TBL_STR;
			cond = rtp->pre->account_code;
			break;
		case rt_mode_srcc:
			table = RT_SQL_SRCC_TBL_STR;
			cond = rtp->pre->src_context;
			break;
		case rt_mode_dstc:
			table = RT_SQL_DSTC_TBL_STR;
			cond = rtp->pre->dst_context;
			break;
		case rt_mode_srctg:
			table = RT_SQL_SRCTG_TBL_STR;
			cond = rtp->pre->src_tgroup;
			clg_nadi = rtp->pre->clg_nadi;
			cld_nadi = rtp->pre->cld_nadi;
			break;
		case rt_mode_dsttg:
			table = RT_SQL_DSTTG_TBL_STR;
			cond = rtp->pre->dst_tgroup;
			clg_nadi = rtp->pre->clg_nadi;
			cld_nadi = rtp->pre->cld_nadi;
			break;
		case rt_mode_sms:
			table = RT_SQL_CLG_TBL_STR;
			cond = rtp->pre->clg;
			clg_nadi = -2;
			cld_nadi = -2;			
			break;
		default:
			return -999;
	};
		
	if((clg_nadi == -1)&&(cld_nadi == -1)) {
		sprintf(str,"SELECT clg.billing_account_id,clg_df.bill_plan_id,bp.start_period,bp.end_period,"
					"bacc.billing_day,bacc.round_mode_id,bacc.day_of_payment "
					" from %s as clg,%s_deff as clg_df,bill_plan as bp,billing_account as bacc "
					" where clg.id = clg_df.%s_id and "
					" bp.id = clg_df.bill_plan_id and "
					" clg.%s = '%s' and "
					" bacc.id = clg.billing_account_id and"
					" bacc.cdr_server_id = %d ",table,table,table,table,cond,rtp->pre->cdr_server_id);
	} else if((clg_nadi == -2)&&(cld_nadi == -2)) {
		sprintf(str,"SELECT clg.billing_account_id,clg_df.sm_bill_plan_id,bp.start_period,bp.end_period,"
					"bacc.billing_day,bacc.round_mode_id,bacc.day_of_payment "
					" from %s as clg,%s_deff as clg_df,bill_plan as bp,billing_account as bacc "
					" where clg.id = clg_df.%s_id and "
					" bp.id = clg_df.sm_bill_plan_id and "
					" clg.%s = '%s' and "
					" bacc.id = clg.billing_account_id and"
					" bacc.cdr_server_id = %d ",table,table,table,table,cond,rtp->pre->cdr_server_id);		
	} else {
		sprintf(str,"SELECT src.billing_account_id,src_df.bill_plan_id,src_df.clg_nadi,src_df.cld_nadi,bp.start_period,bp.end_period,bacc.billing_day,bacc.round_mode_id"
					" from %s as src,%s_deff as src_df,bill_plan as bp,billing_account as bacc  "
					" where src.id = src_df.%s_id and "
					" bp.id = src_df.bill_plan_id and "
					" src.%s = '%s' and "
					" bacc.id = src.billing_account_id and "
					" bacc.cdr_server_id = %d and src_df.clg_nadi = %d and src_df.cld_nadi = %d",
					table,table,table,table,cond,rtp->pre->cdr_server_id,clg_nadi,cld_nadi);
	}
	
	db_select(dbp,str);
	db_fetch(dbp);
				
	if(dbp->conn->result != NULL) {
		result = (db_sql_result_t *)dbp->conn->result;
			
		for(i = 0; i < result->rows; i++) {
			rtp->bacc_ptr->id = atoi(result->cols_list[0].rows_list[i].row);
			rtp->bplan_ptr->id = atoi(result->cols_list[1].rows_list[i].row);
			rtp->bplan_ptr->bplan_start_period = atoi(result->cols_list[2].rows_list[i].row);
			rtp->bplan_ptr->bplan_end_period = atoi(result->cols_list[3].rows_list[i].row);
				
			//strcpy(pre->billing_day,result->cols_list[4].rows_list[i].row);
			rtp->bacc_ptr->billing_day = atoi(result->cols_list[4].rows_list[i].row);
				
			rtp->bacc_ptr->round_mode_id = atoi(result->cols_list[5].rows_list[i].row);
			rtp->bacc_ptr->day_of_payment = atoi(result->cols_list[5].rows_list[i].row);
		}
			
		db_sql_result_free(result);
		dbp->conn->result = NULL;
	}
	
	return DB_OK;
}

//tariff *f_tariff_query(db_t *dbp,rating *pre)
int rt_data_q_tariff_sql(db_t *dbp,racc_t *rtp)
{ 
	int i;
    char str[DB_BUF_LEN];
    
    calc_function_t *tr;
 	db_sql_result_t *result;
 	
	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	if(rtp->pre == NULL) return -999;
	if(rtp->pre->tariff_id <= 0) return -998;
	
	sprintf(str,"select pos,delta_time,fee,iterations from calc_function where tariff_id = %d order by pos ",rtp->pre->tariff_id);
		
	db_select(dbp,str);
	db_fetch(dbp);
				
	if(dbp->conn->result != NULL) {
		result = (db_sql_result_t *)dbp->conn->result;
				
		if(result->rows == 0) return DB_ERR_SQLRES_NUL;
			
		tr = (calc_function_t *)mem_alloc_arr((result->rows+1),sizeof(calc_function_t));
		if(tr != NULL) {
			for(i = 0;i < result->rows;i++) {
				tr[i].pos   = atoi(result->cols_list[0].rows_list[i].row);
				tr[i].delta = atoi(result->cols_list[1].rows_list[i].row);
				tr[i].fee   = atof(result->cols_list[2].rows_list[i].row);
				tr[i].iterations = atoi(result->cols_list[3].rows_list[i].row);
			}
        
			tr[i].pos = 0;
			
			rtp->bplan_ptr->rates_ptr->calc_funcs = tr;
		}
			
		db_sql_result_free(result);
		dbp->conn->result = NULL;
	}
	
	return DB_OK; 
}

int rt_data_q_tariff_nosql(db_t *dbp,racc_t *rtp)
{
	int ret;
	char buf[DB_BUF_LEN];
	
	rt_json_t rt_ptr;
	db_nosql_result_t *result;
	
	if(rtp->bplan_ptr == NULL) return -3;	
	if(rtp->bplan_ptr->rates_ptr == NULL) return -4;

	sprintf(buf,"get %s%s",RT_JSON_HDR_PATTERN_TARIFF,rtp->bplan_ptr->rates_ptr->tariff_name);
	
	ret = db_get(dbp,buf);
	if(ret == DB_OK) {
		result = (db_nosql_result_t *)dbp->conn->result;
		
		if(result->len > 0) {			
			rt_ptr.msg = result->str;
			
			rt_json_tariff_get(&rt_ptr,rtp->bplan_ptr->rates_ptr);
		}
	}
		
	db_free_result(dbp);
		
	db_nosql_result_free((db_nosql_result_t *)dbp->conn->result);
	dbp->conn->result = NULL;
	
	return ret;
}

int rt_data_q_rate_sql(db_t *dbp,racc_t *rtp)
{
	int i;
    int bplan_num;
    char str[SQL_BUF_LEN];
    
	rate_t *rt;
 	db_sql_result_t *result;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	rt = NULL;
	
	if(dbp->t == sql) {		
		bplan_num = rt_data_q_bplan_tree_num(dbp,rtp);

		if(bplan_num == 0) {
			sprintf(str,"select rt.id,rt.tariff_id,pr.prefix,tr.start_period,tr.end_period,rt.prefix_id,tr.free_billsec_id "
						" from rate as rt,prefix as pr,tariff as tr "
						" where rt.bill_plan_id = %d and pr.id = rt.prefix_id and "
						" rt.tariff_id = tr.id "
						" order by pr.prefix desc",
						rtp->bplan_ptr->id);
		} else {
			sprintf(str,"select rt.id,rt.tariff_id,pr.prefix,tr.start_period,tr.end_period,rt.prefix_id,tr.free_billsec_id "
						" from rate as rt,prefix as pr,tariff as tr,bill_plan_tree as tree "
						" where tree.root_bplan_id = %d and "
						" rt.bill_plan_id = tree.bill_plan_id and "
						" pr.id = rt.prefix_id and "
						" rt.tariff_id = tr.id "
						" order by pr.prefix desc",
						rtp->bplan_ptr->id);
		}
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			
			rt = (rate_t *)mem_alloc_arr((result->rows+1),sizeof(rate_t));
			if(rt != NULL) {
				for(i = 0;i < result->rows;i++) {
					rt[i].id = atoi(result->cols_list[0].rows_list[i].row);
					rt[i].tariff_id = atoi(result->cols_list[1].rows_list[i].row);
					strcpy(rt[i].prefix,result->cols_list[2].rows_list[i].row);
					rt[i].start_period = atoi(result->cols_list[3].rows_list[i].row);
					rt[i].end_period = atoi(result->cols_list[4].rows_list[i].row);
					rt[i].prefix_id = atoi(result->cols_list[5].rows_list[i].row);
				//	rt[i].free_billsec_id = atoi(result->cols_list[6].rows_list[i].row);
				}
			}
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -999;
	
	rtp->bplan_ptr->rates_ptr = rt;
	
	return 0;    
}

int rt_data_q_rate_nosql(db_t *dbp,racc_t *rtp,char *search)
{
	int ret;
	char buf[DB_BUF_LEN];
	
	rt_json_t rt_ptr;
	db_nosql_result_t *result;
	
	
	
			
	if(rtp->bplan_ptr == NULL) return -3;	
	
	rtp->bplan_ptr->rates_ptr = (rate_t *)mem_alloc(sizeof(rate_t));
	if(rtp->bplan_ptr->rates_ptr == NULL) return -4;
		
	sprintf(buf,"get %s%s_%s",RT_JSON_HDR_PATTERN_RATE,rtp->bplan_ptr->bplan_name,search);
	
	ret = db_get(dbp,buf);
	if(ret == DB_OK) {
		result = (db_nosql_result_t *)dbp->conn->result;
		
		if(result->len > 0) {			
			rt_ptr.msg = result->str;
			
			rt_json_rate_get(&rt_ptr,rtp->bplan_ptr->rates_ptr);
		}
	}
		
	db_free_result(dbp);
		
	db_nosql_result_free((db_nosql_result_t *)dbp->conn->result);
	dbp->conn->result = NULL;
	
	return ret;
}

/* left shift alg ,minus one symbol */
void rt_data_q_prefix_nosql(db_t *dbp,racc_t *rtp)
{
	int len,ret;
	char dst[CLD_LEN];
	
	rating_t *pre;
	
	pre = rtp->pre;
	
	len = strlen(pre->cld);
	strcpy(dst,pre->cld);
	
	again:
	ret = rt_data_q_rate_nosql(dbp,rtp,dst);
	if(ret < 0) {
		if(len > 0) {
			len--;
			dst[len] = '\0';
			
			goto again;
		}
	} else {
		ret = rt_data_q_tariff_nosql(dbp,rtp);
		
		if(ret < 0) printf("\ntariff error: %d\n",ret);
	}
}

void rt_data_q_prefix_sql(db_t *dbp,racc_t *rtp)
{
    int p,len;

	rate_t *rt;
	bplan_t *btp;
	rating_t *pre;
	
//	ret = rt_data_q_rate(dbp,rtp);

//	if(ret < 0) return;

	pre = rtp->pre;
	btp = rtp->bplan_ptr;
	rt = btp->rates_ptr;

    if( rt != NULL) {
		p=0;
		while((rt[p].tariff_id) > 0)  {
			if(rt_data_q_chk_rate_periods(rt[p].start_period,rt[p].end_period,pre->ts) == 0) {
				p++;
				continue;
			}
	
			if(!strcmp(rt[p].prefix,"&")) goto without_prefix;
			
			len = strlen(rt[p].prefix);
			
			if(strncmp(pre->cld,rt[p].prefix,len) == 0) {
				without_prefix:
				pre->tariff_id = rt[p].tariff_id;
				pre->rate_id = rt[p].id;
				pre->prefix_id = rt[p].prefix_id;
				//pre->free_billsec_id = rt[p].free_billsec_id;
				//chk_tr_opt(dbp,pre);
				if(pre->tariff_id) {
					rt_data_q_tariff_sql(dbp,rtp);
					break;
				}
			}
			p++;
		}    
    }    
}

int rt_data_q_rate(db_t *dbp,racc_t *rtp)
{
	int ret;
	
	if(rtp == NULL) return -999;
	
	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	if(dbp->t == sql) {
		ret = rt_data_q_rate_sql(dbp,rtp);
		if(ret == 0) rt_data_q_prefix_sql(dbp,rtp);
	} else if(dbp->t == nosql) {
		
	} else return DB_ERR_TYPE_UNK;
	
	return ret;
}

//void rt_racc_nosql_query(db_t *dbp,rating *pre,int rt_mode,char *racc)
void rt_data_q_racc_nosql(db_t *dbp,racc_t *rtp)
{
	int ret;
	const char *hdr;
	char *cond;
	char buf[DB_BUF_LEN];

	rt_json_t rt_ptr;
	rating_t *pre;
	db_nosql_result_t *result;

	pre = rtp->pre;

	switch(rtp->rtm) {
		case rt_mode_clg:
			hdr = RT_JSON_HDR_PATTERN_RACC_CLG;
			cond = pre->clg;
			break;
		case rt_mode_acode:
			hdr = RT_JSON_HDR_PATTERN_RACC_ACODE;
			cond = pre->account_code;
			break;
		case rt_mode_srcc:
			hdr = RT_JSON_HDR_PATTERN_RACC_SRCC;
			cond = pre->src_context;
			break;
		case rt_mode_dstc:
			hdr = RT_JSON_HDR_PATTERN_RACC_DSTC;
			cond = pre->dst_context;
			break;
		case rt_mode_srctg:
			hdr = RT_JSON_HDR_PATTERN_RACC_SRCTG;
			cond = pre->src_tgroup;
			break;
		case rt_mode_dsttg: 
			hdr = RT_JSON_HDR_PATTERN_RACC_DSTTG;
			cond = pre->dst_tgroup;
			break;
		default:
			return;
	};

	sprintf(buf,"get %s%s",hdr,cond);
	
	ret = db_get(dbp,buf);
	if(ret == DB_OK) {
		result = (db_nosql_result_t *)dbp->conn->result;
		
		if(result->len > 0) {
			//rtp = rt_data_racc_init();
			
			rt_ptr.msg = result->str;
			//rt_ptr.msg = str_ext_clear_str(result->str,'\'');
			
			ret = rt_json_racc_get(&rt_ptr,rtp);
			if(ret < 0) {
				rt_data_racc_free(rtp);
				rtp = NULL;
			}
		}
	}
		
	db_free_result(dbp);
		
	db_nosql_result_free((db_nosql_result_t *)dbp->conn->result);
	dbp->conn->result = NULL;
}



void rt_data_q_bacc_nosql(db_t *dbp,racc_t *rtp)
{
	int ret;
	char buf[DB_BUF_LEN];
		
	rt_json_t rt_ptr;
	db_nosql_result_t *result;
		
	sprintf(buf,"get %s%s",RT_JSON_HDR_PATTERN_BACC,rtp->bacc_ptr->username);
	
	ret = db_get(dbp,buf);
	if(ret == DB_OK) {
		result = (db_nosql_result_t *)dbp->conn->result;
		
		if(result->len > 0) {			
			rt_ptr.msg = result->str;
			
			ret = rt_json_bacc_get(&rt_ptr,rtp->bacc_ptr);
		}
	}
		
	db_free_result(dbp);
		
	db_nosql_result_free((db_nosql_result_t *)dbp->conn->result);
	dbp->conn->result = NULL;
}

void rt_data_q_bplan_nosql(db_t *dbp,racc_t *rtp)
{
	int ret;
	char buf[DB_BUF_LEN];
		
	rt_json_t rt_ptr;
	db_nosql_result_t *result;
		
	sprintf(buf,"get %s%s",RT_JSON_HDR_PATTERN_BPLAN,rtp->bplan_ptr->bplan_name);
	
	ret = db_get(dbp,buf);
	if(ret == DB_OK) {
		result = (db_nosql_result_t *)dbp->conn->result;
		
		if(result->len > 0) {			
			rt_ptr.msg = result->str;
			
			ret = rt_json_bplan_get(&rt_ptr,rtp->bplan_ptr);
		}
	}
		
	db_free_result(dbp);
		
	db_nosql_result_free((db_nosql_result_t *)dbp->conn->result);
	dbp->conn->result = NULL;	
}

racc_t *rt_data_q_racc(db_t *dbp,rating_t *pre)
{
	int ret;
		
	racc_t *rtp;
	
	if(dbp == NULL) return NULL;
	if(pre == NULL) return NULL;
	
	rtp = rt_data_racc_init();
	
	if(rtp == NULL) return NULL;
	
	rtp->pre = pre;
	rtp->rtm = pre->rating_mode_id;
	rtp->bacc_ptr->cdr_server_id = pre->cdr_server_id;
	
	if(dbp->t == sql) {
		ret = rt_data_q_racc_sql(dbp,rtp);
		
		if(ret < 0) goto error;
		else goto success;
	} else if(dbp->t == nosql) {
		rt_data_q_racc_nosql(dbp,rtp);
		rt_data_q_bacc_nosql(dbp,rtp);
		rt_data_q_bplan_nosql(dbp,rtp);
		
		//rt_racc_nosql_query(dbp,pre,rt_mode_clg,pre->clg);
		//rt_bacc_nosql_query(dbp,pre);
		//rt_bplan_nosql_query(dbp,pre);
		
		// temp patch ???!!!
		//pre->bacc = 1;
		//pre->bplan = 1; 
	}
	
error:
	rt_data_racc_free(rtp);
	return NULL;
	
success:
	return rtp;
}
