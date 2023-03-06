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
	
	unsigned short daemon_flag;
	
	/* Reconnection params for DBs */
	unsigned short num_retries;
	unsigned short int_retries;
}main_cfg_t; 

main_cfg_t *main_cfg_main(char *cfg_filename);
void main_cfg_view(main_cfg_t *cfg);

#endif
