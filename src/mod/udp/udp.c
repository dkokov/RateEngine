/* 
 * https://www.geeksforgeeks.org/udp-server-client-implementation-c/
 * 
 * */

#include <unistd.h>
#include <errno.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../net/net.h"
#include "../mod.h"


int udp_open(net_conn_t *conn)
{
	return net_open_socket(conn);
}

void udp_close(net_conn_t *conn)
{
	net_close_socket(conn);
}
/*
int udp_listen(net_conn_t *conn)
{
    struct sockaddr_in serv_addr;

	if(conn == NULL) return NET_ERROR_SOCKET;

	if(conn->sockfd < 0) return NET_ERROR_SOCKET;  
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = conn->ipv;
    
    if(strlen(conn->ip_str)) serv_addr.sin_addr.s_addr = inet_addr(conn->ip_str);
    else serv_addr.sin_addr.s_addr = INADDR_ANY;
	
	if(conn->port > 0) serv_addr.sin_port = htons(conn->port);
	else return NET_ERROR_PORT;
	
    if(setsockopt(conn->sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&conn->opt, sizeof(conn->opt)) < 0 ) {
		return NET_ERROR_SETSOCKOPT;
    }

    if(bind(conn->sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		return NET_ERROR_BIND;
    }
    
	return 0;
}*/

int udp_recv(net_conn_t *conn)
{
	int n,len;
	struct sockaddr_in  servaddr;
	    
	if(conn == NULL) return NET_ERROR_SOCKET;
	
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    servaddr.sin_family = conn->domain; 
    servaddr.sin_port = htons(conn->port); 
    
    if(strlen(conn->ip_str)) servaddr.sin_addr.s_addr = inet_addr(conn->ip_str);
    else servaddr.sin_addr.s_addr = INADDR_ANY;
    
    n = recvfrom(conn->sockfd,conn->buffer,1024,MSG_WAITALL,(struct sockaddr *) &servaddr,&len);	

	return n;
}

int udp_send(net_conn_t *conn)
{
	struct sockaddr_in  servaddr;
	    
	if(conn == NULL) return NET_ERROR_SOCKET;
	
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    servaddr.sin_family = conn->domain; 
    servaddr.sin_port = htons(conn->port); 
    
    if(strlen(conn->ip_str)) servaddr.sin_addr.s_addr = inet_addr(conn->ip_str);
    else servaddr.sin_addr.s_addr = INADDR_ANY;
    
    sendto(conn->sockfd,conn->buffer,strlen(conn->buffer),MSG_CONFIRM,(const struct sockaddr *) &servaddr,sizeof(servaddr)); 
	
	return 0;
}

int udp_serial_server(net_conn_t *conn,int (*external_func)(char *))
{
	int n;
	
	if(conn == NULL) return NET_T_CONN_POINTER_NUL;
	if(conn->conn_num <= 0) conn->conn_num = NET_CONN_NUM;


	net_buffer_init(conn);
	if(conn->buffer == NULL) return -1002;	
	
	while(TRUE) {
		if(conn->sockfd <= 0) {
			LOG("udp_serial_server()","ERROR socket");
			break;
		}
	
		net_clear_buffer(conn);

		udp_recv(conn);

		LOG("udp_serial_server()","udp read buf: %s",conn->buffer);
		
		n = external_func(conn->buffer);
		DBG2("External function return %d",n);
		
		udp_send(conn);
	}
	
	return NET_OK;
}

int udp_bind_api(net_funcs_t *ptr)
{
	if(ptr == NULL) return -1;
		
	ptr->open     = udp_open;
	ptr->close    = udp_close;
	ptr->accept   = NULL;
	ptr->listen   = NULL;
	ptr->recv_msg = udp_recv;
	ptr->send_msg = udp_send;
	ptr->status   = NULL;
	ptr->connect  = NULL;
	
	ptr->s_server = udp_serial_server;
	
	return 0;
}
