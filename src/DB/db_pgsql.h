#ifndef DB_PGSQL_H
#define DB_PGSQL_H

#include <libpq-fe.h>

#define SQL_BUF_LEN 1024

#define PGSQL_NUM_RETRIES 5
#define PGSQL_INT_RETRIES 1

typedef struct db_table_element {
	
	char col_name[256];
	
}db_table_element_t;

PGresult *db_pgsql_exec(PGconn *conn,char *sql_query);
int *db_pgsql_fnum(PGresult *res,char **col);
int *db_pgsql_fnum_v2(PGresult *res,db_table_element_t *cols,int cols_num);
void db_pgsql_fnum_free(int *fnum);
void db_pgsql_res_clear(PGresult *res);
PGconn *db_pgsql_conn(char *db);
int db_pgsql_conn_check(PGconn *conn,int log,char *func_name);
int db_pgsql_status_query(PGresult *res);

#endif
