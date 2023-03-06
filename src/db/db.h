#ifndef DB_H
#define DB_H

#define DB_BUF_LEN 2048 

#define SQL_BUF_LEN DB_BUF_LEN // ??? da se mahne

#define COL_NAME_LEN 128
#define ROW_LEN 255

/* **** DB interface ERRORS: **** */

/* DB interface struct pointer is NULL */
#define DB_ERR_DBP_NUL       -1

/* DB connection struct pointer is NULL */
#define DB_ERR_CONN_NUL      -2

/* DB connection is not ready/is not OK */
#define DB_ERR_CONN_NOOK     -3

/* DB connection struct,a pointer 'rconn'(engine reconnect) is NULL */
#define DB_ERR_RCONN_NUL     -4

/* DB reconnecting is NOOK - unsuccessful */
#define DB_ERR_RCONN_NOOK    -5

/* DB ENGINE bind api struct is NULL */
#define DB_ERR_ENG_NUL       -6

/* DB ENGINE NAME is empty - the string is NULL(empty) */
#define DB_ERR_ENGNAME_NUL   -7

/* DB ENGINE binding is not successful (cannot find module or api bind function) */
#define DB_ERR_ENGBIND_NUL   -8

/* DB SQL struct pointer is NULL */
#define DB_ERR_SQL_NUL       -9

/* DB SQL - returned result is NULL or don't have value */
#define DB_ERR_SQLRES_NUL   -10

/* DB SQL result columns struct pointer is NULL */
#define DB_ERR_SQLCOLS_NUL  -11

/* DB SQL exec is NOOK - cannot exec or exec is returned with error */
#define DB_ERR_SQLEXEC_NOOK -12

/* DB NOSQL struct pointer is NULL */
#define DB_ERR_NOSQL_NUL    -13

/* DB interaface type is UNK/isn't 'sql' or 'nosql' */
#define DB_ERR_TYPE_UNK     -14

/* DB ENGINE API is UNK */
#define DB_ERR_FUNC_UNK     -15

/* DB ENGINE defined API pointer is NULL */
#define DB_ERR_FUNC_NUL     -16

/* DB connection struct,a pointer 'res' is NULL */
#define DB_ERR_RESULT_NUL   -17

/* DB NOSQL - returned result is NULL or don't have value */
#define DB_ERR_NOSQL_RES_NUL -18

/* import external error from the binding DB engine */
#define DB_ERR_EXT_ERR      -99

/* DB returned value for OK/success  */
#define DB_OK 0

#define DB_SQL_INSERT_STR "insert"
#define DB_SQL_UPDATE_STR "update"
#define DB_SQL_SELECT_STR "select"

#define DB_NOSQL_SET_STR "set"
#define DB_NOSQL_GET_STR "get"
#define DB_NOSQL_OK_STR  "OK"

/* DB interface type: */
typedef enum {
	sql = 1,
	nosql = 2
} db_type_t;

/* DB connection struct */
typedef struct db_conn {
	char enginename[32];
	
	char hostname[255];
	char dbname[255];
	char username[255];
	char password[255];
	unsigned short port;
	unsigned short timeout;

	void *res;	
	void *rconn;
	void *result;
} db_conn_t;

/* DB SQL APIs (function pointers): */
typedef int  (*db_sql_connect) (db_conn_t *conn);
typedef int  (*db_sql_query)   (db_conn_t *conn,char *query);
typedef int  (*db_sql_fetch_array) (db_conn_t *conn);
typedef int  (*db_sql_status)  (db_conn_t *conn);
typedef void (*db_sql_close)   (db_conn_t *conn);
typedef int  (*db_sql_free_result) (db_conn_t *conn);

/* DB SQL APIs struct */
typedef struct db_sql {
	db_sql_connect     connect;
	db_sql_query       query;
	db_sql_fetch_array fetch;
	db_sql_status      status;
	db_sql_close       close;
	db_sql_free_result free_result;
} db_sql_t;

/* DB NOSQL APIs (function pointers): */
typedef int (*db_nosql_connect) (db_conn_t *conn);
typedef int (*db_nosql_command) (db_conn_t *conn,char *command);
typedef int (*db_nosql_set)     (db_conn_t *conn,char *key,char *value);
typedef int (*db_nosql_get)     (db_conn_t *conn,char *key);
typedef int (*db_nosql_status)  (db_conn_t *conn);
typedef int (*db_nosql_close)   (db_conn_t *conn);
typedef int (*db_nosql_free_reply) (db_conn_t *conn);

/* DB NOSQL APIs struct */
typedef struct db_nosql {
	db_nosql_connect     connect;
	db_nosql_command     command;
	db_nosql_set         set;
	db_nosql_get         get;
	db_nosql_status      status;
	db_nosql_close       close;
	db_nosql_free_reply  free_reply;

} db_nosql_t;

/* DB interface struct */
typedef struct db {
	db_type_t t;
	
	union {
		db_sql_t   *sql;
		db_nosql_t *nosql;
	} u;
	
	db_conn_t *conn;	
} db_t;

typedef struct db_sql_row {
	char row[ROW_LEN];
} db_sql_row_t;

typedef struct db_sql_col_name {
	char col_name[COL_NAME_LEN];
	
	db_sql_row_t *rows_list;
} db_sql_col_name_t;

typedef struct db_sql_result {
	int cols;
	int rows;
	
	db_sql_col_name_t *cols_list;
} db_sql_result_t;

typedef struct db_nosql_pair {
	char *key;
	char *value;
} db_nosql_pair_t;

typedef struct db_nosql_result {
	unsigned int len;
	char *str;

	unsigned long elements;
	db_nosql_pair_t *arr;
} db_nosql_result_t;

/* DB interface functions - declarations: */
db_t *db_init(void);
int  db_init_t(db_t *ptr);
void db_free(db_t *ptr);
int  db_engine_bind(db_t *ptr);
db_conn_t *db_conn_init(char *engine,char *host,char *dbname,unsigned short port,char *usr,char *pass,unsigned short timeout);
void db_conn_free(db_conn_t *conn);
int  db_sql_result_init(db_sql_result_t *ptr);
void db_sql_result_free(db_sql_result_t *ptr);
int  db_connect(db_t *ptr);
int  db_query(db_t *ptr,char *query,int clear);
int  db_select(db_t *ptr,char *query);
int  db_fetch(db_t *ptr);
int  db_free_result(db_t *ptr);
int  db_insert(db_t *ptr,char *query);
int  db_command(db_t *ptr,char *command);
int  db_set(db_t *ptr,char *key,char *value);
int  db_get(db_t *ptr,char *key);
int  db_update(db_t *ptr,char *query);
int  db_close(db_t *ptr);
void db_error(int error);
void db_error_put(int error,char *ext_err);
int  db_status(db_t *ptr);
void db_test(void);
db_nosql_result_t *db_nosql_result_init(int n);
void db_nosql_result_free(db_nosql_result_t *ptr);

int db_command_2(db_t *ptr,char *str,...);

#endif
