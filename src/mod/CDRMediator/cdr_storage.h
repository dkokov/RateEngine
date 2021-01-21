#ifndef CDR_STORAGE_H
#define CDR_STORAGE_H

#define CDR_COL_LEN 32

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
	/* local DB , copy pointer from main cfg ! */
	db_t *dbp;

	int ts;
	int chk_ts;
	char cdr_sched_ts[64];

	/* remote DB */
	db_t *rem_dbp;
	
    char dbtype[32];
    char dbhost[255];
    char dbname[64];
    char dbuser[64];
    char dbpass[64];
    int dbport;
    				
	char *sql_query;	
	char cdr_table[128];
	
	char sql_col_where[128];
	
	sql_col_where_type_t sql_col_t;
	
	char sql_where_const[256];
	
	cdr_storage_col_t *cols;
	unsigned short cols_num;
	
	filter *filters;
	int cdr_server_id;
	int cdr_rec_type_id;
	
	char profile_name[32]; 
} cdr_storage_profile_t;

cdr_storage_col_t *cdr_storage_col_init(int num);
cdr_storage_profile_t *cdr_storage_profile_init(void);
void cdr_storage_col_put(cdr_storage_col_t *cols,cdr_table_t *tbl,char *col_name,int col_id);
void cdr_storage_reader(cdr_storage_profile_t *profile);

#endif
