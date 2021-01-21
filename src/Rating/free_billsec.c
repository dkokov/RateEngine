#include "../misc/globals.h"

void f_free_billsec_query(PGconn *conn,rating *pre)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[3];
    
    num = 0;
/*                   
	sprintf(str,"SELECT fr.free_billsec "
	            "from tariff as tr,free_billsec as fr "
	            "where tr.id = %d and "
	            "tr.id = fr.tariff_id and "
	            "tr.free_billsec_id = 0",
	            pre->tariff);
*/

	sprintf(str,"SELECT fr.free_billsec "
	            "from tariff as tr,free_billsec as fr "
	            "where tr.id = %d and "
	            "tr.free_billsec_id = fr.id",
	            pre->tariff);
	            
	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"free_billsec");

			for (i = 0; i < num; i++) {
				pre->free_billsec_limit = atoi(PQgetvalue(res,i,fnum[0]));
			}
		}

		PQclear(res);
	}
}

void f_free_billsec_query_2(PGconn *conn,rating *pre)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[3];
    
    num = 0;
/*
    sprintf(str,"SELECT fr.free_billsec "
                "from tariff as tr,free_billsec as fr "
                "where tr.id = %d and "
                "tr.free_billsec_id = fr.id and "
                "fr.tariff_id = 0",
                pre->tariff);
*/

    sprintf(str,"SELECT fr.free_billsec "
                "from tariff as tr,free_billsec as fr "
                "where tr.id = %d and "
                "tr.free_billsec_id = fr.id",
                pre->tariff);
                
	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"free_billsec");

			for (i = 0; i < num; i++) {
				pre->free_billsec_limit = atoi(PQgetvalue(res,i,fnum[0]));
			}
		}

		PQclear(res);
	}
}

void f_free_billsec_bal_query(PGconn *conn,rating *pre)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[3];
    
    num = 0;
    
//    sprintf(str,"select free_billsec,id from free_billsec_balance where tariff_id = %d and balance_id = %d",
//            pre->tariff,pre->bal_id);

    sprintf(str,"select free_billsec,id from free_billsec_balance where free_billsec_id = %d and balance_id = %d",
            pre->free_billsec_id,pre->bal_id);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"free_billsec");
			fnum[1] = PQfnumber(res,"id");

			for(i = 0; i < num; i++) {
				pre->free_billsec = atoi(PQgetvalue(res,i,fnum[0]));
				pre->free_id = atoi(PQgetvalue(res,i,fnum[1]));
			}
		}

		PQclear(res);
	}
}

void f_free_billsec_bal_query_2(PGconn *conn,rating *pre)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[3];
    
    num = 0;

/*
    sprintf(str,"select sum(free_billsec) as sum "
                "from free_billsec_balance " 
                "where balance_id = %d and "
                "tariff_id = %d ",
                pre->bal_id,pre->tariff);
*/
    sprintf(str,"select sum(free_billsec) as sum "
                "from free_billsec_balance " 
                "where balance_id = %d and "
                "free_billsec_id = %d ",
                pre->bal_id,pre->free_billsec_id);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"sum");
		
			for(i = 0; i < num; i++) {
				pre->free_billsec_sum = atoi(PQgetvalue(res,i,fnum[0]));
			}
		}
    
		PQclear(res);
	}
}

void f_free_bal_id_query(PGconn *conn,rating *pre)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[3];
    
    num = 0;
    
//    sprintf(str,"select id from free_billsec_balance where balance_id = %d and tariff_id = %d",pre->bal_id,pre->tariff);
    sprintf(str,"select id from free_billsec_balance where balance_id = %d and free_billsec_id = %d",pre->bal_id,pre->free_billsec_id);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		num = PQntuples(res);

		if(num) {
			fnum[0] = PQfnumber(res,"id");
		
			for(i = 0; i < num; i++) {
				pre->free_id = atoi(PQgetvalue(res,i,fnum[0]));
			}
		}
	
		PQclear(res);
	}
}

void create_free_billsec_balance_pgsql(PGconn *conn,rating *pre)
{
    PGresult *res;
    char str[512];

//    sprintf(str,
//    "insert into free_billsec_balance (balance_id,tariff_id,free_billsec) values (%d,%d,%d)",
//    pre->bal_id,pre->tariff,pre->free_billsec);

    sprintf(str,
    "insert into free_billsec_balance (balance_id,free_billsec_id,free_billsec) values (%d,%d,%d)",
    pre->bal_id,pre->free_billsec_id,pre->free_billsec);
  
	res = db_pgsql_exec(conn,str);
    
    if(res != NULL) PQclear(res);
    
    f_free_bal_id_query(conn,pre);
}

void update_free_billsec_balance(PGconn *conn,rating *pre)
{
    PGresult *res;
    char str[512];

	if((pre->bal_id > 0)&&(pre->tariff > 0)) {
//		sprintf(str,
//			"update free_billsec_balance set free_billsec = %d,last_update = now() where balance_id = %d and tariff_id = %d",
//			pre->free_billsec,pre->bal_id,pre->tariff);

		sprintf(str,
			"update free_billsec_balance set free_billsec = %d,last_update = now() where balance_id = %d and free_billsec_id = %d",
			pre->free_billsec,pre->bal_id,pre->free_billsec_id); 
   
		res = db_pgsql_exec(conn,str);

		if(res != NULL) PQclear(res);
	}
}

/* Free SMSes  */
void free_billsec_sms_balance(PGconn *conn,rating *pre,char *time1,char *time2)
{
    int free;
    PGresult *res;

    char str[512];
    int n,i;
    
    free = 0;
    
    sprintf(str,"select count(*) as cnt"
                " from rating rt "
                "where rt.billing_account_id = %d and rt.call_ts >= '%s' "
                "and rt.call_ts <= '%s' and rt.call_price < 0 "
                "and rt.free_billsec_id = %d",
                pre->bacc,time1,time2,pre->free_billsec_id);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		n = PQfnumber(res,"cnt");

		for (i = 0; i < PQntuples(res); i++) {
			free = atoi(PQgetvalue(res, i, n));
		}
		
		PQclear(res);
	}

    pre->free_billsec = free;
}

/* 
    This function make balance of the free_billsec from the rating
    with sql query into 'rating' table for defined time period(from date to date)!
    The return value is saved in the 'rating' struct (pre).
*/
void free_billsec_balance(PGconn *conn,rating *pre,char *time1,char *time2)
{
    int free;
    PGresult *res;

    char str[512];
    int n,i;
    
    free = 0;
    
    sprintf(str,"select sum(rt.call_billsec)"
                " from rating rt,rate as rr "
                "where rt.billing_account_id = %d and rt.call_ts >= '%s' "
                "and rt.call_ts <= '%s' and rt.call_price < 0 and "
                "rr.id = rt.rate_id and rr.tariff_id = %d",
                pre->bacc,time1,time2,pre->tariff);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		n = PQfnumber(res,"sum");

		for (i = 0; i < PQntuples(res); i++) {
			free = atoi(PQgetvalue(res, i, n));
		}
		
		PQclear(res);
	}

    pre->free_billsec = free;
}

void free_billsec_balance_v2(PGconn *conn,rating *pre,char *time1,char *time2)
{
    int free;
    PGresult *res;

    char str[512];
    int n,i;
    
    free = 0;
    
    sprintf(str,"select sum(rt.call_billsec)"
                " from rating rt "
                "where rt.billing_account_id = %d and rt.call_ts >= '%s' "
                "and rt.call_ts <= '%s' and rt.call_price < 0 "
                "and rt.free_billsec_id = %d",
                pre->bacc,time1,time2,pre->free_billsec_id);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		n = PQfnumber(res,"sum");

		for (i = 0; i < PQntuples(res); i++) {
			free = atoi(PQgetvalue(res, i, n));
		}
		
		PQclear(res);
	}

    pre->free_billsec = free;
}

void free_billsec_exec(PGconn *conn,rating *pre,char *start,char *end)
{
	free_billsec_balance_v2(conn,pre,start,end); // free_billsec
    
    f_free_bal_id_query(conn,pre);
    
    if(pre->free_id > 0) update_free_billsec_balance(conn,pre);
	else create_free_billsec_balance_pgsql(conn,pre);
    
    f_free_billsec_bal_query_2(conn,pre); // ???? free_billsec_sum
}
