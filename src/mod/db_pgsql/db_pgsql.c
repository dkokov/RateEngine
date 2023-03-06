#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include <libpq-fe.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

int db_pgsql_conn_check(PGconn *conn)
{
	int n = 0;
	int ret = 0;

	top:
	PQconsumeInput(conn);

	switch(PQstatus(conn)) {
		case CONNECTION_OK:
			DBG("db_pgsql_conn_check()","Connection status of the PGSQL server is OK!");
			ret = DB_OK;
			break;
		default:
			LOG("db_pgsql_conn_check()","Connection status of the PGSQL server is NOT OK!");
			
			PQreset(conn);
			sleep(mcfg->int_retries);
			
			n++;
			
			if(n < mcfg->num_retries) goto top;

			LOG("db_pgsql_conn_check()","Connection max retries: %d (dbhost: %s,dbname: %s,dbuser: %s)",
			mcfg->num_retries,PQhost(conn),PQdb(conn),PQuser(conn));
			
			ret = DB_ERR_RCONN_NOOK;
	}
	
	return ret;
}

int db_pgsql_status_query(PGresult *res)
{	
	ExecStatusType res_check;
	
	res_check = PQresultStatus(res);
	
	if(res_check == PGRES_TUPLES_OK) return DB_OK;
	else if(res_check == PGRES_COMMAND_OK) return DB_OK;
	else return DB_ERR_SQLEXEC_NOOK;
}

PGresult *db_pgsql_exec(PGconn *conn,char *sql)
{
	PGresult *res;
    
	res = NULL;
	
	if(db_pgsql_conn_check(conn) == DB_OK) {
		res = PQexec(conn,sql);
	
		if(db_pgsql_status_query(res) == DB_ERR_SQLEXEC_NOOK) {
			LOG("db_pgsql_exec()","query '%s',\nerror: %s",sql,PQerrorMessage(conn));
		} else {
			DBG("db_pgsql_exec()","query '%s'",sql);
		}
	} else {
		LOG("db_pgsql_exec()","PGSQL , no connection !!!");
	}
    
    return res;
}

int db_pgsql_connect(db_conn_t *conn)
{
	char buf[6];
	PGconn *pgconn;

	if(conn == NULL) return DB_ERR_CONN_NUL;

	sprintf(buf,"%d",conn->port);
	
	pgconn = PQsetdbLogin(conn->hostname,buf,NULL,NULL,conn->dbname,conn->username,conn->password);

	switch(PQstatus(pgconn)) {
		case CONNECTION_OK:
				conn->rconn = (void *)pgconn;
				break;
		default:
				PQfinish(pgconn);
				return DB_ERR_CONN_NOOK;
	}

	return DB_OK;
}

int db_pgsql_query(db_conn_t *conn,char *query)
{
	PGconn *pgconn;
	PGresult *res;
	
	if(conn == NULL) return DB_ERR_CONN_NUL;
	
	pgconn = (PGconn *)conn->rconn;
	
	if(pgconn == NULL) return DB_ERR_RCONN_NUL;
	
	res = db_pgsql_exec(pgconn,query);
	
	if(res != NULL) conn->res = (void *)res; 
	else return DB_ERR_SQLRES_NUL;
	
	return DB_OK;
}

int db_pgsql_fetch_array(db_conn_t *conn)
{
	int i,c;
	
	PGresult *res;
	
	db_sql_result_t *sql_res;
	
	if(conn == NULL) return DB_ERR_CONN_NUL;
	
	if(conn->res == NULL) return DB_ERR_SQLRES_NUL;
	
	res = (PGresult *)conn->res;
	
	sql_res = mem_alloc(sizeof(db_sql_result_t));
	
	if(sql_res == NULL) return DB_ERR_RESULT_NUL;
	
	sql_res->rows = PQntuples(res);
	sql_res->cols = PQnfields(res);
	
	if(db_sql_result_init(sql_res) < 0) return DB_ERR_RESULT_NUL;
	
	for(i=0;i<sql_res->cols;i++) {
		strcpy(sql_res->cols_list[i].col_name,PQfname(res,i));

		for(c=0;c<sql_res->rows;c++) {
			strcpy(sql_res->cols_list[i].rows_list[c].row,PQgetvalue(res,c,i));
		}
	}

	conn->result = (void *)sql_res;

	return DB_OK;
}

int db_pgsql_status(db_conn_t *conn)
{
	PGconn *pgconn;
	
	if(conn == NULL) return DB_ERR_CONN_NUL;
	
	pgconn = (PGconn *)conn->rconn;
	
	if(pgconn == NULL) return DB_ERR_RCONN_NUL;
	
	return db_pgsql_conn_check(pgconn);
}

void db_pgsql_close(db_conn_t *conn)
{
	PGconn *pgconn;
	
	pgconn = (PGconn *)conn->rconn;
	
	if(pgconn != NULL) PQfinish(pgconn);
}

int db_pgsql_free_result(db_conn_t *conn)
{
	PGresult *res;
	
	if(conn == NULL) return DB_ERR_CONN_NUL;
	
	if(conn->res == NULL) return DB_ERR_SQLRES_NUL;
	
	res = (PGresult *)conn->res;		
	PQclear(res);
	
	conn->res = NULL;
	
	return DB_OK;
}

int db_pgsql_bind_api(db_t *ptr)
{
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	
	ptr->t = sql;
	
	ret = db_init_t(ptr);

	if(ret < 0) return ret;
	
	ptr->u.sql->connect     = db_pgsql_connect;
	ptr->u.sql->query       = db_pgsql_query;
	ptr->u.sql->fetch       = db_pgsql_fetch_array;
	ptr->u.sql->status      = db_pgsql_status;
	ptr->u.sql->close       = db_pgsql_close;
	ptr->u.sql->free_result = db_pgsql_free_result;
	
	return DB_OK;
}
