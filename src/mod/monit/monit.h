#ifndef MONIT_H
#define MONIT_H

#define MONIT_SUBJECT_STR "RE7 MONITOR"

typedef enum monit_type {
	monit_unkw = 0,
	monit_smtp = 1,
	monit_snmp = 2,
	monit_all  = 3
} monit_type_t;

typedef enum monit_level {
	monit_info         = 0,
	monit_warning      = 1,
	monit_error        = 2,
	monit_critical     = 3
} monit_level_t;

typedef struct monit_cfg {
	monit_type_t t;

	/* SMTP settings */
	char smtp_url[256];
	char smtp_username[128];
	char smtp_password[128];
	char smtp_from[128];
	char smtp_to[128];

	/* SNMP settings */
	char snmp_host[128];
	unsigned short snmp_port;
	char snmp_community[64];
} monit_cfg_t;

typedef struct monit_event {
	monit_level_t level;
	char message[512];
	time_t ts;
} monit_event_t;

int monit_init(void);
int monit_free(void);
int monit_send(monit_cfg_t *cfg,monit_event_t *event);
int monit_notify(monit_level_t level,const char *msg);

#endif
