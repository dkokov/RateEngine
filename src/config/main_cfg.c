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

/* '<param name="active" value="yes|no">' -> 't'/'f'. Anything other than
 * "yes" counts as disabled, matching the module-side cfg readers. */
static char main_cfg_active_val(char *value)
{
	if((value != NULL)&&(strcmp(value,"yes") == 0)) return 't';

	return 'f';
}

/* Read only the core-relevant params from <Rating>: which module runs the
 * offline batch rater and whether the service is active at all. The Rating
 * module still parses the rest of <Rating> (leg, batch limit, pcard/billing
 * options) for its own engine. */
void main_cfg_rating_get(main_cfg_t *cfg)
{
	xml_param_t *params = NULL;

	/* default: the Rating module itself does batch rating */
	strcpy(cfg->rating_module,"rt.so");

	/* no 'active' param -> service is enabled (backward compatible) */
	cfg->rating_active = 't';

	strcpy(cfg->node->node_name,"Rating");

	xml_cfg_params_get(cfg->root,cfg->node);

	params = cfg->node->params;

	while(params != NULL) {
		if(strcmp(params->name,"RatingModule") == 0) {
			if(strlen(params->value) > 0) {
				strncpy(cfg->rating_module,params->value,sizeof(cfg->rating_module)-1);
				cfg->rating_module[sizeof(cfg->rating_module)-1] = '\0';
			}
		}

		if(strcmp(params->name,"active") == 0) {
			cfg->rating_active = main_cfg_active_val(params->value);
		}

		params = params->next_param;
	}

	xml_cfg_params_free(cfg->node->params);
}

/* Core-relevant param from <CallControl>: is the CC server active. The
 * CallControl module reads the rest of the section itself (cc_cfg.c). */
void main_cfg_cc_get(main_cfg_t *cfg)
{
	xml_param_t *params = NULL;

	cfg->cc_active = 't';

	strcpy(cfg->node->node_name,"CallControl");

	xml_cfg_params_get(cfg->root,cfg->node);

	params = cfg->node->params;

	while(params != NULL) {
		if(strcmp(params->name,"active") == 0) {
			cfg->cc_active = main_cfg_active_val(params->value);
		}

		params = params->next_param;
	}

	xml_cfg_params_free(cfg->node->params);
}

/* Core-relevant param from <CDRMediator>: is CDR fetching active. Note the
 * per-profile 'active' in config/cdr_profiles/*.xml is a different switch -
 * this one decides whether the mediator engine is started at all. */
void main_cfg_cdrm_get(main_cfg_t *cfg)
{
	xml_param_t *params = NULL;

	cfg->cdrm_active = 't';

	strcpy(cfg->node->node_name,"CDRMediator");

	xml_cfg_params_get(cfg->root,cfg->node);

	params = cfg->node->params;

	while(params != NULL) {
		if(strcmp(params->name,"active") == 0) {
			cfg->cdrm_active = main_cfg_active_val(params->value);
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
		LOG("main_cfg_view()","active: rating='%c'(%s),callcontrol='%c',cdrmediator='%c'",
			cfg->rating_active,cfg->rating_module,cfg->cc_active,cfg->cdrm_active);
	}
}

main_cfg_t *main_cfg_main(char *cfg_filename)
{
	main_cfg_t *cfg;

	/* Initialise libxml2 once, here on the main thread, before any config XML
	 * is parsed and before subsystem threads start. libxml2 is not thread-safe
	 * without this: CallControl, the rating engine and CDRMediator parse config
	 * concurrently at startup, and the lazy global init would otherwise race.
	 * (Paired with the removal of the per-free xmlCleanupParser().) */
	xmlInitParser();

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

					/* offline batch rating backend selector + 'active' */
					main_cfg_rating_get(cfg);

					/* per-service 'active' switches for re7_starter() */
					main_cfg_cc_get(cfg);
					main_cfg_cdrm_get(cfg);

					mem_free(cfg->node);
				}
			}
			xml_cfg_free_doc(cfg->doc);
		}
	}
	
	return cfg;
}
