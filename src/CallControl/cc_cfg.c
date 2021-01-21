#include "../misc/globals.h"
#include "../misc/exten/file_list.h"
#include "../misc/mem/mem.h"

#include "cc_cfg.h"

cc_cfg_t *cc_cfg_init(void)
{
	return (cc_cfg_t *)mem_alloc(sizeof(cc_cfg_t));
} 

cc_cfg_int_t *cc_cfg_int_init(int n)
{	
	return (cc_cfg_int_t *)mem_alloc((sizeof(cc_cfg_int_t)*n)+1);
}

void cc_cfg_get_int_xml(cc_cfg_int_t *cfg)
{
	xml_param_t *params = NULL;
		 
	if(cfg != NULL) {
		cfg->node = xml_cfg_node_init();
		strcpy(cfg->node->node_name,"config");
				
		xml_cfg_params_get(cfg->root,cfg->node);

		params = cfg->node->params;

		while(params != NULL) {
			if(strcmp(params->name,"proto") == 0) {
				if(strcmp(params->value,CC_PROTO_MY_CC) == 0) cfg->proto = my_cc;
				else if(strcmp(params->value,CC_PROTO_DIAM) == 0) cfg->proto = diam;
				else if(strcmp(params->value,CC_PROTO_MY_CC_v2) == 0) cfg->proto = my_cc_v2;				
				else cfg->proto = unkn_proto;
			}
			
			if(strcmp(params->name,"ip") == 0) {
				strcpy(cfg->ip,params->value);
			}

			if(strcmp(params->name,"port") == 0) {
				cfg->port = atoi(params->value);
			}
						
			params = params->next_param;
		}
	
		xml_cfg_params_free(cfg->node->params);
	} else {
		re_write_syslog_2(config.log,"cc_cfg_get_int_xml()","cfg is null!");
	}
}

void cc_cfg_get_int(cc_cfg_t *cfg)
{
	int i;
	file_list_t lst;
	
	memset(&lst,0,sizeof(file_list_t));
	
	strcpy(lst.dirname,cfg->cc_int_cfg_dir);
	
	file_list_get(&lst);
	
	cfg->int_number = lst.files_number;
	
	if(cfg->int_number > 0 ) {
		cfg->interfaces = cc_cfg_int_init(cfg->int_number);
	
		if((cfg->interfaces) != NULL) {
			for(i=0;i<lst.files_number;i++) {
				char ff[256];
				bzero(ff,256);
				
				sprintf(ff,"%s%s",lst.dirname,lst.list[i].filename);
				re_write_syslog_2(config.log,"cc_cfg_get_int()","int config file: %s",ff);
				
				cfg->interfaces[i].doc = xml_cfg_doc(ff);
	
				if(cfg->interfaces[i].doc != NULL) {
					cfg->interfaces[i].root = xml_cfg_root(cfg->interfaces[i].doc);
	
					if(cfg->interfaces[i].root != NULL) {
						cc_cfg_get_int_xml(&cfg->interfaces[i]);
					} else {
						re_write_syslog_2(config.log,"cc_cfg_get_int()","A main 'root' pointer is null!");
						free(cfg);
						cfg = NULL;
					}
		
					if(&cfg->interfaces[i] != NULL) {
						mem_free(cfg->interfaces[i].node);
						xml_cfg_free_doc(cfg->interfaces[i].doc);
					}
				} else {
					re_write_syslog_2(config.log,"cc_cfg_get_int()","doc is null");
				}				
			}
		}
	}
	
	if(lst.list != NULL) mem_free(lst.list);	
}

void cc_cfg_get(cc_cfg_t *cfg)
{
	xml_param_t *params = NULL;
	
	if(cfg != NULL) {
		cfg->node = xml_cfg_node_init();
		strcpy(cfg->node->node_name,"CallControl");
	
		xml_cfg_params_get(cfg->root,cfg->node);

		params = cfg->node->params;
	
		while(params != NULL) {
			if(strcmp(params->name,"active") == 0) {
				if(strcmp(params->value,"yes") == 0) cfg->cc_active_flag = 't';
				else cfg->cc_active_flag = 'f';
			}

			if(strcmp(params->name,"IntConfigDIR") == 0) {
				strcpy(cfg->cc_int_cfg_dir,params->value);
			}
			
			if(strcmp(params->name,"LogFile") == 0) {
				strcpy(cfg->cc_logfile,params->value);
			}
			
			if(strcmp(params->name,"LogFileMaxSize") == 0) {
				cfg->cc_log_max_file_size = atoi(params->value);
			}
			
			if(strcmp(params->name,"CallMaxsecLimit") == 0) {
				cfg->call_maxsec_limit = atoi(params->value);
			}
			
			if(strcmp(params->name,"SimCalls") == 0) {
				cfg->sim_calls = atoi(params->value);
			}
			
			if(strcmp(params->name,"CCServerMicroSleep") == 0) {
				cfg->cc_server_usleep = atoi(params->value);
			}
			
			if(strcmp(params->name,"CCMonitoringInterval") == 0) {
				cfg->cc_interval = atoi(params->value);
			}			
			
			params = params->next_param;
		}
	
		xml_cfg_params_free(cfg->node->params);
	}
}

void cc_cfg_params_init(cc_cfg_t *cfg)
{

}

cc_cfg_t *cc_cfg_main(char *xmlFileName)
{
	cc_cfg_t *cfg;
	
	cfg = cc_cfg_init();

	if(cfg != NULL) {
		cfg->doc = xml_cfg_doc(xmlFileName);
	
		if(cfg->doc != NULL) {
			cfg->root = xml_cfg_root(cfg->doc);
	
			if(cfg->root != NULL) {
				cc_cfg_get(cfg);
			} else {
				re_write_syslog_2(config.log,"cc_cfg_main()","A main 'root' pointer is null!");
				free(cfg);
				cfg = NULL;
			}
		
			if(cfg != NULL) {
				mem_free(cfg->node);
				xml_cfg_free_doc(cfg->doc);
			}
		}
				
		if(strcmp(cfg->cc_int_cfg_dir,"") == 0) {
			re_write_syslog_2(config.log,"cc_cfg_main()","A 'IntConfigDIR' is empty!");
			free(cfg);
			cfg = NULL;
		} else {
			cc_cfg_get_int(cfg);
		}
				
		cc_cfg_params_init(cfg);
	}
	
	return cfg;
}
