#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/socket.h>

#include "misc/globals.h"
#include "misc/init.h"
#include "misc/daemon.h"
#include "misc/chk_db_version.h"
#include "misc/mem/mem.h"
#include "syslog/syslog.h"

#include "misc/mem/shm_mem.h"
#include "misc/re5_manager.h"

#include "Rating/rt_stat.h"
#include "CallControl/cc_stat.h"

#include "RateEngine.h"

void help(void)
{
	fprintf(stderr,"\n"
	               " RateEngine commands from the console (help):\n\n"
	               "  --help or -h           ,show this display;\n"
			       "  --conf or -c           ,read and parse a general configuration file;\n"
			       "  --test or -t           ,test(checking) of the config file and the license files;\n"
			       "  --get  or -g           ,get cdrs from all defined CDR servers;\n"
			       "  --rating or -r         ,rating of the all cdrs with defined plan,rates,tariffs;\n"
			       "  --ccserver or -2c      ,start Call Control server(not fork);\n"
			       "  --stop or -k           ,exit from the backgroud mode of the RateEngine5(stop demonization);\n"
			       "  --version or -v        ,show version of the RateEngine5;\n"
			       "  --bg  or -d            ,backgroud mode of the RateEngine5(demonization);\n"
			       "  --stat                 ,show RateEngine status services and global statistics"
			       "\n"
			       );
}

void re_stat(void)
{
	char buff[20];
	
	shm_mem_t shm,cc_st_shm;
	rt_stat_t *rt;
	cc_status_t *status;
	
	cc_status_init(&cc_st_shm,0);
	status = (cc_status_t *)cc_st_shm.ptr;
	if(status) {
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(status->cc_status_ts)));
		
		fprintf(stderr,"\n"
						"  CallControl thread 'status' :\n"
						"    ts : %s\n"
						"    sim: %d\n",buff,status->cc_status_sim);
		
		cc_status_free(&cc_st_shm,0);
	}
	
	rt_stat_init(&shm,0);
	rt = (rt_stat_t *)shm.ptr;
	if(rt != NULL) {
		memset(buff,0,sizeof(buff));
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(rt->rt_stat_ts)));
		
		fprintf(stderr,"\n"
						"  Rating thread 'statistics' :\n"
						"    ts :     %s\n"
						"    total:   %d\n"
						"    success: %d\n"
						"    error:   %d\n"
						"    mins:    %d\n",
						buff,rt->rt_stat_total_calls,rt->rt_stat_success_calls,rt->rt_stat_error_calls,rt->rt_stat_rating_minutes);
		
		rt_stat_free(&shm,0);
	}
}

void re5_mgr_stat(void)
{
	char buff[20];
	re5_mgr_mem_t *ptr;
	
	ptr = re5_mgr_mem_init(&re5_mgr_shm,2);
	if(ptr) {
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(ptr->re5_mgr_ts)));
		
		fprintf(stderr,"\n"
						"  RateEngine manager process 'status':\n"
						"    re5_mgr_flag: '%c'\n"
						"    re5_mgr_ts : %s\n"
						"    re5_mgr_pid: %d\n",ptr->re5_mgr_flag,buff,ptr->re5_mgr_pid);
		
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(ptr->rt_ts)));
		
		fprintf(stderr,"\n"
						"  Rating process 'status':\n"
						"    rt_flag: '%c'\n"
						"    rt_ts : %s\n"
						"    rt_interval: %d\n"
						"    rt_pid: %d\n",ptr->rt_flag,buff,ptr->rt_interval,ptr->rt_pid);
		
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(ptr->cdrm_ts)));
		
		fprintf(stderr,"\n"
						"  CDRMediator process 'status':\n"
						"    cdrm_flag: '%c'\n"
						"    cdrm_ts : %s\n"
						"    cdrm_interval: %d\n"
						"    cdrm_pid: %d\n",ptr->cdrm_flag,buff,ptr->cdrm_interval,ptr->cdrm_pid);
						
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(ptr->cc_ts)));
		
		fprintf(stderr,"\n"
						"  CallControl process 'status':\n"
						"    cc_flag: '%c'\n"
						"    cc_ts : %s\n"
						"    cc_interval: %d\n"
						"    cc_pid: %d\n",ptr->cc_flag,buff,ptr->cc_interval,ptr->cc_pid);
		
		fprintf(stderr,"\n\n");
		
		re5_mgr_mem_free(&re5_mgr_shm,0);
	}
}

int cli_opts_parser(int arg_num, char *arg_arr[])
{
    int i;
    
    for(i=0;i<arg_num;i++) {
		if((!strcmp(arg_arr[i],"--help"))||(!strcmp(arg_arr[i],"-h"))) {
			help();
			return RE_ERROR;
		}
		
		if((!strcmp(arg_arr[i],"--stat"))) {
			opt_cli_mem.stat_flag = TRUE;
			return RE_SUCCESS;
		}		
		
		if((!strcmp(arg_arr[i],"--test"))||(!strcmp(arg_arr[i],"-t"))) {
			opt_cli_mem.test_flag = TRUE;
		}
				
		if((!strcmp(arg_arr[i],"--version"))||(!strcmp(arg_arr[i],"-v"))) {
			fprintf(stderr,"\n"
			               "  Version: %s.%s\n"
			               "  Date:    %s\n"
			               "  Creator: %s\n"
			               "\n",
			        VERSION,RELEASE,DATE_RELEASE,CREATOR);
			return 1;
		}
		
		if((!strcmp(arg_arr[i],"--conf"))||(!strcmp(arg_arr[i],"-c"))) {
			if(arg_num < (i+1)) {
				printf("Lose a second argument(cfg filename)!\n");
				return RE_ERROR;
			}
			
			strcpy(opt_cli_mem.cfgfile,arg_arr[(i+1)]);
		}
		
		if((!strcmp(arg_arr[i],"--get"))||(!strcmp(arg_arr[i],"-g"))) {
			get_cdrs_flag = TRUE;
		}

		if((!strcmp(arg_arr[i],"--rating"))||(!strcmp(arg_arr[i],"-r"))) {
			rating_flag = 1;
			
			if((i+1)<arg_num) strcpy(opt_cli_mem.leg,arg_arr[(i+1)]);
			
			if(opt_cli_mem.leg[0]) printf("Rating of the leg[%c]...\n",opt_cli_mem.leg[0]);
			else return RE_ERROR;
		}

		if((!strcmp(arg_arr[i],"--ccserver"))||(!strcmp(arg_arr[i],"-2c"))) {
			call_control_flag = TRUE;
		}
		
		if((!strcmp(arg_arr[i],"--bg"))||(!strcmp(arg_arr[i],"-d"))) {
			opt_cli_mem.daemon_flag = TRUE;
		}
		
		if((!strcmp(arg_arr[i],"--stop"))||(!strcmp(arg_arr[i],"-k"))) {
			opt_cli_mem.kill_flag = TRUE;
		}
    }
    
    return RE_SUCCESS;
}

int main(int argc, char *argv[])
{   
	int pid;
	
	re5_mgr_mem_t *mgr;
	
	init_globals();
	
	open_syslog();
	
	loop_flag = 't';
	
	pid = 0;
	
	if(argc == 1) {
		syslog (LOG_NOTICE,"The application is not worked without arguments!");
		help();
		goto end;
	}
	
	/* Init global struct 'cli_opt_mem' */
	memset(&opt_cli_mem,0,sizeof(opt_cli_mem));
	
	/* Parser for cli options */
	if(cli_opts_parser(argc,argv)) {
		syslog(LOG_NOTICE,"CLI options parser error!!!");
		goto end;
	}

	/* Show RateEngine Status&Statistics */
	if(opt_cli_mem.stat_flag == 1) {
		re_stat();
		re5_mgr_stat();
		exit(0);
	}

	/* Get cfg filename and copy to 'cfgfile' */
    if(strcmp(opt_cli_mem.cfgfile,"") == 0) {
		strcpy(opt_cli_mem.cfgfile,CONF);
    }

    /* Get main config from xml file */
    mcfg = main_cfg_main(opt_cli_mem.cfgfile);
    
    if(mcfg != NULL) {
		/* Init parameters for logging */
		init_log_params();
		
		/* config.log = ? , open is success ? */
		
		close_syslog();
		
		/* Init daemon flag in a 'mcfg' struct */
		mcfg->daemon_flag = opt_cli_mem.daemon_flag;
		
		LOG("main()","The RateEngine configuration is parsed succesful!");
	} else {
		syslog(LOG_NOTICE,"Cannot open a RE main config file !");
		goto end;
	}
	
    if((rating_flag == 0)&&(get_cdrs_flag == 0)&&(call_control_flag == 0)) {
		if(opt_cli_mem.daemon_flag) {
			rating_flag = 1;
			get_cdrs_flag = 1;
			call_control_flag = 1;
			
			pid = daemonize(mcfg->system_dir, mcfg->system_pid_file);
			LOG("main()","The RateEngine daemon is starting...");
			
			mgr = re5_mgr_mem_init(&re5_mgr_shm,1);
			if(mgr == NULL) {
				LOG("main()","The RateEngine 're5_mgr_mem' is not started (no init SHMEM)!");
				goto end;
			} else {
				mgr->re5_mgr_pid = pid;
				mgr->re5_mgr_ts = time(NULL);
				mgr->re5_mgr_flag = 't';
				
				if(socketpair(AF_LOCAL, SOCK_STREAM, 0, mgr->cc_sv) < 0) {
					LOG("main()","socketpair error");
					goto end;
				}
				
				if(socketpair(AF_LOCAL, SOCK_STREAM, 0, mgr->cdrm_sv) < 0) {
					LOG("main()","socketpair error");
					goto end;
				}
				
				if(socketpair(AF_LOCAL, SOCK_STREAM, 0, mgr->rt_sv) < 0) {
					LOG("main()","socketpair error");
					goto end;
				}
				
				re5_mgr_mem_free(&re5_mgr_shm,0);
			}

			demonize = pid;
		} else if(opt_cli_mem.kill_flag) {
			mgr = re5_mgr_mem_init(&re5_mgr_shm,2);
			if(mgr) {
				mgr->cc_flag = 'f';
				mgr->rt_flag = 'f';
				mgr->cdrm_flag = 'f';
				mgr->re5_mgr_flag = 'f';
				
				re5_mgr_mem_free(&re5_mgr_shm,0);
			}
			
			LOG("main()","The RateEngine processes are stoping...");
			goto end;
		} else {
			LOG("main()","Don't have option");
			goto end;
		}
	}
        
	/* Init 'first' db(pgsql) connection */
	config.conn = db_pgsql_conn(mcfg->db);
    
    if(db_pgsql_conn_check(config.conn,config.log,"main()"))
		LOG("main()","The PGSQL init is succesful!");
    else {
		LOG("main()","The PGSQL init is not succesful!");
		goto end;
    }
    
    if(chk_db_version(mcfg->system_db_version)) {
		LOG("main()","A System DB Version is different from [%s]!",mcfg->system_db_version);
		goto end;
	} else {
		LOG("main()","A System DB Version is [%s]!",mcfg->system_db_version);
		
		if(mcfg->num_retries == 0) mcfg->num_retries = PGSQL_NUM_RETRIES;
		if(mcfg->int_retries == 0) mcfg->int_retries = PGSQL_INT_RETRIES;
		
		LOG("main()","DB NumberRetries: %d,DB IntervalRetries: %d",mcfg->num_retries,mcfg->int_retries);
	}

	if(re5_starter()) goto end;

	re5_manager();
	 
	end:
		if(mcfg != NULL) {
			chdir(mcfg->system_dir);
			remove(mcfg->system_pid_file);
			mem_free(mcfg);
		}
		
		if(cdr_tbl_cpy_ptr != NULL) mem_free(cdr_tbl_cpy_ptr);
		
		close_syslog();
		
		PQfinish(config.conn);
		
		LOG("main()","RateEngine's config data/open files are cleared/closed! %d",getpid());
		
		exit(0);
}
