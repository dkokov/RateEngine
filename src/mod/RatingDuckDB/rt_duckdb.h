#ifndef RT_DUCKDB_H
#define RT_DUCKDB_H

#include "../../db/db.h"
#include "../Rating/rt_data.h"

typedef struct rt_duckdb {
	/* DuckDB accessed through the db_duckdb engine (duckdb.so) */
	db_t *duck;

	/* PostgreSQL connection string for postgres_scanner */
	char pg_connstr[512];

	/* batch settings */
	int batch_limit;
} rt_duckdb_t;

int rt_duckdb_init(rt_duckdb_t *ctx,const char *pg_host,const char *pg_dbname,
                   const char *pg_user,const char *pg_pass,int pg_port);
void rt_duckdb_close(rt_duckdb_t *ctx);

/* rate a batch of CDRs using DuckDB — returns number rated */
int rt_duckdb_rate_batch(rt_duckdb_t *ctx,db_t *pg_dbp,char leg,int limit);

#endif
