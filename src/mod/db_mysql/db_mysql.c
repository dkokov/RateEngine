/* 
 * https://dev.mysql.com/doc/refman/8.0/en/c-api-function-overview.html
 * https://dev.mysql.com/doc/connector-c/en/
 * 
 * */

#include <string.h>
#include <stddef.h>

#include <mysql.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

int db_mysql_connect(db_conn_t *conn)
{
	MYSQL *myconn;

	if(conn == NULL) return DB_ERR_CONN_NUL;
	
	myconn = mysql_init(NULL);
	
	mysql_real_connect(myconn,conn->hostname,conn->username,conn->password,conn->dbname,conn->port, NULL, 0);

	if(myconn == NULL) return DB_ERR_CONN_NOOK;

	conn->rconn = (void *)myconn;

	return DB_OK;
}

int db_mysql_query(db_conn_t *conn,char *query)
{
	MYSQL *myconn;
	MYSQL_RES *res;
	
	if(conn == NULL) return DB_ERR_CONN_NUL;
	if(conn->rconn == NULL) return DB_ERR_RCONN_NUL;
	
	myconn = (MYSQL *)conn->rconn;
		
	if(mysql_query(myconn,query)) {
		LOG("db_mysql_query()","query '%s', MySQL server error:'%s'",query,mysql_error(myconn));
		return DB_ERR_SQLEXEC_NOOK;
	}
	
	res = mysql_store_result(myconn);
	
	if(res != NULL) conn->res = (void *)res; 
	else return DB_ERR_SQLRES_NUL;
	
	return DB_OK;
}

int db_mysql_fetch_array(db_conn_t *conn)
{
	int i,c;
	
	MYSQL_ROW row;
	MYSQL_RES *res;
	MYSQL_FIELD *fields;
	
	db_sql_result_t *sql_res;
	
	if(conn == NULL) return DB_ERR_CONN_NUL;
	if(conn->rconn == NULL) return DB_ERR_RCONN_NUL;	
	if(conn->res == NULL) return DB_ERR_SQLRES_NUL;
	
	res = (MYSQL_RES *)conn->res;
	
	sql_res = mem_alloc(sizeof(db_sql_result_t));
	
	if(sql_res == NULL) return DB_ERR_RESULT_NUL;
	
	sql_res->rows = mysql_num_rows(res);
	sql_res->cols = mysql_num_fields(res);

	if(db_sql_result_init(sql_res) < 0) return DB_ERR_RESULT_NUL;
	 
	fields = mysql_fetch_fields(res);
	for(i=0;i<sql_res->cols;i++) strcpy(sql_res->cols_list[i].col_name,fields[i].name);
		
	c = 0;
	while((row = mysql_fetch_row(res))) {
		for(i=0;i<sql_res->cols;i++) strcpy(sql_res->cols_list[i].rows_list[c].row,row[i]);

		c++;
	}

	conn->result = (void *)sql_res;

	return DB_OK;
}

void db_mysql_close(db_conn_t *conn)
{
	MYSQL *myconn;
	
	myconn = (MYSQL *)conn->rconn;
	
	if(myconn != NULL) mysql_close(myconn);
}

int db_mysql_free_result(db_conn_t *conn)
{
	MYSQL_RES *result;
	
	if(conn == NULL) return DB_ERR_CONN_NUL;
	
	if(conn->res == NULL) return DB_ERR_SQLRES_NUL;
	
	result = (MYSQL_RES *)conn->res;		
	mysql_free_result(result);
	
	conn->res = NULL;

	return DB_OK;
}

int db_mysql_bind_api(db_t *ptr)
{
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	
	ptr->t = sql;
	
	ret = db_init_t(ptr);

	if(ret < 0) return ret;
	
	ptr->u.sql->connect     = db_mysql_connect;
	ptr->u.sql->query       = db_mysql_query;
	ptr->u.sql->fetch       = db_mysql_fetch_array;
	ptr->u.sql->status      = NULL;
	ptr->u.sql->close       = db_mysql_close;
	ptr->u.sql->free_result = db_mysql_free_result;
	
	return DB_OK;	
}
