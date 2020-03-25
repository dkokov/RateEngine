/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2011, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Dimitar Kokov <d_kokov@abv.bg>
 *
 * mod_call_control.c -- The 'Call Control' module
 * version 0.2 / 2012-08-24 /
 *
 * version 0.3 / 2014-11-18 /
 *
 * version 0.3.1 / 2015-03-17 /
 *
 * version 0.3.2 / 2015-04-02 / , fs-version 1.4
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include <sys/stat.h> 
#include <switch.h>

static struct {
	switch_memory_pool_t *pool;
	switch_hash_t *fd_hash;
	switch_hash_t *template_hash;
	char tcp_server_ip[16];
	char command[32];
	char get_uuid_mode;
	int tcp_server_port;
	int tcp_conn_mode;
	int ccserver_id;
	int sockfd;
	int maxsec;
	
	char file_err_st_0[256];
	char file_err_st_1[256];
	char file_err_st_2[256];
	char file_err_st_3[256];
	char file_err_st_4[256];
	char file_err_st_5[256];
	char file_err_st_6[256];
	char file_err_st_7[256];
	char file_err_st_8[256];
	
	switch_mutex_t *db_mutex;
 } globals = { 0 };

/* Prototypes */
int tcp_client(char *arg3)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    portno = globals.tcp_server_port;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERROR opening socket!\n");
	return 0;
    }
//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%s\n",globals.tcp_server_ip);
    server = gethostbyname(globals.tcp_server_ip);
    if (server == NULL) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERROR,no such host!\n");
        return 0;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);

    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
    
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERROR connecting!\n");        
        return 0;
    }

    bzero(buffer,256);
    strcpy(buffer,arg3);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERROR writing to socket!\n");
         return 0;
    }

    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERROR reading from socket!\n");
         return 0;
    }
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "%s\n",buffer);
    close(sockfd);

    return atoi(buffer);
}

SWITCH_STANDARD_APP(call_control_function)
{
	char str[512];
	char res[32];
	switch_channel_t *channel = switch_core_session_get_channel(session);

	const char *uuid = NULL;
	const char *file = NULL;
	int maxsec = 0;

	if(globals.get_uuid_mode == 't') uuid = switch_channel_get_variable(channel, "uuid");
	/* 
	else ???
	*/
	if (zstr(data)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "No DATA from the call_control function!!!\n");
	} else {
		sprintf(str,"%d,%s,%s,%s",globals.ccserver_id,globals.command,data,uuid);
		maxsec = tcp_client(str);
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE, "\n\n\ndata='%s' \n\n\n",str);
		sprintf(res,"%d",maxsec); 
		switch_channel_set_variable(channel,"maxsec",res);
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE,"The CallControl checking is done %s => maxsec = %d sec.!!!\n",
		str,maxsec);
	}
	
/*	if(maxsec == 0)
	{
	    switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
	    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE,"The CallControl hanguped your call!\n");
	} */
	
	if(maxsec <= 0)
	{
	    switch(maxsec)
	    {
		case 0:
			file = globals.file_err_st_0;
			break;
		case -1:
			file = globals.file_err_st_1;
			break;
		case -2:
			file = globals.file_err_st_2;
			break;
		case -3:
			file = globals.file_err_st_3;
			break;
		case -4:
			file = globals.file_err_st_4;
			break;
		case -5:
			file = globals.file_err_st_5;
			break;
		case -6:
			file = globals.file_err_st_6;
			break;
		case -7:
			file = globals.file_err_st_7;
			break;
		case -8:
			file = globals.file_err_st_8;
			break;
	    }
	
	    switch_ivr_play_file(session,NULL,file,NULL);
	    switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
	    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE,"The CallControl hanguped your call!\n");
	}
}

static switch_status_t load_config(switch_memory_pool_t *pool)
{
    char *cf = "call_control.conf.xml";
    switch_xml_t cfg, xml, settings, param;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    memset(&globals, 0, sizeof(globals));
    switch_core_hash_init(&globals.fd_hash);
    switch_core_hash_init(&globals.template_hash);
    switch_mutex_init(&globals.db_mutex, SWITCH_MUTEX_NESTED, pool);

    globals.pool = pool;

    if ((xml = switch_xml_open_cfg(cf, &cfg, NULL))) {

    if ((settings = switch_xml_child(cfg, "settings"))) {
	for (param = switch_xml_child(settings, "param"); param; param = param->next) {
	    char *var = (char *) switch_xml_attr_soft(param, "name");
	    char *val = (char *) switch_xml_attr_soft(param, "value");
	    if (!strcasecmp(var, "tcp-server-ip")) {
					    strcpy(globals.tcp_server_ip,val);
					    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.tcp_server_ip);
	    }
	    if (!strcasecmp(var, "tcp-server-port")) {
					    globals.tcp_server_port = atoi(val);
					    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %d\n", var,globals.tcp_server_port);
	    }
	    if (!strcasecmp(var, "tcp-conn-mode")) {
					    if(!(strcmp(val,"single"))) globals.tcp_conn_mode = 1;
					    else globals.tcp_conn_mode = 2;
					    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %d\n", var,globals.tcp_conn_mode);
	    }
	    if (!strcasecmp(var, "call-control-srv")) {
					    globals.ccserver_id = atoi(val);
					    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %d\n", var,globals.ccserver_id);
	    }
	    if (!strcasecmp(var, "auto-command")) {
					    strcpy(globals.command,val);
					    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.command);
	    }
	    if (!strcasecmp(var, "get-uuid-mode")) {
					    if(!(strcmp(val,"true"))) globals.get_uuid_mode = 't';
					    else globals.get_uuid_mode = 'f';
					    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %c\n", var,globals.get_uuid_mode);
	    }
            if (!strcasecmp(var, "af_err_state_0")) {
                                            strcpy(globals.file_err_st_0,val);
                                            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.file_err_st_0);
            }
            if (!strcasecmp(var, "af_err_state_1")) {
                                            strcpy(globals.file_err_st_1,val);
                                            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.file_err_st_1);
            }
            if (!strcasecmp(var, "af_err_state_2")) {
                                            strcpy(globals.file_err_st_2,val);
                                            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.file_err_st_2);
            }
            if (!strcasecmp(var, "af_err_state_3")) {
                                            strcpy(globals.file_err_st_3,val);
                                            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.file_err_st_3);
            }
            if (!strcasecmp(var, "af_err_state_4")) {
                                            strcpy(globals.file_err_st_4,val);
                                            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.file_err_st_4);
            }
            if (!strcasecmp(var, "af_err_state_5")) {
                                            strcpy(globals.file_err_st_5,val);
                                            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.file_err_st_5);
            }
            if (!strcasecmp(var, "af_err_state_6")) {
                                            strcpy(globals.file_err_st_6,val);
                                            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.file_err_st_6);
            }
            if (!strcasecmp(var, "af_err_state_7")) {
                                            strcpy(globals.file_err_st_7,val);
                                            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.file_err_st_7);
            }
            if (!strcasecmp(var, "af_err_state_8")) {
                                            strcpy(globals.file_err_st_8,val);
                                            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Parsing of the '%s' => %s\n", var,globals.file_err_st_8);
            }
        }
    }
  }

  switch_xml_free(xml);

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "The call_control.conf.xml was loaded!\n");
  return status;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_call_control_shutdown);
SWITCH_MODULE_LOAD_FUNCTION(mod_call_control_load);

SWITCH_MODULE_DEFINITION(mod_call_control, mod_call_control_load, mod_call_control_shutdown, NULL);

SWITCH_MODULE_LOAD_FUNCTION(mod_call_control_load)
{

    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_application_interface_t *app_interface;
    load_config(pool);

    *module_interface = switch_loadable_module_create_module_interface(pool, modname);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Loading mod_call_control ...!\n");
	
    SWITCH_ADD_APP(app_interface, "call_control", "CallControl", "CallControl from Dimitar Kokov", 
	           call_control_function, "",SAF_SUPPORT_NOMEDIA);
	
    switch_core_set_variable("switch_call_control_flag", "true");

    return status;
}


SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_call_control_shutdown)
{
	switch_core_set_variable("switch_call_control_flag", "false");
	return SWITCH_STATUS_SUCCESS;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4
 */
