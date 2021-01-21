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
	char system_db_version[255];

    /* Local DB */
    char dbhost[255];
    char dbname[64];
    char dbuser[64];
    char dbpass[64];
    int dbport;
    char db[512];
	
	/* Logs */
    char logfile[255];
    char log_separator;
    short log_debug_level;
    unsigned int log_max_file_size;
	
	unsigned short daemon_flag;
	
	/* Reconnection params for DBs */
	unsigned short num_retries;
	unsigned short int_retries;
}main_cfg_t; 

main_cfg_t *main_cfg_main(char *cfg_filename);

#endif
