/*
 * https://www.ibm.com/developerworks/library/l-sctp/
 * 
 * */

#include <unistd.h>
#include <errno.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../net/net.h"
#include "../mod.h"

#include <netinet/sctp.h>

int sctp_open(net_conn_t *conn)
{
	return net_open_socket(conn);
}

void sctp_close(net_conn_t *conn)
{
	net_close_socket(conn);
}

int sctp_serial_server(net_conn_t *conn,int (*external_func)(char *))
{
	
	
	return NET_OK;
}

int sctp_bind_api(net_funcs_t *ptr)
{
	if(ptr == NULL) return -1;
		
	ptr->open     = sctp_open;
	ptr->close    = sctp_close;
	ptr->accept   = NULL;
	ptr->listen   = NULL;
	ptr->recv_msg = NULL;
	ptr->send_msg = NULL;
	ptr->status   = NULL;
	ptr->connect  = NULL;
	
	ptr->s_server = sctp_serial_server;
	
	return 0;
}
