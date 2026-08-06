#ifndef MAIN_CFG_H
#define MAIN_CFG_H

#include "xml_cfg.h"

typedef struct main_cfg {
	
	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
	
	/* main cfg filename */
	char cfg_filename[512];
	
	/* System */
	char system_dir[255];
	char system_pid_file[255];

    /* Local DB */
    char dbtype[32];
    char dbhost[255];
    char dbname[64];
    char dbuser[64];
    char dbpass[64];
    int dbport;
	
	/* Logs */
    char logfile[255];
    char log_separator;
    short log_debug_level;
    unsigned int log_max_file_size;
	
	/* set only when the process actually forked into the background (-d);
	 * "am I a long-running supervisor?" is the global 'run_mode' instead */
	unsigned short daemon_flag;

	/* mirror of the global 'run_mode' so modules can see it without pulling
	 * in globals.h (RUN_ONESHOT=0 / RUN_SERVICE=1) */
	unsigned short run_mode;

	/* Reconnection params for DBs */
	unsigned short num_retries;
	unsigned short int_retries;

	/* offline batch rating backend module (<Rating> RatingModule);
	 * default "rt.so" (Rating). Set "rt_duckdb.so" to run the DuckDB batch
	 * rater instead - CallControl still binds rt.so for online charging. */
	char rating_module[64];

	/* Per-service '<param name="active">' switches, read from <Rating>,
	 * <CallControl> and <CDRMediator>. 't' = start the service in service mode
	 * (-d/-f), 'f' = do not. Absent param defaults to 't' (start it), so old
	 * configs keep working. Explicit CLI service flags (-g/-r/-2c) always win.
	 * The core needs these so re7_starter() can decide what to launch; each
	 * module also still reads its own 'active' for its internal loop. */
	char rating_active;
	char cc_active;
	char cdrm_active;
}main_cfg_t;

main_cfg_t *main_cfg_main(char *cfg_filename);
void main_cfg_view(main_cfg_t *cfg);

#endif
