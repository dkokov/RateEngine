#include "../misc/globals.h"

#include "balance.h"

unsigned short bal_create_tmp_bal(PGconn *conn)
{
	PGresult *res;

	res = db_pgsql_exec(conn,SQL_TMP_BAL);

    if(res != NULL) { 
		PQclear(res);
		LOG("bal_create_tmp_tbl()","The 'tmp_bal' is created successful!");
		
		return 0;
	}
	
	return 1;
}

/* Get balance ID and amount(sum) */
void bal_get_balance(PGconn *conn,rating *pre,char *start,char *end)
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

/* Get balance ID from the table 'tmp_bal' */
void bal_get_balance_id(PGconn *conn,rating *pre,char *start,char *end)
{
    PGresult *res;

    char str[512];
    int i,num;
    int fnum[1];
    
    num = 0;
    
    sprintf(str,"select id from tmp_bal "
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

/* insert a new row in the 'tmp_bal' table */
void bal_insert_balance(PGconn *conn,rating *pre,char *start,char *end)
{
    PGresult *res;
    char str[512];

    sprintf(str,
    "insert into tmp_bal (billing_account_id,start_date,end_date,active,amount,last_update) values (%d,'%s','%s','t',%f,'now()')",
    pre->bacc,start,end,pre->balanse);

	res = db_pgsql_exec(conn,str);

    if(res != NULL) PQclear(res);
    
    bal_get_balance_id(conn,pre,start,end);
}

/* Update balance in the 'tmp_bal' table */
void bal_update_balance(PGconn *conn,int bid,double bill)
{
    PGresult *res;
    char str[80];

    sprintf(str,"update tmp_bal set amount = %f,last_update = now() where id = %d",bill,bid);

	res = db_pgsql_exec(conn,str);
    
    if(res != NULL) PQclear(res);
}

/* Get 'rating.call_price' sum */
void bal_get_cp_sum(PGconn *conn,rating *pre,char *time1,char *time2)
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

void bal_exec(PGconn *conn,rating *pre,char *start,char *end)
{
	bal_get_cp_sum(conn,pre,start,end);

	bal_get_balance(conn,pre,start,end);

	if((pre->bal_id > 0)) {
		bal_update_balance(conn,pre->bal_id,pre->balanse);
	} else {
		bal_insert_balance(conn,pre,start,end);
		
		LOG("bal_exec()","create new balance,bacc %d,start %s,end %s,balance_id %d",
			pre->bacc,start,end,pre->bal_id);
	}
}
