#ifndef CDR_STORAGE_H
#define CDR_STORAGE_H

#define CDR_COL_LEN 32

/* storage type */
typedef enum cdr_storage_type {
	pgsql,
	mysql,
	oracle
}cdr_storage_type_t;

typedef struct cdr_storage_col {
	unsigned short col_id;
	char col_name[CDR_COL_LEN];
	
	void (*func)(cdr_t *,char *);
}cdr_storage_col_t;

typedef enum sql_col_where_type {
	ts,
	epoch
}sql_col_where_type_t;

typedef struct cdr_storage_profile {
	
	char dbhost[128];
	char dbuser[128];
	char dbpass[128];
	char dbname[128];

	PGconn *pgsql;
	//mysql
	//oracle

	int ts;
	int chk_ts;
	char cdr_sched_ts[64];

	char *sql_query;	
	char cdr_table[128];
	
	char sql_col_where[128];
	
	sql_col_where_type_t sql_col_t;
	
	char sql_where_const[256];
	
	cdr_storage_type_t t;

	cdr_storage_col_t *cols;
	unsigned short cols_num;
	
	PGconn *conn;
	filter *filters;
	int cdr_server_id;
	int cdr_rec_type_id;
	
}cdr_storage_profile_t;

PGconn *cdr_storage_pgsql_conn(cdr_storage_profile_t *profile);
cdr_storage_col_t *cdr_storage_col_init(int num);
cdr_storage_profile_t *cdr_storage_profile_init(void);
void cdr_storage_col_put(cdr_storage_col_t *cols,cdr_table_t *tbl,char *col_name,int col_id);
void cdr_storage_sql_query_parser(cdr_storage_profile_t *profile);
PGresult *cdr_storage_pgsql_cdrs_exec(cdr_storage_profile_t *profile);
int cdr_storage_pgsql_cols_num(PGresult *res);
int *cdr_storage_pgsql_cols_mem(int num);
int *cdr_storage_pgsql_cols_parser(PGresult *res,cdr_storage_profile_t *profile);

int cdr_storage_get_pgsql_cdrs(cdr_storage_profile_t *profile);
void cdr_storage_reader(cdr_storage_profile_t *profile);

#endif
