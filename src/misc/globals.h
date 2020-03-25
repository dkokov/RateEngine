#ifndef GLOBALS_H
#define GLOBALS_H

/* Default path to the config file */
#define CONF "/usr/local/RateEngine/config/RateEngine6.xml"

/* Default log manager pause(seconds) - time interval waiting */
#define LOG_MNG_PAUSE 2

#define LOOP_PAUSE 2

/* Default log max file size(bytes) */
#define LOG_MAX_FILE_SIZE 20480000

#include "../config/main_cfg.h"
#include "../syslog/rt_log.h"

#include "../CDRMediator/cdr.h"
#include "../Rating/rating.h"

#define RE_SUCCESS  0
#define RE_ERROR    1
#define RE_ERROR_N -1

#if !defined(TRUE)
    #define TRUE 1
#endif

#if !defined(FALSE)
	#define FALSE 0
#endif

/* Logging Level */
unsigned short log_debug_level;

char log_separator;

unsigned int log_max_file_size;

/* A flag for demonize of the RE5 */
int demonize;

/* A flag for CDRMediator starting */
unsigned short get_cdrs_flag;

//char cdr_storage_sched_ts[64];

/* A flag for RatingEngine starting */
unsigned short rating_flag;

//unsigned int console_call_id;

/* A flag for CallControl(ccserver) starting */
unsigned short call_control_flag;

//unsigned int caching_refresh;

/* Start/Stop RE5 Manager */
char loop_flag;

unsigned int call_maxsec_limit;
unsigned int sim_calls;
unsigned int cdr_tcp_conn;
unsigned long rates_per_bplan;

/* Default Billing Day */
char billing_day[2];

/* Default Payment Day*/
int day_of_payment;

/* */
float k_limit_min;

/* A file descriptor to PIDFile of the application */
int pidFilehandle;

/* Copy a CDR Table */
cdr_table_t *cdr_tbl_cpy_ptr;

/* Main config dynamic struct - XML config file */
main_cfg_t *mcfg;

rate_engine rt_eng;

typedef struct re5_server {
    int     log;
    PGconn  *conn;
    
    char log_filename[128]; 
} re5_server_t;

re5_server_t config;

/* Inserting options by CLI */
typedef struct opt_cli {

	char cfgfile[512];

	unsigned short daemon_flag;
    unsigned short kill_flag;
    unsigned short test_flag;
    
    unsigned short stat_flag;
    
    char leg[2];

} opt_cli_t;

opt_cli_t opt_cli_mem;

#endif
