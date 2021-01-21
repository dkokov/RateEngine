#include <unistd.h>
#include <sys/time.h>

#include "db_pgsql.h"
#include "../misc/globals.h"

int db_pgsql_conn_check(PGconn *conn,int log,char *func_name)
{
	int n = 0;
	int ret = 0;
	
	top:
	PQconsumeInput(conn);

	switch(PQstatus(conn)) {
		case CONNECTION_OK:
			if(log_debug_level >= LOG_LEVEL_TIME_DEBUG) {
				LOG(func_name,"Connection status of the PGSQL server is OK!");
			}
				
			ret = 1;
			
			break;
		default:
			if(log_debug_level >= LOG_LEVEL_DEBUG) {
				LOG(func_name,"Connection status of the PGSQL server is NOT OK!");
			}
			
			PQreset(conn);
			sleep(mcfg->int_retries);
			
			n++;
			
			if(n < mcfg->num_retries) goto top;
			
			LOG(func_name,"Connection max retries: %d (dbhost: %s,dbname: %s,dbuser: %s)",
			mcfg->num_retries,PQhost(conn),PQdb(conn),PQuser(conn));
			
			ret = 0;
	}
	
	return ret;
}

PGconn *db_pgsql_conn(char *db)
{
	PGconn *conn;
	conn = PQconnectdb(db);
	
	switch(PQstatus(conn)) {
		case CONNECTION_OK:
				break;
		default:
				conn = NULL;
	}
	
	return conn;
}

int db_pgsql_status_query(PGresult *res)
{	
	ExecStatusType res_check;
	
	res_check = PQresultStatus(res);
	
	if(res_check == PGRES_TUPLES_OK) return 0;
	else if(res_check == PGRES_COMMAND_OK) return 0;
	else return 1;
}

PGresult *db_pgsql_exec(PGconn *conn,char *sql)
{
	int status;
	PGresult *res;
	struct timeval tim;
	double t1,t2;
	
	res = NULL;
	
	if(log_debug_level == LOG_LEVEL_TIME_DEBUG) {
		gettimeofday(&tim, NULL);
		t1 = tim.tv_sec+(tim.tv_usec/1000000.0);
	}
	
	status = db_pgsql_conn_check(conn,config.log,"db_pgsql_exec()");
	if(status == 0) return res;
	
	res = PQexec(conn,sql);
	
	if(db_pgsql_status_query(res)) {
		LOG("db_pgsql_exec()","query '%s',\nerror: %s",sql,PQerrorMessage(conn));
	} else {
		if(log_debug_level == LOG_LEVEL_TIME_DEBUG) {
			gettimeofday(&tim, NULL);
			t2 = tim.tv_sec+(tim.tv_usec/1000000.0);
			
			LOG("db_pgsql_exec()","query '%s',\ntime exec: %f",sql,(t2-t1));
		}
	}
	
	return res;
}

int *db_pgsql_fnum(PGresult *res,char **col)
{
	int i;
	int number;
	int *fnum;
	
	fnum = 0;
	
	i=0;
	while((strcmp(col[i],""))) i++;
	
	if(i == 0) return 0;
	else number = i;
	
	fnum = (int *)malloc(sizeof(int)*(number));
	if(fnum == 0) return 0;
	
	for(i=0;i < number;i++) fnum[i] = PQfnumber(res,col[i]);
	
	return fnum;
}

int *db_pgsql_fnum_v2(PGresult *res,db_table_element_t *cols,int cols_num)
{
	int i;
	int *fnum;
		
	if(cols_num == 0) return 0;
	
	fnum = (int *)malloc(sizeof(int)*(cols_num));
	if(fnum == 0) return NULL;
	
	for(i=0;i < cols_num;i++) fnum[i] = PQfnumber(res,cols[i].col_name);
	
	return fnum;
}

void db_pgsql_fnum_free(int *fnum)
{
	free(fnum);
}

void db_pgsql_res_clear(PGresult *res)
{
	PQclear(res);
}
