#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>

#include "misc/globals.h"
#include "misc/init.h"
#include "misc/daemon.h"
#include "misc/chk_db_version.h"
#include "misc/re5_manager.h"
#include "misc/mem/mem.h"
#include "misc/stat/stat.h"
#include "syslog/syslog.h"

#include "RateEngine.h"

void help(void)
{
	fprintf(stderr,"\n"
	               " RateEngine commands from the console (help):\n\n"
	               "  --help or -h           ,show this display;\n"
			       "  --conf or -c           ,read and parse a general configuration file;\n"
			       "  --test or -t           ,test(checking) of the config file and the license files;\n"
			       "  --get  or -g           ,get cdrs from all defined CDR servers;\n"
			       "  --get-force or -g2     ,force get all cdrs from all defined CDR servers;\n"
			       "  --rating or -r         ,rating of the all cdrs with defined plan,rates,tariffs;\n"
			       "  --rating-force or -r2  ,force rating;\n"
			       "  --rating-cdr-id or -r3 ,rating of the define cdr id;\n"
			       "  --rating-racc or -r4   ,rating of the define RatingAccountTypeId and RatingAccount;\n"
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
	stat_data_t *ptr;
	
	ptr = stat_read();
	
	if(ptr) {
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(ptr->ts)));

		fprintf(stderr,"\n"
						"  RateEngine realtime 'status & statistics' :\n"
						"    ts : %s\n"
						"    sim: %d\n\n",buff,ptr->sim);
	}
}

int cli_opts_parser(int arg_num, char *arg_arr[])
{
    int i;
    
    for(i=0;i<arg_num;i++) {
		if((!strcmp(arg_arr[i],"--help"))||(!strcmp(arg_arr[i],"-h"))) {
			help();
			return 1;
		}
		
		if((!strcmp(arg_arr[i],"--stat"))) {
			opt_cli_mem.stat_flag = 1;
			return 0;
		}		
		
		if((!strcmp(arg_arr[i],"--test"))||(!strcmp(arg_arr[i],"-t"))) {
			opt_cli_mem.test_flag = 1;
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
				return 1;
			}
			
			strcpy(opt_cli_mem.cfgfile,arg_arr[(i+1)]);
		}
		
		if((!strcmp(arg_arr[i],"--get"))||(!strcmp(arg_arr[i],"-g"))) {
			get_cdrs_flag = 1;
		}
		
		if((!strcmp(arg_arr[i],"--get-force"))||(!strcmp(arg_arr[i],"-g2"))) {
			get_cdrs_flag = 1;
			get_cdrs_mode =1;
		}
	
		if((!strcmp(arg_arr[i],"--rating"))||(!strcmp(arg_arr[i],"-r"))) {
			rating_flag = 1;
			
			if((i+1)<arg_num) strcpy(opt_cli_mem.leg,arg_arr[(i+1)]);
			
			if(opt_cli_mem.leg[0]) printf("Rating of the leg[%c]...\n",opt_cli_mem.leg[0]);
			else return 1;
		}
		
		if((!strcmp(arg_arr[i],"--rating-force"))||(!strcmp(arg_arr[i],"-r2"))) {
			rating_flag = 2;
			
			if((i+1)<arg_num) strcpy(opt_cli_mem.leg,arg_arr[(i+1)]);
		}

		if((!strcmp(arg_arr[i],"--rating-racc"))||(!strcmp(arg_arr[i],"-r4"))) {
			rating_flag = 4;
			
			if((i+1)<arg_num) strcpy(opt_cli_mem.leg,arg_arr[(i+1)]);
		
			if((i+2)<arg_num) strcpy(opt_cli_mem.rating_mode,arg_arr[(i+2)]);
			
			if((i+3)<arg_num) strcpy(opt_cli_mem.rating_account,arg_arr[(i+3)]);
		}		

		if((!strcmp(arg_arr[i],"--ccserver"))||(!strcmp(arg_arr[i],"-2c"))) {
			call_control_flag = 1;
		}
		
		if((!strcmp(arg_arr[i],"--bg"))||(!strcmp(arg_arr[i],"-d"))) {
			opt_cli_mem.daemon_flag = 1;
		}
		
		if((!strcmp(arg_arr[i],"--stop"))||(!strcmp(arg_arr[i],"-k"))) {
			opt_cli_mem.kill_flag = 1;
		}
    }
    
    return 0;
}

int main(int argc, char *argv[])
{            
    config.pid    = getpid();
    config.pid_ts = time(NULL);
    
	init_globals();

	open_syslog();

	loop_flag = 't';

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
		
		LOG("RateEngine","The RateEngine configuration is parsed succesful!");
	} else {
		syslog(LOG_NOTICE,"Cannot open a RE main config file !");
		goto end;
	}
	
    if((rating_flag == 0)&&(get_cdrs_flag == 0)&&(call_control_flag == 0)) {
		if(opt_cli_mem.daemon_flag) {
			rating_flag = 1;
			get_cdrs_flag = 1;
			call_control_flag =1;	
			
			daemonize(mcfg->system_dir, mcfg->system_pid_file);
			LOG("RateEngine","The RateEngine daemon is starting...");
			
			if(stat_init() == 0) LOG("RateEngine","The RateEngine stat is not started (no init SHMEM)!");
		} else if(opt_cli_mem.kill_flag) {
			stat_remove();
			
			chdir(mcfg->system_dir);
			stop_daemon(mcfg->system_pid_file);
			
			LOG("RateEngine","The RateEngine daemon is stoping...");
			goto end;	
		} else {
			LOG("RateEngine","Don't have option");
			goto end;
		}
	}
    
    pthread_mutex_init(&config.sync_bt_thread,NULL);
    
	/* Init 'first' db(pgsql) connection */
	config.conn = db_pgsql_conn(mcfg->db);
    
    if(db_pgsql_conn_check(config.conn,config.log,"main()"))
		LOG("RateEngine","The PGSQL init is succesful!");
    else {
		LOG("RateEngine","The PGSQL init is not succesful!");
		goto end;
    }
    
    if(chk_db_version(mcfg->system_db_version)) {
		LOG("RateEngine","A System DB Version is different from [%s]!",mcfg->system_db_version);
		goto end;		
	} else {
		LOG("RateEngine","A System DB Version is [%s]!",mcfg->system_db_version);
		
		if(mcfg->num_retries == 0) mcfg->num_retries = PGSQL_NUM_RETRIES;
		if(mcfg->int_retries == 0) mcfg->int_retries = PGSQL_INT_RETRIES;
		
		LOG("RateEngine","DB NumberRetries: %d,DB IntervalRetries: %d",mcfg->num_retries,mcfg->int_retries);
	}

	if(re5_starter_v2(opt_cli_mem.leg,opt_cli_mem.rating_account,opt_cli_mem.rating_mode)) goto end;

	re5_manager_v3();
 
	end:
		if(mcfg != NULL) {
			chdir(mcfg->system_dir);
			remove(mcfg->system_pid_file);			
			mem_free(mcfg);
		}
		
		if(cdr_tbl_cpy_ptr != NULL) mem_free(cdr_tbl_cpy_ptr);
		
		close_syslog();
		
		PQfinish(config.conn);
		
		LOG("RateEngine","RateEngine's config data/open files are cleared/closed!");
		
		pthread_mutex_destroy(&config.sync_bt_thread);
		pthread_exit(NULL);
}
