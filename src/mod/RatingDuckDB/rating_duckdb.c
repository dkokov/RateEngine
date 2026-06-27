#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>

#include "../mod.h"
#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"
#include "../../config/xml_cfg.h"

#include "../Rating/rt_bind_api.h"
#include "../CDRMediator/cdr_bind_api.h"

#include "rt_duckdb.h"

static rt_duckdb_t duckdb_ctx;
static db_t *pg_dbp;
static int rating_interval = 300;       /* idle poll (seconds) when queue empty */
static int wait_rating = 1500;          /* throttle (microseconds) between drain cycles; default mirrors /Rating WAIT_RATING */
static int batch_limit = 5000;
static int rating_threads = 0;   /* 0 = DuckDB default (all cores) */
static int cache_mode = RT_DIMCACHE_ALL;   /* dimension cache: all|static|none */
static char leg = 'a';
static char active = 'f';

int rt_init_duckdb(void);
int rt_free_duckdb(void);

mod_dep_t rt_mod_dep[] = {
	{"cdrm.so",0,1},
	{"duckdb.so",0,1},
	{"",0,0}
};

mod_t rt_mod_t = {
	.mod_name = "RatingDuckDB",
	.ver      = 1,
	.init     = NULL,
	.destroy  = NULL,
	.depends  = rt_mod_dep,
	.handle   = NULL,
	.next     = NULL
};

int rt_init_duckdb(void)
{
	int ret;

	/* create PostgreSQL connection for writing results */
	pg_dbp = db_init();
	pg_dbp->conn = db_conn_init(mcfg->dbtype,mcfg->dbhost,mcfg->dbname,mcfg->dbport,mcfg->dbuser,mcfg->dbpass,0);

	if(db_engine_bind(pg_dbp) < 0) {
		LOG("rt_init_duckdb()","pg db_engine_bind error");
		return -1;
	}

	if(db_connect(pg_dbp) < 0) {
		LOG("rt_init_duckdb()","pg db_connect error");
		return -1;
	}

	/* init DuckDB with PostgreSQL scanner */
	ret = rt_duckdb_init(&duckdb_ctx,mcfg->dbhost,mcfg->dbname,mcfg->dbuser,mcfg->dbpass,mcfg->dbport,rating_threads,cache_mode);
	if(ret < 0) {
		LOG("rt_init_duckdb()","DuckDB init failed: %d",ret);
		return -2;
	}

	return 0;
}

int rt_free_duckdb(void)
{
	rt_duckdb_close(&duckdb_ctx);

	if(pg_dbp != NULL) {
		db_close(pg_dbp);
		db_free(pg_dbp);
	}

	return 0;
}

void *RateEngine(void *dt)
{
	xml_node_t *node;
	xml_param_t *params;
	xmlDoc *doc;
	xmlNode *root;

	/* read config */
	doc = xml_cfg_doc(mcfg->cfg_filename);
	if(doc != NULL) {
		root = xml_cfg_root(doc);
		if(root != NULL) {
			node = xml_cfg_node_init();
			strcpy(node->node_name,"Rating");
			xml_cfg_params_get(root,node);

			params = node->params;
			while(params != NULL) {
				if(strcmp(params->name,"active") == 0) {
					if(strcmp(params->value,"yes") == 0) active = 't';
				}
				if(strcmp(params->name,"RatingInterval") == 0) rating_interval = atoi(params->value);
				if(strcmp(params->name,"WaitRatingInterval") == 0) wait_rating = atoi(params->value);
				if(strcmp(params->name,"BatchLimit") == 0) batch_limit = atoi(params->value);
				if(strcmp(params->name,"RatingThreads") == 0) rating_threads = atoi(params->value);
				if(strcmp(params->name,"CacheDimensions") == 0) {
					if(strcmp(params->value,"none") == 0)        cache_mode = RT_DIMCACHE_NONE;
					else if(strcmp(params->value,"static") == 0) cache_mode = RT_DIMCACHE_STATIC;
					else                                         cache_mode = RT_DIMCACHE_ALL;
				}
				if(strcmp(params->name,"leg") == 0) leg = params->value[0];
				params = params->next_param;
			}

			xml_cfg_params_free(node->params);
			mem_free(node);
		}
		xml_cfg_free_doc(doc);
	}

	if((mcfg->daemon_flag)&&(active == 'f')) goto _end;

	if(batch_limit <= 0) batch_limit = 5000;
	if(batch_limit > 50000) batch_limit = 50000;

	LOG("RateEngine","DuckDB rating module, batch_limit: %d, interval: %d",batch_limit,rating_interval);

	if(rt_init_duckdb() < 0) {
		LOG("RateEngine","rt_init_duckdb() failed");
		goto _end;
	}

	int dim_cycles = 0;   /* drive periodic dimension-cache refresh */

	loop:
	{
		struct timeval t1,t2;
		int processed = 0;   /* CDRs consumed from the window this cycle */
		int rated = 0;       /* of those, how many produced a rating row */

		LOG("RateEngine","DuckDB rating cycle started ...");

		gettimeofday(&t1,NULL);

		if(leg == '\0') {
			int ra = 0,rb = 0,pa,pb;
			pa = rt_duckdb_rate_batch(&duckdb_ctx,pg_dbp,'a',batch_limit,&ra);
			pb = rt_duckdb_rate_batch(&duckdb_ctx,pg_dbp,'b',batch_limit,&rb);
			processed = (pa > 0 ? pa : 0) + (pb > 0 ? pb : 0);
			rated = ra + rb;
		} else {
			processed = rt_duckdb_rate_batch(&duckdb_ctx,pg_dbp,leg,batch_limit,&rated);
			if(processed < 0) processed = 0;
		}

		gettimeofday(&t2,NULL);

		double elapsed = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;

		if(processed > 0) {
			LOG("RateEngine","cycle done: processed %d (rated %d) in %f sec (%.0f cdr/s)",
				processed,rated,elapsed,processed/elapsed);
		} else {
			LOG("RateEngine","cycle done: 0 CDRs to rate");
		}

		if(active == 't') {
			/* keep draining while the window was non-empty - even if nothing
			 * matched, there may be more (unrateable) backlog ahead. Only sleep
			 * once the queue is genuinely empty. */
			if(processed > 0) {
				/* periodic refresh during a long continuous drain */
				if(++dim_cycles % 1000 == 0) rt_duckdb_load_dims(&duckdb_ctx);
				/* throttle between drain cycles so rating shares DB/CPU with
				 * the CDRMediator instead of monopolising it */
				if(wait_rating > 0) usleep(wait_rating);
				goto loop;
			}
			/* idle: refresh the dimension cache so subscriber/rate/tariff
			 * changes propagate before the next poll */
			rt_duckdb_load_dims(&duckdb_ctx);
			sleep(rating_interval);
			goto loop;
		} else {
			loop_flag = 'f';
		}
	}

	rt_free_duckdb();

_end:
	pthread_exit(NULL);
}

/* dummy functions for rt_bind_api compatibility */
void rt_exec(db_t *dbp,racc_t *rtp,char l) {}
void rt_maxsec(db_t *dbp,racc_t *rtp) {}

int rt_bind_api(rt_funcs_t *api)
{
	if(api == NULL) return -1;

	api->engine = RateEngine;
	api->maxsec = NULL;
	api->exec   = NULL;

	return 0;
}
