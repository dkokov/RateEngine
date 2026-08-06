#include <pthread.h>
#include <unistd.h>

#include "../mem/mem.h"
#include "../mod/mod.h"
#include "../db/db.h"

#include "globals.h"
#include "re5_fstat.h"

#include "../mod/CDRMediator/cdr_bind_api.h"

//#include "../mod/Rating/rating.h"
//#include "../mod/Rating/pcard.h"
#include "../mod/Rating/rt_data.h"
#include "../mod/Rating/rt_bind_api.h"

//#include "../mod/CallControl/cc.h"
//#include "../mod/CallControl/cc_bind_api.h"

int get_cdrs(void)
{
	int rc;
	
	mod_t *mod_ptr;
	cdr_funcs_t *api = NULL;

	mod_ptr = mod_find_module("cdrm.so");
	
	if((mod_ptr == NULL)||(mod_ptr->handle == NULL)) {
		LOG("get_cdrs()","The module isn't find!");
		return RE_ERROR;
	}
	
	api = (cdr_funcs_t *)mod_find_sim(mod_ptr->handle,"cdrm_api");
	if(api == NULL) {
		LOG("get_cdrs()","ERROR!The struct 'cdrm_api' is not find!");
		return RE_ERROR;
	}
	
	rc = 0;
	
	rc = pthread_create(&config.thread_cdrstorage_engine,NULL,api->engine,NULL);
	if(rc) {
		LOG("CDRMediatorEngine","pthread_create(cdr_mediator_engine) error");
		return RE_ERROR;
	}
	
	/* Only a one-shot CLI run (-g) waits here. In service mode (-d/-f) the
	 * thread runs detached, otherwise this join would block re7_starter() and
	 * the remaining services would never be started. */
	if(run_mode == RUN_ONESHOT) {
		pthread_join(config.thread_cdrstorage_engine,NULL);
		LOG("CDRMediatorEngine","pthread_join() is join");
	}
	
	return RE_SUCCESS;
}

int rating_action(void)
{
	int rc,ret;
	void *func;
	
	mod_t *mod_ptr;
	rt_funcs_t api;
	int (*fptr)(rt_funcs_t *);

	/* offline batch rating backend is config-selectable (<Rating> RatingModule,
	 * default rt.so). Lets RatingDuckDB (rt_duckdb.so) do batch rating while
	 * CallControl keeps binding rt.so for online charging. */
	char *rt_mod = (mcfg != NULL && strlen(mcfg->rating_module) > 0) ? mcfg->rating_module : "rt.so";

	mod_ptr = mod_find_module(rt_mod);

	if((mod_ptr == NULL)||(mod_ptr->handle == NULL)) {
		LOG("rating_action()","ERROR! The rating module '%s' is not found!",rt_mod);
		return RE_ERROR;
	}

	LOG("rating_action()","offline batch rating engine: %s",rt_mod);
	
	memset(&api,0,sizeof(rt_funcs_t));

	func = mod_find_sim(mod_ptr->handle,"rt_bind_api");
	if(func != NULL) {
		fptr = func;
			
		ret = fptr(&api);
		if(ret < 0) {
			LOG("rt_bind_api()","ret: %d",ret);
			return RE_ERROR;
		}
	}
	
	rc = 0;
	
	rc = pthread_create(&config.thread_rate_engine,NULL,api.engine,NULL);
	if(rc) {
		LOG("RateEngine","pthread_create(rate_engine) error");
		return RE_ERROR;
	}
	
	/* one-shot '-r <leg>' waits for the rating run to finish; see get_cdrs() */
	if(run_mode == RUN_ONESHOT) {
		pthread_join(config.thread_rate_engine,NULL);
		LOG("RateEngine","pthread_join() is join");
	}
	
	return RE_SUCCESS;
}

int cc_server_action(void)
{
	void *sym;
	mod_t *mod_ptr;
	void *(*cc_main)(void *);

	/* module file is cc.so (MOD_LIB in mod/CallControl/Makefile) */
	mod_ptr = mod_find_module("cc.so");

	if((mod_ptr == NULL)||(mod_ptr->handle == NULL)) {
		LOG("cc_server_action()","ERROR! The module 'cc.so' is not find!");
		return RE_ERROR;
	}

	/* Resolve the module's server-thread entry by symbol and launch it - the
	 * core treats the module as a black box (no CallControl headers here). */
	sym = mod_find_sim(mod_ptr->handle,"cc_server_main");
	if(sym == NULL) {
		LOG("cc_server_action()","ERROR! The symbol 'cc_server_main' is not find!");
		return RE_ERROR;
	}

	cc_main = (void *(*)(void *))sym;

	if(pthread_create(&config.thread_cc_server,NULL,cc_main,NULL)) {
		LOG("CC","pthread_create(cc_server) error");
		return RE_ERROR;
	}

	/* cc_server_main() is only a setup thread - it spawns the detached janitor
	 * and interface threads and exits, so this join returns quickly either way */
	if(run_mode == RUN_ONESHOT) {
		pthread_join(config.thread_cc_server,NULL);
		LOG("CC","pthread_join() is join");
	}

	return RE_SUCCESS;
}

void re5_chk_reload_logfile(time_t res)
{
	char new_fn[256];
	struct tm *tt;
	unsigned int filesize;
	
	tt = localtime(&res);
		
	filesize = re5_fstat(config.log_filename);
	if(filesize >= log_max_file_size) {				
		sprintf(new_fn,"%s.%.4d%.2d%.2d%.2d%.2d%.2d",config.log_filename,
										 (tt->tm_year+1900),(tt->tm_mon+1),(tt->tm_mday),
										 (tt->tm_hour),(tt->tm_min),(tt->tm_sec));
		re_reload_syslog(config.log,config.log_filename,new_fn);
			
		LOG("re5_chk_reload_logfile()","A syslog is reloaded(%s)!",new_fn);
	}
}

/* In service mode (-d/-f) the set of services to run comes from the
 * '<param name="active">' switches in RateEngine7.xml. An explicit CLI service
 * flag (-g/-r/-2c) overrides the config and forces a one-shot run of just that
 * service, so this is only reached when none of them was given. */
static void re7_services_from_cfg(void)
{
	if(run_mode != RUN_SERVICE) return;
	if(mcfg == NULL) return;

	get_cdrs_flag     = (mcfg->cdrm_active   == 't') ? 1 : 0;
	rating_flag       = (mcfg->rating_active == 't') ? 1 : 0;
	call_control_flag = (mcfg->cc_active     == 't') ? 1 : 0;

	LOG("re7_starter()","services from config: cdrmediator=%c rating=%c callcontrol=%c",
		mcfg->cdrm_active,mcfg->rating_active,mcfg->cc_active);

	if((get_cdrs_flag == 0)&&(rating_flag == 0)&&(call_control_flag == 0))
		LOG("re7_starter()","WARNING! No active service in the config - nothing to start!");
}

int re7_starter(void)
{
	/* an explicit CLI service flag means one-shot; otherwise take the config */
	if((rating_flag == 0)&&(get_cdrs_flag == 0)&&(call_control_flag == 0))
		re7_services_from_cfg();

    if((call_control_flag)) {
		if(cc_server_action()) return RE_ERROR;
    }

	if(get_cdrs_flag) {
		if(get_cdrs()) return RE_ERROR;
    }

	if((rating_flag)) {
		if(rating_action()) return RE_ERROR;
	}
    
	return RE_SUCCESS;
}

void re7_manager(void)
{
	time_t res;

	do {
//	while(loop_flag == 't') {	
		res = time(NULL);
		
		re5_chk_reload_logfile(res);

		sleep(LOG_MNG_PAUSE);
//	}
	} while(loop_flag == 't');


	LOG("re7_manager","loop is stoped");
}
