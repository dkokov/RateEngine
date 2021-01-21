#include "../misc/globals.h"
#include "../mem/mem.h"

#include "main_cfg.h"

main_cfg_t *main_cfg_init(void)
{
	return (main_cfg_t *)mem_alloc(sizeof(main_cfg_t));	
}

void main_cfg_system_get(main_cfg_t *cfg)
{
	xml_param_t *params = NULL;
	
	strcpy(cfg->node->node_name,"System");
		
	xml_cfg_params_get(cfg->root,cfg->node);
	
	params = cfg->node->params;
	
	while(params != NULL) {
		if(strcmp(params->name,"DIR") == 0) {
			strcpy(cfg->system_dir,params->value);
		}
			
		if(strcmp(params->name,"PIDFile") == 0) {
			strcpy(cfg->system_pid_file,params->value);
		}
			
		params = params->next_param;
	}
	
	xml_cfg_params_free(cfg->node->params);
}

void main_cfg_db_get(main_cfg_t *cfg)
{
	xml_param_t *params = NULL;

	strcpy(cfg->node->node_name,"DB");
	
	xml_cfg_params_get(cfg->root,cfg->node);
		
	params = cfg->node->params;
	
	while(params != NULL) {
		if(strcmp(params->name,"dbtype") == 0) {
			strcpy(cfg->dbtype,params->value);
		}
		
		if(strcmp(params->name,"dbhost") == 0) {
			strcpy(cfg->dbhost,params->value);
		}
			
		if(strcmp(params->name,"dbname") == 0) {
			strcpy(cfg->dbname,params->value);
		}
			
		if(strcmp(params->name,"dbuser") == 0) {
			strcpy(cfg->dbuser,params->value);
		}
			
		if(strcmp(params->name,"dbpass") == 0) {
			strcpy(cfg->dbpass,params->value);
		}
			
		if(strcmp(params->name,"dbport") == 0) {
			cfg->dbport = atoi(params->value);
		}
		
		if(strcmp(params->name,"NumberRetries") == 0) {
			cfg->num_retries = atoi(params->value);
		}

		if(strcmp(params->name,"IntervalRetries") == 0) {
			cfg->int_retries = atoi(params->value);
		}
		
		params = params->next_param;
	}
	
	xml_cfg_params_free(cfg->node->params);	
}

void main_cfg_logs_get(main_cfg_t *cfg)
{
	xml_param_t *params = NULL;

	strcpy(cfg->node->node_name,"Logs");
	
	xml_cfg_params_get(cfg->root,cfg->node);
		
	params = cfg->node->params;
	
	while(params != NULL) {
		if(strcmp(params->name,"LogFile") == 0) {
			strcpy(cfg->logfile,params->value);
		}
			
		if(strcmp(params->name,"LogMaxFileSize") == 0) {
			cfg->log_max_file_size = atoi(params->value);
		}
			
		if(strcmp(params->name,"LogSeparator") == 0) {
			cfg->log_separator = *(params->value);
		}
			
		if(strcmp(params->name,"LogDebugLevel") == 0) {
			cfg->log_debug_level = atoi(params->value);
		}
			
		params = params->next_param;
	}
	
	xml_cfg_params_free(cfg->node->params);
}

void main_cfg_view(main_cfg_t *cfg)
{
	if(cfg != NULL) {
		LOG("main_cfg_view()","cfg filename: %s",cfg->cfg_filename);
		LOG("main_cfg_view()","system dir: %s",cfg->system_dir);
		LOG("main_cfg_view()","system pid file: %s",cfg->system_pid_file);
		LOG("main_cfg_view()","dbtype: %s,dbhost: %s",cfg->dbtype,cfg->dbhost);
	}
}

main_cfg_t *main_cfg_main(char *cfg_filename)
{
	main_cfg_t *cfg;
	
	cfg = main_cfg_init();
	
	if(cfg != NULL) {
		strcpy(cfg->cfg_filename,cfg_filename);
		
		cfg->doc = xml_cfg_doc(cfg_filename);
		
		if(cfg->doc != NULL) {
			cfg->root = xml_cfg_root(cfg->doc);
			
			if(cfg->root != NULL) {
				cfg->node = xml_cfg_node_init();
		
				if(cfg->node != NULL) {
					/* SYSTEM */
					main_cfg_system_get(cfg);
					
					/* Local DB */
					main_cfg_db_get(cfg);
		
					/* Logs */
					main_cfg_logs_get(cfg);
		
					mem_free(cfg->node);
				}
			}
			xml_cfg_free_doc(cfg->doc);
		}
	}
	
	return cfg;
}
