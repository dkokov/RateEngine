#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>

#include "misc/globals.h"
#include "misc/init.h"
#include "misc/daemon.h"
#include "misc/chk_db_version.h"
#include "misc/re7_manager.h"
#include "mem/mem.h"
#include "misc/stat/stat.h"

#include "db/db.h"
#include "net/net.h"

#include "RateEngine.h"

void help(void)
{
	fprintf(stderr,"\n"
	               " RateEngine commands from the console (help):\n\n"
	               "  --help or -h           ,show this display;\n"
			       "  --conf or -c           ,read and parse a general configuration file;\n"
			       "  --test or -t           ,test(checking) of the config file;\n"
			       "  --get  or -g           ,get cdrs from all defined CDR servers;\n"
			       "  --rating or -r         ,rating of the all cdrs with defined plan,rates,tariffs;\n"
			       "  --ccserver or -2c      ,start Call Control server(not fork);\n"
			       "  --stop or -k           ,exit from the backgroud mode of the RateEngine(stop demonization);\n"
			       "  --version or -v        ,show version of the RateEngine;\n"
			       "  --bg  or -d            ,backgroud mode of the RateEngine(demonization);\n"
			       "  --fg  or -f            ,foreground mode - same services as '-d' but no fork\n"
			       "                          (for containers/systemd Type=simple);\n"
			       "  --debug [0-7]          ,debug log level in 'not fork' mode;\n"
			       "  --stat                 ,show RateEngine status services and global statistics;\n"
			       "\n"
			       );
}

void re_stat(void)
{
	char buff[20];
	char rt_buff[20];
	char cdrm_buff[20];
	stat_data_t *ptr;

	ptr = stat_read();

	if(ptr) {
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(ptr->ts)));

		fprintf(stderr,"\n"
						"  RateEngine manager 'status':\n"
						"    ts :  %s\n"
						"    pid:  %d\n"
						"    flag: '%c'\n"
						"    sim:  %d\n",
						buff,ptr->mgr_pid,ptr->mgr_flag,ptr->sim);

		if(ptr->rt_ts) {
			strftime(rt_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(ptr->rt_ts)));

			fprintf(stderr,"\n"
							"  Rating 'statistics':\n"
							"    ts :     %s\n"
							"    flag:    '%c'\n"
							"    total:   %d\n"
							"    success: %d\n"
							"    error:   %d\n"
							"    mins:    %d\n",
							rt_buff,ptr->rt_flag,
							ptr->rt_total_calls,ptr->rt_success_calls,
							ptr->rt_error_calls,ptr->rt_rating_minutes);
		}

		if(ptr->cdrm_ts) {
			strftime(cdrm_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(ptr->cdrm_ts)));

			fprintf(stderr,"\n"
							"  CDRMediator 'statistics':\n"
							"    ts :   %s\n"
							"    flag:  '%c'\n"
							"    total: %d\n",
							cdrm_buff,ptr->cdrm_flag,ptr->cdrm_total_cdrs);
		}

		fprintf(stderr,"\n"
						"  CallControl:\n"
						"    flag: '%c'\n\n",ptr->cc_flag);
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
		
		if((!strcmp(arg_arr[i],"--debug"))) {
			opt_cli_mem.debug = atoi(arg_arr[i+1]);
		}
				
		if((!strcmp(arg_arr[i],"--test"))||(!strcmp(arg_arr[i],"-t"))) {
			opt_cli_mem.test_flag = TRUE;
			i++;
			
			if(!strcmp(arg_arr[i],"db")) opt_cli_mem.test_mode = TEST_DB;
			else if(!strcmp(arg_arr[i],"net")) opt_cli_mem.test_mode = TEST_NET;
			else opt_cli_mem.test_mode = TEST_ALL;
		}
				
		if((!strcmp(arg_arr[i],"--version"))||(!strcmp(arg_arr[i],"-v"))) {
			fprintf(stderr,"\n"
			               "  Version: %s.%s\n"
			               "  Date:    %s\n"
			               "  Creator: %s\n"
			               "\n",
			        VERSION,RELEASE,DATE_RELEASE,CREATOR);
			return RE_ERROR;
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
			rating_flag = TRUE;
			
			if((i+1)<arg_num) strcpy(opt_cli_mem.leg,arg_arr[(i+1)]);
			
			if(opt_cli_mem.leg[0]) printf("Rating of the leg[%c]...\n",opt_cli_mem.leg[0]);
			else return RE_ERROR;
		}

		if((!strcmp(arg_arr[i],"--ccserver"))||(!strcmp(arg_arr[i],"-2c"))) {
			call_control_flag = TRUE;
		}

		if((!strcmp(arg_arr[i],"--bg"))||(!strcmp(arg_arr[i],"-d"))) {
			opt_cli_mem.daemon_flag = TRUE;
			run_mode = RUN_SERVICE;
		}

		/* foreground service mode: same services as '-d' but no fork - the
		 * process stays attached to the tty/container init and keeps stdio */
		if((!strcmp(arg_arr[i],"--fg"))||(!strcmp(arg_arr[i],"-f"))) {
			opt_cli_mem.foreground_flag = TRUE;
			run_mode = RUN_SERVICE;
		}

		if((!strcmp(arg_arr[i],"--stop"))||(!strcmp(arg_arr[i],"-k"))) {
			opt_cli_mem.kill_flag = TRUE;
		}
    }
    
    return RE_SUCCESS;
}

int main(int argc, char *argv[])
{
	char mod_dir[1024];
	     
    config.pid    = getpid(); // izpolzvali se za neshto ???
    config.pid_ts = time(NULL); // izpolzvali se za neshto ???
    
	init_globals();

	loop_flag = 't';

    if(argc == 1) {
		fprintf(stderr,"\nERROR!!!\nThe application is not worked without arguments!\n\n");
		help();
		
		goto end;
    }

	/* Init global struct 'cli_opt_mem' */
	memset(&opt_cli_mem,0,sizeof(opt_cli_mem));
	
	/* Parser for cli options */
	if(cli_opts_parser(argc,argv)) {
		//fprintf(stderr,"\nERROR!!!\nCLI options parser error!!!\n\n");
		goto end;
	}

	/* Show RateEngine Status&Statistics */
	if(opt_cli_mem.stat_flag == TRUE) {
		re_stat();
		exit(EXIT_SUCCESS);
	}

	/* Get cfg filename and copy to 'cfgfile' */
	if(strlen(opt_cli_mem.cfgfile) == 0) {
		strcpy(opt_cli_mem.cfgfile,CONF);
    }

    /* Get main config from xml file */
    mcfg = main_cfg_main(opt_cli_mem.cfgfile);

    if(mcfg != NULL) {
		/* Init parameters for logging */
		init_log_params();
		
		/* Check opening the 'RE' log file is success */
		if(config.log < 0) {
			fprintf(stderr,"\nERROR!!!\nCannot open the 'RE' log file!!!\n\n");
			goto end;
		}
		
		/* Init daemon flag('did we fork') and run mode in a 'mcfg' struct */
		mcfg->daemon_flag = opt_cli_mem.daemon_flag;
		mcfg->run_mode    = (unsigned short)run_mode;

		/* Log level: the config value applies to every mode; an explicit
		 * '--debug N' on the command line overrides it. (The old test here was
		 * 'if(&opt_cli_mem.debug > 0)' - the address of a struct member, always
		 * true - so --debug silently forced level 0 in the CLI modes.) */
		log_debug_level = mcfg->log_debug_level;
		if(opt_cli_mem.debug > 0) log_debug_level = opt_cli_mem.debug;
		
		LOG("RateEngine","The RateEngine configuration is parsed succesful!");
	} else {
		fprintf(stderr,"\nERROR!!!\nCannot open a RE main config file !\n\n");
		goto end;
	}

	/* Load RateEngine's modules */
	memset(mod_dir,0,sizeof(mod_dir));

	if(strlen(mcfg->system_dir) > 0) sprintf(mod_dir,"%smodules/",mcfg->system_dir);
	else strcpy(mod_dir,MODF);

	mod_load_modules(mod_dir);
	if(mod_lst == NULL) {
		LOG("RateEngine","The 'mod_lst' is empty - don't load modules !!!");
		goto end;
	}
	
	/* Test DB,NET,CFG,LOG */
	if(opt_cli_mem.test_flag) {
		main_cfg_view(mcfg);
		if((opt_cli_mem.test_mode == TEST_DB)||(opt_cli_mem.test_mode == TEST_ALL)) db_test();
		if((opt_cli_mem.test_mode == TEST_NET)||(opt_cli_mem.test_mode == TEST_ALL)) net_test();
		goto end;
	}
	
	/* Which mode do we run in?
	 *
	 *   -d  : service mode, forked into the background
	 *   -f  : service mode, stays in the foreground (container/systemd)
	 *   -g / -r / -2c : one-shot run of that single service
	 *   -k  : stop a running service
	 *
	 * In service mode re7_starter() picks the services to launch from the
	 * 'active' switches in the config - it no longer forces all three on. */
	if(run_mode == RUN_SERVICE) {
		if(opt_cli_mem.daemon_flag) {
			daemonize(mcfg->system_dir, mcfg->system_pid_file);
			LOG("RateEngine","The RateEngine daemon is starting...");
		} else {
			if(run_foreground(mcfg->system_dir, mcfg->system_pid_file)) {
				LOG("RateEngine","The RateEngine foreground mode cannot be started!");

				/* Fail with a non-zero status, like the '-d' path does: a
				 * refused start (another instance holds the pid file lock)
				 * must be visible to whoever supervises us - docker, systemd
				 * or a shell. 'goto end' would fall through to the closing
				 * pthread_exit(NULL) and report success. */
				exit(EXIT_FAILURE);
			}

			LOG("RateEngine","The RateEngine foreground is starting...");
		}

		if(stat_init() == 0) LOG("RateEngine","The RateEngine stat is not started (no init SHMEM)!");
	} else if(opt_cli_mem.kill_flag) {
		stat_remove();

		if(chdir(mcfg->system_dir) < 0) LOG("RateEngine","Could not chdir to '%s'",mcfg->system_dir);
		stop_daemon(mcfg->system_pid_file);

		LOG("RateEngine","The RateEngine daemon is stoping...");
		goto end;
	} else if((rating_flag == 0)&&(get_cdrs_flag == 0)&&(call_control_flag == 0)) {
		LOG("RateEngine","Don't have option");
		goto end;
	}


    pthread_mutex_init(&config.sync_bt_thread,NULL);
    
    /* RateEngine starter - get CDRs,rating(offline),CallControl */
	if(re7_starter()) goto end;

	/* RateEngine manager(loop -keep the app,logfile size cheching and rollover logfile ) */
	re7_manager();

	end:
		if(mcfg != NULL) {
			/* Only remove the pid file if THIS process created it (service
			 * mode). Otherwise a plain '-t'/'-r'/'-g' run would delete the pid
			 * file of the daemon that is actually running and break '-k'. */
			if(pidFilehandle > 0) {
				if(chdir(mcfg->system_dir) < 0) LOG("RateEngine","Could not chdir to '%s'",mcfg->system_dir);
				remove(mcfg->system_pid_file);
			}

			mem_free(mcfg);
		}
						
		LOG("RateEngine","RateEngine's config data/open files are cleared/closed!");

		if(mod_lst != NULL) {
			mod_destroy_modules();
			mod_free();
		}
		
		LOG_CLOSE;
		
		pthread_mutex_destroy(&config.sync_bt_thread);
		pthread_exit(NULL); // ??? exit(EXIT_SUCCESS);
}
