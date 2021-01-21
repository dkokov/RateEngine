#include <unistd.h>

#include "globals.h"
#include "mem/mem.h"
#include "re5_fstat.h"

#include "../CDRMediator/cdr_mediator.h"
#include "../CallControl/cc_server.h"
#include "../Rating/rt_cfg.h"

//#include "../misc/tcp/tcp.h"

int get_cdrs_v2(void)
{
	int rc = 0;
	
	rc = pthread_create(&config.thread_cdrstorage_engine,NULL,CDRMediatorEngine,NULL);
	if(rc) {
		re_write_syslog_2(config.log,"CDRMediatorEngine","pthread_create(cdr_mediator_engine) error");
		return 1;
	}
	
	if(opt_cli_mem.daemon_flag == 0) pthread_join(config.thread_cdrstorage_engine,NULL);
	
	return rc;
}

int rating_action_v2(char *leg,char *rating_mode,char *rating_account)
{
	int rc = 0;
	pthread_t thr_rate_engine;
	
	rt_cfg_t *cfg;
	
	rt_eng.conn = db_pgsql_conn(mcfg->db);
	rt_eng.log = config.log;
	
	cfg = rt_cfg_main(mcfg->cfg_filename);
	
	if(cfg == NULL) {
		LOG("rating_action_v2()","pthread_create(rate_engine) error");
		return 1;
	}
	
	if((mcfg->daemon_flag)&&((cfg->rating_active_flag) == 'f')) return 0;
	
	rt_eng.active = cfg->rating_active_flag;
	rt_eng.rating_interval = cfg->rating_interval;
	rt_eng.leg = leg[0];
	rt_eng.no_prefix_rating = cfg->no_prefix_rating;
	rt_eng.rating_flag = rating_flag;
	
	strcpy(rt_eng.rating_mode,rating_mode);
	strcpy(rt_eng.rating_account,rating_account);
	
	if(strcmp(cfg->pcard_sort_key,"") == 0) strcpy(rt_eng.pcard_sort_key,PCARD_SORT_KEY);
	else strcpy(rt_eng.pcard_sort_key,cfg->pcard_sort_key);
	
	if(strcmp(cfg->pcard_sort_mode,"") == 0) strcpy(rt_eng.pcard_sort_key,PCARD_SORT_MODE);
	else strcpy(rt_eng.pcard_sort_mode,cfg->pcard_sort_mode);
			
	if((rt_eng.leg == '\0')&&(cfg->leg != '\0')) {
		rt_eng.leg = cfg->leg;
	}
	
	if(cfg->bal_num > 0) rt_eng.bal_num = cfg->bal_num; 
	else rt_eng.bal_num = RT_BAL_NUM;
	
	mem_free(cfg);
	
	rc = pthread_create(&thr_rate_engine,NULL,RateEngine,NULL);
	
	if(rc) {
		PQfinish(rt_eng.conn);
		LOG("rating_action_v2()","pthread_create(rate_engine) error");
		return 1;
	}
	
	if(opt_cli_mem.daemon_flag == 0) pthread_join(thr_rate_engine,NULL);
	
	return 0;
}

int cc_server_action(void)
{	
	rt_cfg_t *cfg;
	cfg = rt_cfg_main(mcfg->cfg_filename);

	if(cfg == NULL) {
		re_write_syslog_2(rt_eng.log,"cc_server_action()","read config file error");
		return 1;
	}

	if(strcmp(cfg->pcard_sort_key,"") == 0) strcpy(rt_eng.pcard_sort_key,PCARD_SORT_KEY);
	else strcpy(rt_eng.pcard_sort_key,cfg->pcard_sort_key);
	
	if(strcmp(cfg->pcard_sort_mode,"") == 0) strcpy(rt_eng.pcard_sort_key,PCARD_SORT_MODE);
	else strcpy(rt_eng.pcard_sort_mode,cfg->pcard_sort_mode);
	
	mem_free(cfg);
	
	cc_server_main();

	return 0;
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

int re5_starter_v2(char *leg,char *rating_account,char *rating_mode)
{
    if((call_control_flag)) {		
		if(cc_server_action()) return 1;
    }

	if(get_cdrs_flag) {
		if(get_cdrs_v2()) return 1;
    }

	if((rating_flag)) {
		if(rating_action_v2(leg,rating_mode,rating_account)) return 1;
	}
    
	return 0;
}

void re5_manager_v3(void)
{
//	char buf[64];
//	int sockfd;
	time_t res;
	
//	sockfd = 0;
//	if(opt_cli_mem.daemon_flag == 1) {
//		sockfd = tcp_client_connect("127.0.0.1",9090);
//	}
	
	while(loop_flag == 't') {	
		res = time(NULL);
		
		re5_chk_reload_logfile(res);

//		if(sockfd > 0) {
//			memset(buf,0,sizeof(buf);			
//			tcp_send(sockfd,buf,sizeof(buf));
//			
//			memset(buf,0,sizeof(buf);
//			tcp_recv(sockfd,buf,sizeof(buf));
//		}

		sleep(LOG_MNG_PAUSE);
	}
	
	LOG("re5_manager_v3","loop is stoped");
//	if(opt_cli_mem.daemon_flag == 1) tcp_close(sockfd);
}
