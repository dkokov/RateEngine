#ifndef DB_DUCKDB_H
#define DB_DUCKDB_H

#include "duckdb.h"

/*
 * Per-connection DuckDB handles, stored in db_conn_t.rconn.
 * DuckDB is embedded: one in-memory (or file) database + one connection.
 * The attach to an external store (e.g. PostgreSQL via postgres_scanner)
 * is issued by the caller as plain SQL through db_query().
 */
typedef struct db_duckdb_ctx {
	duckdb_database   db;
	duckdb_connection conn;
} db_duckdb_ctx_t;

/* engine API (bound into the db_sql_t vtable by db_duckdb_bind_api) */
int  db_duckdb_connect(db_conn_t *conn);
int  db_duckdb_query(db_conn_t *conn,char *query);
int  db_duckdb_fetch_array(db_conn_t *conn);
int  db_duckdb_status(db_conn_t *conn);
void db_duckdb_close(db_conn_t *conn);
int  db_duckdb_free_result(db_conn_t *conn);
int  db_duckdb_bind_api(db_t *ptr);

#endif
