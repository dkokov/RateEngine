#include "globals.h"

mod_t *mod_lst = NULL;
main_cfg_t *mcfg = NULL;
int pidFilehandle = 0;
float k_limit_min;
int day_of_payment;
char billing_day[2];
opt_cli_t opt_cli_mem;
re5_server config;
unsigned short get_cdrs_mode;
unsigned long rates_per_bplan;
unsigned int sim_calls;
unsigned int call_maxsec_limit;
char loop_flag;
unsigned int caching_refresh;
unsigned short call_control_flag;
unsigned int console_call_id;
unsigned short rating_flag;
char cdr_storage_sched_ts[64];
unsigned short get_cdrs_flag;
unsigned short demonize;
unsigned int log_max_file_size;
char log_separator;
unsigned short log_debug_level;

void init_globals(void)
{
	get_cdrs_flag = 0;
    rating_flag = 0;
    
    sim_calls = 0;
    rates_per_bplan = 0;
    log_debug_level = 0;
    call_control_flag = 0;
    call_maxsec_limit = 0;
    get_cdrs_mode = 0;
	
	bzero(billing_day,sizeof(billing_day));
	day_of_payment = 0;
	
	k_limit_min = 0;	
}

void init_log_params(void)
{
	char logfile[255];

	sprintf(logfile,"%s/%s",mcfg->system_dir,mcfg->logfile);

	if(strlen(logfile)) config.log = re_open_syslog_2(logfile);
	else config.log = re_open_syslog_2("./rate_engine.log");
		
	strcpy(config.log_filename,logfile);
		
	if(mcfg->log_separator == '\0') log_separator = LOG_SEPARATOR;
	else log_separator = mcfg->log_separator;
		
	if(mcfg->log_max_file_size == 0) log_max_file_size = LOG_MAX_FILE_SIZE;
	else log_max_file_size = mcfg->log_max_file_size;		
}
