#include "globals.h"

unsigned short log_debug_level;
char log_separator;
unsigned int log_max_file_size;
unsigned short demonize;
unsigned short get_cdrs_flag;
char cdr_storage_sched_ts[64];
unsigned short rating_flag;
unsigned int console_call_id;
unsigned short call_control_flag;
unsigned int caching_refresh;
char loop_flag;
unsigned int call_maxsec_limit;
unsigned int sim_calls;
unsigned int cdr_tcp_conn;
unsigned long rates_per_bplan;
unsigned short get_cdrs_mode;
char billing_day[2];
int day_of_payment;
float k_limit_min;
int pidFilehandle;
cdr_table_t *cdr_tbl_cpy_ptr;

main_cfg_t *mcfg;
rate_engine rt_eng;

re5_server config;
opt_cli_t opt_cli_mem;

void init_globals(void)
{
	get_cdrs_flag = 0;
    rating_flag = 0;
    
    sim_calls = 0;
    cdr_tcp_conn = 0;
    rates_per_bplan = 0;
    log_debug_level = 0;
    call_control_flag = 0;
    call_maxsec_limit = 0;
    get_cdrs_mode = 0;
	
	bzero(billing_day,sizeof(billing_day));
	day_of_payment = 0;
	
	k_limit_min = 0;
	
	cdr_tbl_cpy_ptr = NULL;
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
		
	log_debug_level = mcfg->log_debug_level;
}
