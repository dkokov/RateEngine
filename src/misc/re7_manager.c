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
	
	if(opt_cli_mem.daemon_flag == 0) {
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

	mod_ptr = mod_find_module("rt.so");
	
	if((mod_ptr == NULL)||(mod_ptr->handle == NULL)) {
		LOG("rating_action()","ERROR!The module 'rt.so' is not find!");
		return RE_ERROR;
	}
	
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
	
	if(opt_cli_mem.daemon_flag == 0) {
		pthread_join(config.thread_rate_engine,NULL);
		LOG("RateEngine","pthread_join() is join");
	}
	
	return RE_SUCCESS;
}

int cc_server_action(void)
{	
/*	int rc,ret;
	void *func;
	
	mod_t *mod_ptr;
	cc_funcs_t api;
	int (*fptr)(cc_funcs_t *);

	mod_ptr = mod_find_module("CallControl.so");

	if(mod_ptr == NULL) return RE_ERROR;
	if(mod_ptr->handle == NULL) return RE_ERROR;
	
	memset(&api,0,sizeof(cc_funcs_t));

	func = mod_find_func(mod_ptr->handle,"cc_bind_api");
	if(func != NULL) {
		fptr = func;
			
		ret = fptr(&api);
		if(ret < 0) {
			LOG("cc_bind_api()","ret: %d",ret);
			return RE_ERROR;
		}
	}
	
	rc = 0;
	
	rc = pthread_create(&config.thread_cc_server,NULL,api.cc_main,NULL);
	if(rc) {
		LOG("CC","pthread_create(rate_engine) error");
		return RE_ERROR;
	}
	
	if(opt_cli_mem.daemon_flag == 0) {
		pthread_join(config.thread_cc_server,NULL);
		LOG("CC","pthread_join() is join");
	}
	*/
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

int re7_starter(void)
{
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
