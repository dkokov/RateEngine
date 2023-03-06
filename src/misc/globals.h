#ifndef GLOBALS_H
#define GLOBALS_H

/* Default path to the config file */
#define CONF "/usr/local/RateEngine/config/RateEngine7.xml"

/* Default path to the modules files */
#define MODF "/usr/local/RateEngine/modules/"

/* Default log manager pause(seconds) - time interval waiting */
#define LOG_MNG_PAUSE 2

/* Default log max file size(bytes) */
#define LOG_MAX_FILE_SIZE 20480000

#include "../config/main_cfg.h"
#include "../log/rt_log.h"

#include "../mod/mod.h"

#define RE_SUCCESS  0
#define RE_ERROR    1
#define RE_ERROR_N -1

#if !defined(TRUE)
    #define TRUE 1
#endif

#if !defined(FALSE)
	#define FALSE 0
#endif

#define TEST_ALL 0
#define TEST_DB  1
#define TEST_NET 2

/* Logging Level */
unsigned short log_debug_level;

char log_separator;

unsigned int log_max_file_size;

/* A flag for demonize of the RE5 */
unsigned short demonize;

/* A flag for CDRMediator starting */
unsigned short get_cdrs_flag;

char cdr_storage_sched_ts[64];

/* A flag for RatingEngine starting */
unsigned short rating_flag;

unsigned int console_call_id;

/* A flag for CallControl(ccserver) starting */
unsigned short call_control_flag;

unsigned int caching_refresh;

/* Start/Stop RE5 Manager */
char loop_flag;

unsigned int call_maxsec_limit;
unsigned int sim_calls;
//unsigned int cdr_tcp_conn;
unsigned long rates_per_bplan;

unsigned short get_cdrs_mode;

/* Default Billing Day */
char billing_day[2];

/* Default Payment Day*/
int day_of_payment;

/* */
float k_limit_min;

/* A file descriptor to PIDFile of the application */
int pidFilehandle;

/* Copy a CDR Table */
//cdr_table_t *cdr_tbl_cpy_ptr;

/* Main config dynamic struct */
main_cfg_t *mcfg;

//rate_engine rt_eng;

typedef struct re5_server
{
    int     i;
    int     log;
    int     portno;
    
    char log_filename[128];
    
    pid_t pid;
    time_t pid_ts;
    
    pthread_t thread_cdrstorage_engine;
    time_t cdrstorage_engine_ts;
    
    pthread_t thread_cc_server;
    time_t cc_server_ts;
    
    pthread_t thread_tcp_server;
    time_t tcp_server_ts;
    
    pthread_t thread_cache_engine;
    time_t cache_engine_ts;
    
    pthread_t thread_rate_engine;
    time_t rate_engine_ts;
    
    pthread_mutex_t sync_bt_thread;
    
    pthread_mutex_t sync_ccserver_threads;
    
    pthread_mutex_t sync_crating_threads;
}re5_server;

re5_server config;

#define ERROR_STR_LEN 512
#define ERROR_END 1

typedef struct err {
	int  error_num;
	char error_str[ERROR_STR_LEN];
} err_t;

typedef struct opt_cli {
	char cfgfile[512];

	unsigned short daemon_flag;
    unsigned short kill_flag;
    unsigned short test_flag;
	unsigned short test_mode;
    unsigned short stat_flag;
    unsigned short debug;

    char leg[2];
	char rating_account[80];
	char rating_mode[80];
}opt_cli_t;

/* Command options from CLI */
opt_cli_t opt_cli_mem;

/* modules list , pointer */
mod_t *mod_lst;

#endif
