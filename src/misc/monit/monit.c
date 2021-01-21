/* 
 * gcc -Wall -c monit.c -I/usr/include/libxml2
 * gcc -g -o monit smtp/send_email.c monit.c  -I/usr/include/libxml2 -lpq -lcurl -L ../../ -lre6core -lcdrmediator -lrating -lre6cc
 * valgrind --leak-check=full -s ./monit
 * */

#include "../globals.h"
#include "../mem/mem.h"
#include "../../config/xml_cfg.h"

#include "smtp/send_email.h"
#include "snmp/snmp_defs.h"
#include "monit.h"

void monit_cfg_get(monit_cfg_t *cfg)
{
	xml_param_t *params = NULL;
	
	if(cfg != NULL) {
		cfg->node = xml_cfg_node_init();
		strcpy(cfg->node->node_name,"Monitoring");
		
		xml_cfg_params_get(cfg->root,cfg->node);
		
		params = cfg->node->params;
		
		while(params != NULL) {
			if(strcmp(params->name,"type") == 0) {
				if(strcmp(params->value,"smtp") == 0) {
					cfg->t = smtp;
					cfg->smtp = (send_email_t *)mem_alloc(sizeof(send_email_t));
				} else if(strcmp(params->value,"snmp") == 0) { 
					cfg->t = snmp;
					cfg->snmp = (snmp_cfg_t *)mem_alloc(sizeof(snmp_cfg_t));
				} else cfg->t = unkw;
			}
			
			if(strcmp(params->name,"username") == 0) {
				if(cfg->smtp) {
					strcpy(cfg->smtp->username,params->value);
				}
			}
			
			if(strcmp(params->name,"password") == 0) {
				if(cfg->smtp) {
					strcpy(cfg->smtp->password,params->value);
				}
			}
			
			if(strcmp(params->name,"smtpURL") == 0) {
				if(cfg->smtp) {
					strcpy(cfg->smtp->smtp_url,params->value);
				}
			}
			
			if(strcmp(params->name,"From") == 0) {
				if(cfg->smtp) {
					strcpy(cfg->smtp->from,params->value);
				}
			}
			
			if(strcmp(params->name,"To") == 0) {
				if(cfg->smtp) {
					strcpy(cfg->smtp->to,params->value);
				}
			}
			
			params = params->next_param;
		}
	
		xml_cfg_params_free(cfg->node->params);
	}
}

monit_cfg_t *monit_cfg_main(char *xmlFileName)
{
	monit_cfg_t *cfg = NULL;
	
	cfg = (monit_cfg_t *)mem_alloc(sizeof(monit_cfg_t));
	if(cfg != NULL) {
		cfg->doc = xml_cfg_doc(xmlFileName);
		
		if(cfg->doc != NULL) {
			cfg->root = xml_cfg_root(cfg->doc);
			
			if(cfg->root != NULL) {
				monit_cfg_get(cfg); 
			} else {
				LOG("monit_cfg_main()","A main 'root' pointer is null!");
				free(cfg);
				cfg = NULL;
			}
			
			if(cfg != NULL) {
				mem_free(cfg->node);
				xml_cfg_free_doc(cfg->doc);
			}
		}
	}
	
	return cfg;
}

int monit_main(monit_cfg_t *cfg,monit_info_t *info)
{
	if(cfg == NULL) {
		LOG("monit_main()","Error!'cfg' is NULL!");
		return 1;
	}
	
	if(info == NULL) {
		LOG("monit_main()","Error!'info' is NULL!");
		return 1;
	}
	
	/* Get Alarms and send to email or snmpd */
	if((cfg->t == smtp)||(cfg->t == all_m)) {
		if(cfg->smtp == NULL) {
			LOG("monit_main()","Error!'cfg->smtp' is NULL!");
			return 2;
		}
		
		if(info->t == notifications) {
			if(info->ptr == NULL) {
				LOG("monit_main()","Error!'info->ptr' is NULL!");
				return 3;
			}
			
			strcpy(cfg->smtp->subject,MONIT_SUBJECT_STR);
			
			send_email_src_msg_init(cfg->smtp);
			
			strcpy(cfg->smtp->src_msg[7],(char *)info->ptr);
			
			send_email_run(cfg->smtp);
		}
	}
	
	/* Get Status&Statistics&Notifications and save to snmp mibs tree */
	if((cfg->t == snmp)||(cfg->t == all_m)) {
		if(cfg->snmp == NULL ) return 2;
	}
	
	return 0;
}

void monit_cfg_free(monit_cfg_t *cfg)
{
	if(cfg != NULL) {
		if(cfg->smtp != NULL) mem_free(cfg->smtp);
		if(cfg->snmp != NULL) mem_free(cfg->snmp);
		mem_free(cfg);
	}
}

/*
int main(void)
{
	char msg[] = "Test message from DKokov ...";
	monit_cfg_t *cfg = NULL;
	monit_info_t info = {0};
	
	cfg = monit_cfg_main("../../config/samples/RateEngine6.xml");
	if(cfg) {
		info.t = notifications;
		info.ptr = (void *)msg;
		
		monit_main(cfg,&info);
		
		monit_cfg_free(cfg);
	}
	
	return 0;
}
*/
