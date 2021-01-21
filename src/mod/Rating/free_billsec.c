#include "../../misc/globals.h"
#include "../../db/db.h"

#include "rating.h"
#include "free_billsec.h"

int f_free_billsec_query(db_t *dbp,rating_t *pre)
{
    char str[DB_BUF_LEN];
    
 	db_sql_result_t *result;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return -2;
		
		sprintf(str,"SELECT fr.free_billsec from tariff as tr,free_billsec as fr "
					"where tr.id = %d and tr.free_billsec_id = fr.id",pre->tariff_id);
	
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			
			if(result->rows == 1) pre->free_billsec_limit = atoi(result->cols_list[0].rows_list[0].row);
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -3;
	
	return 0;
}

int f_free_billsec_query_2(db_t *dbp,rating_t *pre)
{
    char str[SQL_BUF_LEN];
	
 	db_sql_result_t *result;

	if(dbp == NULL) return -1;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return -2;
		
		sprintf(str,"SELECT fr.free_billsec "
					"from tariff as tr,free_billsec as fr "
					"where tr.id = %d and "
					"tr.free_billsec_id = fr.id",
					pre->tariff_id);
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			
			if(result->rows == 1) pre->free_billsec_limit = atoi(result->cols_list[0].rows_list[0].row);
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -3;
	
	return 0;
}

int f_free_billsec_bal_query(db_t *dbp,rating_t *pre)
{	
    char str[SQL_BUF_LEN];
	
 	db_sql_result_t *result;

	if(dbp == NULL) return -1;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return -2;
		
		sprintf(str,"select free_billsec,id from free_billsec_balance where free_billsec_id = %d and balance_id = %d",
					 pre->free_billsec_id,pre->bal_id);		
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			
			if(result->rows == 1) {
				pre->free_billsec = atoi(result->cols_list[0].rows_list[0].row);
				pre->free_id = atoi(result->cols_list[1].rows_list[0].row);
			}
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -3;
	
	return 0;	
}

int f_free_billsec_bal_query_2(db_t *dbp,rating_t *pre)
{	
    char str[SQL_BUF_LEN];
	
 	db_sql_result_t *result;

	if(dbp == NULL) return -1;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return -2;
		
		sprintf(str,"select sum(free_billsec) as sum from free_billsec_balance " 
					"where balance_id = %d and free_billsec_id = %d ",pre->bal_id,pre->free_billsec_id);	
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			
			if(result->rows == 1) {
				pre->free_billsec_sum = atoi(result->cols_list[0].rows_list[0].row);
			}
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -3;
	
	return 0;	
}

int f_free_bal_id_query(db_t *dbp,rating_t *pre)
{	
    char str[SQL_BUF_LEN];
	
 	db_sql_result_t *result;

	if(dbp == NULL) return -1;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return -2;
		
		sprintf(str,"select id from free_billsec_balance where balance_id = %d and free_billsec_id = %d",pre->bal_id,pre->free_billsec_id);
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			
			if(result->rows == 1) {
				pre->free_id = atoi(result->cols_list[0].rows_list[0].row);
			}
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -3;
	
	return 0;	
}

int create_free_billsec_balance(db_t *dbp,rating_t *pre)
{
    char str[SQL_BUF_LEN];
    
	if(dbp == NULL) return -1;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return -2;

		sprintf(str,"insert into free_billsec_balance (balance_id,free_billsec_id,free_billsec) values (%d,%d,%d)",pre->bal_id,pre->free_billsec_id,pre->free_billsec);		
		
		db_insert(dbp,str);		
	} else return -3;
	
    return f_free_bal_id_query(dbp,pre);
}

int update_free_billsec_balance(db_t *dbp,rating_t *pre)
{
    char str[SQL_BUF_LEN];
    
	if(dbp == NULL) return -1;

	if((pre->bal_id > 0)&&(pre->tariff_id > 0)) {
		if(dbp->t == sql) {		
			if(dbp->u.sql == NULL) return -2;

			sprintf(str,"update free_billsec_balance set free_billsec = %d,last_update = now() where balance_id = %d and free_billsec_id = %d",
					 pre->free_billsec,pre->bal_id,pre->free_billsec_id); 	
		
			db_update(dbp,str);		
		} else return -3;
	}
	
	return 0;
}

/* Free SMSes  */
int free_billsec_sms_balance(db_t *dbp,racc_t *rtp,char *time1,char *time2)
{    
    char str[SQL_BUF_LEN];
	
	rating_t *pre;
 	db_sql_result_t *result;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	pre = rtp->pre;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return -2;
		
		sprintf(str,"select count(*) as cnt"
                " from rating rt "
                "where rt.billing_account_id = %d and rt.call_ts >= '%s' "
                "and rt.call_ts <= '%s' and rt.call_price < 0 "
                "and rt.free_billsec_id = %d",
                rtp->bacc_ptr->id,time1,time2,pre->free_billsec_id);		
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			
			if(result->rows == 1) {
				pre->free_billsec = atoi(result->cols_list[0].rows_list[0].row);
			}
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -3;
	
	return DB_OK;    
}

/* 
    This function make balance of the free_billsec from the rating
    with sql query into 'rating' table for defined time period(from date to date)!
    The return value is saved in the 'rating' struct (pre).
*/
int free_billsec_balance(db_t *dbp,racc_t *rtp,char *time1,char *time2)
{
    char str[DB_BUF_LEN];
    
	rating_t *pre;
 	db_sql_result_t *result;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	pre = rtp->pre;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return -2;
		
		sprintf(str,"select sum(rt.call_billsec)"
					" from rating rt,rate as rr "
					"where rt.billing_account_id = %d and rt.call_ts >= '%s' "
					"and rt.call_ts <= '%s' and rt.call_price < 0 and "
					"rr.id = rt.rate_id and rr.tariff_id = %d",
					rtp->bacc_ptr->id,time1,time2,pre->tariff_id);	
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			
			if(result->rows == 1) {
				pre->free_billsec = atoi(result->cols_list[0].rows_list[0].row);
			}
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -3;
	
	return DB_OK;    
}

int free_billsec_balance_v2(db_t *dbp,racc_t *rtp,char *time1,char *time2)
{
    char str[DB_BUF_LEN];
	
	rating_t *pre;
 	db_sql_result_t *result;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	
	pre = rtp->pre;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return -2;
		
		sprintf(str,"select sum(rt.call_billsec)"
					" from rating rt "
					"where rt.billing_account_id = %d and rt.call_ts >= '%s' "
					"and rt.call_ts <= '%s' and rt.call_price < 0 "
					"and rt.free_billsec_id = %d",
					rtp->bacc_ptr->id,time1,time2,pre->free_billsec_id);
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			
			if(result->rows == 1) {
				pre->free_billsec = atoi(result->cols_list[0].rows_list[0].row);
			}
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return -3;
	
	return DB_OK;
}

int free_billsec_exec(db_t *dbp,racc_t *rtp,char *start,char *end)
{
	rating_t *pre;
	
	pre = rtp->pre;
	
	if(free_billsec_balance_v2(dbp,rtp,start,end) < 0) return RE_ERROR;
    
    if(f_free_bal_id_query(dbp,rtp->pre) < 0) return RE_ERROR;
    
    if(pre->free_id > 0) { 
		if(update_free_billsec_balance(dbp,pre) < 0) return RE_ERROR;
	} else { 
		if(create_free_billsec_balance(dbp,pre) < 0) return RE_ERROR;
	}
    
    if(f_free_billsec_bal_query_2(dbp,pre) < 0) return RE_ERROR;

	return RE_SUCCESS;
}
