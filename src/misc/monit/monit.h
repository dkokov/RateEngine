#ifndef MONIT_H
#define MONIT_H

#define MONIT_SUBJECT_STR "RE6 MONITOR SEND EMAIL"

typedef enum monit_type {
	unkw   = 0,
	smtp   = 1,
	snmp   = 2,
	all_m  = 3
} monit_type_t;

typedef enum monit_type_info {
	unkw_info     = 0,
	status        = 1,
	statistics    = 2,
	notifications = 3
} monit_type_info_t;

typedef struct _monit_cfg {
	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
	
	monit_type_t t;
	
	send_email_t *smtp;
	snmp_cfg_t *snmp;
} monit_cfg_t;

typedef struct monit_info {
	monit_type_info_t t;
	
	void *ptr;
} monit_info_t;

monit_cfg_t *monit_cfg_main(char *xmlFileName);
void monit_cfg_free(monit_cfg_t *cfg);
int monit_main(monit_cfg_t *cfg,monit_info_t *info);

#endif
