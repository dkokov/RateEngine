#ifndef RT_DUCKDB_H
#define RT_DUCKDB_H

#include "../../db/db.h"
#include "../Rating/rt_data.h"

/* dimension-cache modes (memory vs speed) */
#define RT_DIMCACHE_NONE   0   /* all live (DuckDB views over pg.*) */
#define RT_DIMCACHE_STATIC 1   /* cache small config tables; account tables live */
#define RT_DIMCACHE_ALL    2   /* cache everything (default; fastest, most RAM) */

typedef struct rt_duckdb {
	/* DuckDB accessed through the db_duckdb engine (duckdb.so) */
	db_t *duck;

	/* PostgreSQL connection string for postgres_scanner */
	char pg_connstr[512];

	/* batch settings */
	int batch_limit;

	/* dimension cache mode (RT_DIMCACHE_*) */
	int cache_mode;
} rt_duckdb_t;

int rt_duckdb_init(rt_duckdb_t *ctx,const char *pg_host,const char *pg_dbname,
                   const char *pg_user,const char *pg_pass,int pg_port,int threads,int cache_mode);
void rt_duckdb_close(rt_duckdb_t *ctx);

/* (re)load the static dimension tables into local DuckDB tables */
int rt_duckdb_load_dims(rt_duckdb_t *ctx);

/* rate a batch of CDRs using DuckDB — returns number rated */
int rt_duckdb_rate_batch(rt_duckdb_t *ctx,db_t *pg_dbp,char leg,int limit);

#endif
