#include <time.h>
#include <string.h>

#include "../mod.h"
#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../config/xml_cfg.h"

#include "monit.h"

static monit_cfg_t *monit_cfg;

int monit_init(void);
int monit_free(void);

mod_dep_t monit_mod_dep[] = {
	{"",0,0}
};

mod_t monit_mod_t = {
	.mod_name = "Monitoring",
	.ver      = 1,
	.init     = monit_init,
	.destroy  = monit_free,
	.depends  = monit_mod_dep,
	.handle   = NULL,
	.next     = NULL
};

int monit_cfg_load(monit_cfg_t *cfg,char *xmlFileName)
{
	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
	xml_param_t *params;

	if(cfg == NULL) return RE_ERROR;
	if(xmlFileName == NULL) return RE_ERROR;

	doc = xml_cfg_doc(xmlFileName);
	if(doc == NULL) return RE_ERROR;

	root = xml_cfg_root(doc);
	if(root == NULL) {
		xml_cfg_free_doc(doc);
		return RE_ERROR;
	}

	node = xml_cfg_node_init();
	if(node == NULL) {
		xml_cfg_free_doc(doc);
		return RE_ERROR;
	}

	strcpy(node->node_name,"Monitoring");
	xml_cfg_params_get(root,node);

	params = node->params;
	while(params != NULL) {
		if(strcmp(params->name,"type") == 0) {
			if(strcmp(params->value,"smtp") == 0) cfg->t = monit_smtp;
			else if(strcmp(params->value,"snmp") == 0) cfg->t = monit_snmp;
			else if(strcmp(params->value,"all") == 0) cfg->t = monit_all;
			else cfg->t = monit_unkw;
		}

		if(strcmp(params->name,"smtpURL") == 0) strcpy(cfg->smtp_url,params->value);
		if(strcmp(params->name,"username") == 0) strcpy(cfg->smtp_username,params->value);
		if(strcmp(params->name,"password") == 0) strcpy(cfg->smtp_password,params->value);
		if(strcmp(params->name,"From") == 0) strcpy(cfg->smtp_from,params->value);
		if(strcmp(params->name,"To") == 0) strcpy(cfg->smtp_to,params->value);

		if(strcmp(params->name,"snmpHost") == 0) strcpy(cfg->snmp_host,params->value);
		if(strcmp(params->name,"snmpPort") == 0) cfg->snmp_port = atoi(params->value);
		if(strcmp(params->name,"snmpCommunity") == 0) strcpy(cfg->snmp_community,params->value);

		params = params->next_param;
	}

	xml_cfg_params_free(node->params);
	mem_free(node);
	xml_cfg_free_doc(doc);

	return RE_SUCCESS;
}

int monit_send_smtp(monit_cfg_t *cfg,monit_event_t *event)
{
	/* TODO: implement SMTP send via libcurl */
	LOG("monit_send_smtp()","[%d] %s",event->level,event->message);

	return RE_SUCCESS;
}

int monit_send_snmp(monit_cfg_t *cfg,monit_event_t *event)
{
	/* TODO: implement SNMP trap send */
	LOG("monit_send_snmp()","[%d] %s",event->level,event->message);

	return RE_SUCCESS;
}

int monit_send(monit_cfg_t *cfg,monit_event_t *event)
{
	if(cfg == NULL) return RE_ERROR;
	if(event == NULL) return RE_ERROR;

	if((cfg->t == monit_smtp)||(cfg->t == monit_all)) {
		monit_send_smtp(cfg,event);
	}

	if((cfg->t == monit_snmp)||(cfg->t == monit_all)) {
		monit_send_snmp(cfg,event);
	}

	return RE_SUCCESS;
}

int monit_notify(monit_level_t level,const char *msg)
{
	monit_event_t event;

	if(monit_cfg == NULL) return RE_ERROR;
	if(msg == NULL) return RE_ERROR;

	memset(&event,0,sizeof(monit_event_t));
	event.level = level;
	event.ts = time(NULL);
	strncpy(event.message,msg,sizeof(event.message)-1);

	return monit_send(monit_cfg,&event);
}

int monit_init(void)
{
	monit_cfg = (monit_cfg_t *)mem_alloc(sizeof(monit_cfg_t));
	if(monit_cfg == NULL) return RE_ERROR;

	memset(monit_cfg,0,sizeof(monit_cfg_t));

	if(mcfg != NULL) {
		if(monit_cfg_load(monit_cfg,mcfg->cfg_filename) < 0) {
			LOG("monit_init()","monitoring config not found or invalid - disabled");
			monit_cfg->t = monit_unkw;
		}
	}

	LOG("monit_init()","monitoring module loaded, type: %d",monit_cfg->t);

	return RE_SUCCESS;
}

int monit_free(void)
{
	if(monit_cfg != NULL) {
		mem_free(monit_cfg);
		monit_cfg = NULL;
	}

	return RE_SUCCESS;
}
