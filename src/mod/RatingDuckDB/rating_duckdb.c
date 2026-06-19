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
static int rating_interval = 300;
static int batch_limit = 5000;
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
	ret = rt_duckdb_init(&duckdb_ctx,mcfg->dbhost,mcfg->dbname,mcfg->dbuser,mcfg->dbpass,mcfg->dbport);
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
				if(strcmp(params->name,"BatchLimit") == 0) batch_limit = atoi(params->value);
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

	loop:
	{
		struct timeval t1,t2;
		int batch_count = 0;

		LOG("RateEngine","DuckDB rating cycle started ...");

		gettimeofday(&t1,NULL);

		if(leg == '\0') {
			batch_count += rt_duckdb_rate_batch(&duckdb_ctx,pg_dbp,'a',batch_limit);
			batch_count += rt_duckdb_rate_batch(&duckdb_ctx,pg_dbp,'b',batch_limit);
		} else {
			batch_count = rt_duckdb_rate_batch(&duckdb_ctx,pg_dbp,leg,batch_limit);
		}

		gettimeofday(&t2,NULL);

		double elapsed = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;

		if(batch_count > 0) {
			LOG("RateEngine","cycle done: %d rated in %f sec (%f ms/cdr, %.0f cdr/s)",
				batch_count,elapsed,
				elapsed/batch_count*1000,
				batch_count/elapsed);
		} else {
			LOG("RateEngine","cycle done: 0 CDRs to rate");
		}

		if(active == 't') {
			if(batch_count > 0) goto loop;
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
