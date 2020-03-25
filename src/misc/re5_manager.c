#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "globals.h"
#include "mem/mem.h"
#include "mem/shm_mem.h"

#include "../CDRMediator/cdr_mediator.h"
#include "../CallControl/cc_server.h"
#include "../Rating/rt_cfg.h"

#include "stat/stat.h"
#include "daemon.h"

#include "reload_logfile.h"
#include "re5_manager.h"

int chk_proc(int pid)
{	
	int status;
	pid_t return_pid;
	
	return_pid = waitpid(pid, &status, WNOHANG);
	if (return_pid == -1) {
    	DBG("chk_proc()","error");
    	return RE_ERROR;
	} else if (return_pid == 0) {
    	DBG("chk_proc()","child is still running");
		//return RE_ERROR; ???
	} else if (return_pid == pid) {
    	DBG("chk_proc()","child is finished. exit status in   status");
	}

	return RE_SUCCESS;
}

int get_cdrs(void)
{
	int new_pid;
	re5_mgr_mem_t *ptr;
		
	if(demonize == getpid()) {
		new_pid = fork();
		if(new_pid < 0) {
			LOG("get_cdrs()","Cannot create new process!");
		
			return RE_ERROR;
		}
	
		if(new_pid == 0) {
			/* child process */
			new_pid = getpid();
			LOG("get_cdrs()","CDRMediator process is with PID: %d",new_pid);

			ptr = (re5_mgr_mem_t *)shm_mem_read(RE5_MGR_MEM_KEY,sizeof(re5_mgr_mem_t));
			if(ptr == NULL) return RE_ERROR;

			ptr->cdrm_pid = new_pid;
			ptr->cdrm_ts = time(NULL);
			ptr->cdrm_flag = 't';
			
			shm_mem_free((char *)ptr);
		} else {
			/* parent process */
			return RE_SUCCESS;
		}				
	} else if(demonize) return RE_SUCCESS;
	
	CDRMediatorEngine();
	
	return RE_SUCCESS;
}

int get_cdrs_v2(void)
{
	int rc = 0;

#if PROC_THREAD_FLAG	
	rc = pthread_create(&config.thread_cdrstorage_engine,NULL,CDRMediatorEngine,NULL);
	if(rc) {
		re_write_syslog_2(config.log,"CDRMediatorEngine","pthread_create(cdr_mediator_engine) error");
		return 1;
	}
	
	if(opt_cli_mem.daemon_flag == 0) pthread_join(config.thread_cdrstorage_engine,NULL);
#endif

	return rc;
}

int rating_action_v2(char *leg,char *rating_mode,char *rating_account)
{
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
	
#if PROC_THREAD_FLAG
	int rc = 0;
	pthread_t thr_rate_engine;
	
	rc = pthread_create(&thr_rate_engine,NULL,RateEngine,NULL);
	
	if(rc) {
		PQfinish(rt_eng.conn);
		LOG("rating_action_v2()","pthread_create(rate_engine) error");
		return 1;
	}	
	
	if(opt_cli_mem.daemon_flag == 0) pthread_join(thr_rate_engine,NULL);
#endif

	return RE_SUCCESS;
}

int rating_action(void)
{
	int pid,new_pid;
	re5_mgr_mem_t *ptr;
	
	rt_cfg_t *cfg;

	pid = 0;
	ptr = NULL;

	if(demonize) {
		ptr = (re5_mgr_mem_t *)shm_mem_read(RE5_MGR_MEM_KEY,sizeof(re5_mgr_mem_t));
		if(ptr == NULL) return RE_ERROR;
		
		pid = ptr->re5_mgr_pid;
		if(pid != getpid()) return RE_SUCCESS;
		
		new_pid = fork();
		if(new_pid < 0) {
			LOG("rating_action()","Cannot create new process!");
		
			return RE_ERROR;
		}
	
		if(new_pid == 0) {
			/* child process */
			new_pid = getpid();
			LOG("rating_action()","Rating process is with PID: %d",new_pid);

			ptr->rt_pid = new_pid;
			ptr->rt_ts = time(NULL);
			ptr->rt_flag = 't';
		
		} else {
			/* parent process */
			shm_mem_free((char *)ptr);

			return RE_SUCCESS;
		}
	}

	rt_eng.conn = db_pgsql_conn(mcfg->db);
	rt_eng.log = config.log;
	
	cfg = rt_cfg_main(mcfg->cfg_filename);
	
	if(cfg == NULL) {
		LOG("rating_action()","pthread_create(rate_engine) error");
		return RE_ERROR;
	}
	
	if((mcfg->daemon_flag)&&((cfg->rating_active_flag) == 'f')) return RE_SUCCESS;
	
	rt_eng.active = cfg->rating_active_flag;
	rt_eng.rating_interval = cfg->rating_interval;
	
	if(ptr) {
		ptr->rt_interval = cfg->rt_interval;
		shm_mem_free((char *)ptr);
	}
	
	rt_eng.no_prefix_rating = cfg->no_prefix_rating;
	rt_eng.rating_flag = rating_flag;
	rt_eng.leg = opt_cli_mem.leg[0];
	
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
	
//	RateEngine();
	int rc = 0;
	pthread_t thr_rate_engine;
	
	rc = pthread_create(&thr_rate_engine,NULL,RateEngine,NULL);
	if(rc) {
		PQfinish(rt_eng.conn);
		LOG("rating_action()","pthread_create(rate_engine) error");
		return RE_ERROR;
	}	
	
	if(opt_cli_mem.daemon_flag == 0) pthread_join(thr_rate_engine,NULL);
	
	return RE_SUCCESS;
}

int cc_server_action(void)
{	
	int new_pid;
	re5_mgr_mem_t *ptr;
	
	rt_cfg_t *cfg;
	
	if(demonize == getpid()) {
		ptr = (re5_mgr_mem_t *)shm_mem_read(RE5_MGR_MEM_KEY,sizeof(re5_mgr_mem_t));
		if(ptr == NULL) return RE_ERROR;
		
		new_pid = fork();
		if(new_pid < 0) {
			LOG("cc_server_action()","Cannot create new process!");
		
			return RE_ERROR;
		}
		
		if(new_pid == 0) {
			/* child process */
			new_pid = getpid();
			LOG("cc_server_action()","CallControl process is with PID: %d",new_pid);

			ptr->cc_pid = new_pid;
			ptr->cc_ts = time(NULL);
			ptr->cc_flag = 't';
		} else {
			/* parent process */
			shm_mem_free((char *)ptr);

			return RE_SUCCESS;
		}
		
		shm_mem_free((char *)ptr);	
	}
	
	cfg = rt_cfg_main(mcfg->cfg_filename);

	if(cfg == NULL) {
		LOG("cc_server_action()","read config file error");
		return RE_ERROR;
	}

	if(strcmp(cfg->pcard_sort_key,"") == 0) strcpy(rt_eng.pcard_sort_key,PCARD_SORT_KEY);
	else strcpy(rt_eng.pcard_sort_key,cfg->pcard_sort_key);
	
	if(strcmp(cfg->pcard_sort_mode,"") == 0) strcpy(rt_eng.pcard_sort_key,PCARD_SORT_MODE);
	else strcpy(rt_eng.pcard_sort_mode,cfg->pcard_sort_mode);
	
	mem_free(cfg);
	
	cc_server_main();

	return RE_SUCCESS;
}
/*
int re5_chk_reload_logfile(time_t res)
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
	
		return RE_ERROR;
	}
	
	return RE_SUCCESS;
}*/

int re5_starter(void)
{
	if((call_control_flag)) {		
		if(cc_server_action()) return 1;
	}

	if(get_cdrs_flag) {
#if PROC_THREAD_FLAG
		if(get_cdrs_v2()) return RE_ERROR;
#else
		if(get_cdrs()) return RE_ERROR;
#endif
    }

	if((rating_flag)) {
#if PROC_THREAD_FLAG
		if(rating_action_v2(leg,rating_mode,rating_account)) return RE_ERROR;
#else
		if(rating_action()) return RE_ERROR;
#endif
	}
	
	return RE_SUCCESS;
}

void re5_manager(void)
{
	int pid,n,ret,fd;
	time_t res;	
	char buf[16];
	
	re5_mgr_mem_t *ptr;
				
	ptr = (re5_mgr_mem_t *)shm_mem_read(RE5_MGR_MEM_KEY,sizeof(re5_mgr_mem_t));
	if(ptr == NULL) { 		
		if(demonize) exit(-1);
 		else {
 			while(loop_flag == 't') sleep(LOOP_PAUSE);
 			
 			return;
 		}
 	}

	pid = getpid();
	if(pid == ptr->re5_mgr_pid) {
		/* Waiting to start all processes */
		sleep(LOOP_PAUSE);
				
		while(ptr->re5_mgr_flag == 't') {
			res = time(NULL);
		
			ret = reload_logfile(res);
			if(ret) {
				if(reload_logfile_fd_write(ptr->cc_sv[0], "1", 1, config.log) <= 0) {
					LOG("re5_manager()","'reload_logfile_fd_write()' returned error");
				}
				
				if(reload_logfile_fd_write(ptr->cdrm_sv[0], "1", 1, config.log) <= 0) {
					LOG("re5_manager()","'reload_logfile_fd_write()' returned error");
				}
				
				if(reload_logfile_fd_write(ptr->rt_sv[0], "1", 1, config.log) <= 0) {
					LOG("re5_manager()","'reload_logfile_fd_write()' returned error");
				}		
			}
			
			/* Checking 'CallControl' process */
			if((ptr->cc_pid > 0)&&((res - ptr->cc_ts) > ptr->cc_interval)) {
				DBG("re5_manager()","'cc' is stoped .... %d %d????",(res - ptr->cc_ts),ptr->cc_interval);
				
				if(chk_proc(ptr->cc_pid)) {
					n = 0;
					cc_loop:
					if(n <= RE5_MGR_RET_NUM) {
						LOG("re5_manager()","'cc' is starting again ...%d",n);
						ret = cc_server_action();
						if(ret == RE_ERROR) {
							sleep(LOOP_PAUSE);
							n++;
							goto cc_loop;
						}
					}
				}
			}

			/* Checking 'Rating' process */
			if((ptr->rt_pid > 0)&&((res - ptr->rt_ts) > ptr->rt_interval)) {
				DBG("re5_manager()","'rating' is stoped .... %d %d????",(res - ptr->rt_ts),ptr->rt_interval);
				
				if(chk_proc(ptr->rt_pid)) {
					n = 0;
					rt_loop:
					if(n <= RE5_MGR_RET_NUM) {
						LOG("re5_manager()","'rt' is starting again ...");
						ret = rating_action();
						if(ret == RE_ERROR) {
							sleep(LOOP_PAUSE);
							n++;
							goto rt_loop;
						}
					}
				}
			}
			
			/* Checking 'CDRMediator' process */
			if((ptr->cdrm_pid > 0)&&((res - ptr->cdrm_ts) > ptr->cdrm_interval)) {
				DBG("re5_manager()","'cdrm' is stoped .... %d %d????",(res - ptr->cdrm_ts),ptr->cdrm_interval);
				
				if(chk_proc(ptr->cdrm_pid)) {
					n = 0;
					cdrm_loop:
					if(n <= RE5_MGR_RET_NUM) {
						LOG("re5_manager()","'cdrm' is starting again ...");
						ret = get_cdrs();
						if(ret == RE_ERROR) {
							sleep(LOOP_PAUSE);
							n++;
							goto cdrm_loop;
						}		
					}
				}
			}
			
			sleep(LOG_MNG_PAUSE);
		}
		
		ptr->re5_mgr_pid = 0;
		LOG("re5_manager()","'re5_manager' loop is stoped");
	} else if(pid == ptr->cdrm_pid) {
		while((ptr->cdrm_flag == 't')) {
			if(reload_logfile_fd_read(ptr->cdrm_sv[1], buf, sizeof(buf), &fd) > 0) {
				config.log = fd;
				DBG("re5_manager()","'cdrm' call 'reload_logfile_fd_read()',change 'config.log' fd");
			}
					
			sleep(LOOP_PAUSE);
		}
		
		close(ptr->cdrm_sv[0]);
		close(ptr->cdrm_sv[1]);
		
		ptr->cdrm_pid = 0;
		LOG("re5_manager()","cdrm loop is stoped");		
	} else if(pid == ptr->rt_pid) {
		while((ptr->rt_flag == 't')) {
			if(reload_logfile_fd_read(ptr->rt_sv[1], buf, sizeof(buf), &fd) > 0) {
				config.log = fd;
				DBG("re5_manager()","'rt' call 'reload_logfile_fd_read()',change 'config.log' fd");
			}
					
			sleep(LOOP_PAUSE);
		}		
		
		close(ptr->rt_sv[0]);
		close(ptr->rt_sv[1]);
		
		ptr->rt_pid = 0;
		LOG("re5_manager()","rt loop is stoped");		
	} else if(pid == ptr->cc_pid) {
		while(ptr->cc_flag == 't') {			
			if(reload_logfile_fd_read(ptr->cc_sv[1], buf, sizeof(buf), &fd) > 0) {
				config.log = fd;
				DBG("re5_manager()","'cc' call 'reload_logfile_fd_read()',change 'config.log' fd");
			}
					
			sleep(LOOP_PAUSE);
		}
		
		close(ptr->cc_sv[0]);
		close(ptr->cc_sv[1]);		
		
		ptr->cc_pid = 0;
		LOG("re5_manager()","cc loop is stoped");		
	} 
	
	if((ptr->re5_mgr_pid == 0)&&(ptr->cdrm_pid == 0)&&(ptr->cc_pid == 0)&&(ptr->rt_pid == 0)) shm_mem_remove(RE5_MGR_MEM_KEY,sizeof(re5_mgr_mem_t));
	else shm_mem_free((char *)ptr);
}
