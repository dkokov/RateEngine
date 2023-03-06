/* 
 * https://gist.github.com/jirihnidek/388271b57003c043d322 
 * https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
 * 
 * */

#include <unistd.h>
#include <errno.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../net/net.h"
#include "../mod.h"


int tcp_open(net_conn_t *conn)
{
	return net_open_socket(conn);
}

void tcp_close(net_conn_t *conn)
{
	net_close_socket(conn);
}
/*
int tcp_listen(net_conn_t *conn)
{
	size_t str_sz;
	struct sockaddr_in serv_addr;
	struct sockaddr_in6 serv_addr6;
	struct sockaddr *tmp;

	if(conn == NULL) return NET_ERROR_SOCKET;

	if(conn->sockfd < 0) return NET_ERROR_SOCKET;  

	if(conn->ipv == ipv4) {
		str_sz = sizeof(serv_addr);
    
		bzero((char *) &serv_addr, sizeof(serv_addr));
    
		serv_addr.sin_family = conn->ipv;
    
		if(strlen(conn->ip_str)) serv_addr.sin_addr.s_addr = inet_addr(conn->ip_str);
		else serv_addr.sin_addr.s_addr = INADDR_ANY;
	
		if(conn->port > 0) serv_addr.sin_port = htons(conn->port);
		else return NET_ERROR_PORT;

		tmp = (struct sockaddr *)&serv_addr;
	} else if(conn->ipv == ipv6) {
		str_sz = sizeof(serv_addr6);
    
		bzero((char *) &serv_addr6, sizeof(serv_addr6));
    
		serv_addr6.sin6_family = conn->ipv;
    
//		if(strlen(conn->ip_str)) serv_addr6.sin6_addr = inet_addr(conn->ip_str);
//		else 
serv_addr6.sin6_addr = in6addr_any;
		
		if(conn->port > 0) serv_addr6.sin6_port = htons(conn->port);
		else return NET_ERROR_PORT;

		tmp = (struct sockaddr *)&serv_addr6;		
	} else return NET_ERROR_BIND;
	
	if(setsockopt(conn->sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&conn->opt, sizeof(conn->opt)) < 0 )
		return NET_ERROR_SETSOCKOPT;
		
	if(bind(conn->sockfd,tmp,str_sz) < 0)
		return NET_ERROR_BIND;			
	
	if(conn->conn_num <= 0) conn->conn_num = NET_CONN_NUM;
	
    if(listen(conn->sockfd,conn->conn_num) < 0) return NET_ERROR_LISTEN;
    
	return NET_OK;
}*/

int tcp_accept(net_conn_t *conn)
{
    socklen_t clilen;
	struct sockaddr_in cli_addr;

    clilen = sizeof(cli_addr);

	if(conn == NULL) return NET_ERROR_SOCKET;

	conn->newsockfd = accept(conn->sockfd,(struct sockaddr *) &cli_addr,&clilen);
		
	if(conn->newsockfd < 0) return NET_ERROR_ACCEPT;
		
	return NET_OK;
}

int tcp_recv(net_conn_t *conn)
{
	int n;
	
	if(conn == NULL) return NET_ERROR_SOCKET;
	
	if(conn->t == server) n = recv(conn->sockfd,conn->buffer,conn->buf_size,0);
	else n = recv(conn->newsockfd,conn->buffer,conn->buf_size,0);
	
	if(n < 0) return NET_ERROR_RECV;
	
	return n;
}

int tcp_send(net_conn_t *conn)
{
	int n;
	
	if(conn == NULL) return NET_ERROR_SOCKET;
	
	if(conn->t == server) n = send(conn->sockfd,conn->buffer,conn->buf_size,0);
	else n = send(conn->newsockfd,conn->buffer,conn->buf_size,0);
	
	if(n < 0) return NET_ERROR_SEND;
	
	return n;
}

int tcp_socket_status(net_conn_t *conn)
{
	int error = 0;
	socklen_t len = sizeof(error);
	
	if(conn == NULL) return NET_ERROR_SOCKET;
	
	//int retval = 
	getsockopt(conn->sockfd,SOL_SOCKET,SO_ERROR,&error,&len);

	//if(retval != 0) {
		/* there was a problem getting the error code */
	//	fprintf(stderr, "error getting socket error code: %s\n", strerror(retval));
	//}

	//if(error != 0) {
		/* socket has a non zero error status */
	//	fprintf(stderr, "socket error: %s %d\n", strerror(error),error);
	//}

	return error;
}

int tcp_client_connect(net_conn_t *conn)
{
	struct sockaddr_in serv_addr;
    struct hostent *server;
 
	if(conn == NULL) return NET_ERROR_SOCKET;
    
    conn->sockfd = socket(conn->domain, SOCK_STREAM, 0);
    if(conn->sockfd < 0) return NET_ERROR_SOCKET;
	
    server = gethostbyname(conn->hostname);
    if(server == NULL) return NET_ERROR_GETHOST;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = conn->domain;
    
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    
    serv_addr.sin_port = htons(conn->port);
    
    if(connect(conn->sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		return NET_ERROR_CONNECT;

    return NET_OK;
}

/* 
 * Serial Network Server / independent by the proto /;
 * 
 * external_func(char *buffer) :
 * have to read buffer (1),
 * make same working with these data (2) and 
 * write in the buffer same response (3)
 * 
 * */
int tcp_serial_server(net_conn_t *conn,int (*external_func)(char *))
{
	int n,i;
    fd_set readfds;
    int *client_socket;
	int sd,max_sd,activity;

    struct sockaddr_in serv_addr;
    socklen_t clilen;

	if(conn == NULL) return NET_T_CONN_POINTER_NUL;
	if(conn->conn_num <= 0) conn->conn_num = NET_CONN_NUM;

	client_socket = mem_alloc(conn->conn_num);
	if(client_socket == NULL) return -1001;

	net_buffer_init(conn);
	if(conn->buffer == NULL) return -1002;

    for(i = 0; i < conn->conn_num; i++) client_socket[i] = 0;
    
	clilen = sizeof(serv_addr);

	while(TRUE) {
		if(conn->sockfd <= 0) {
			LOG("tcp_serial_server()","ERROR socket");
			break;
		}
		
		FD_ZERO(&readfds);

		FD_SET(conn->sockfd, &readfds);
		max_sd = conn->sockfd;
		
		for(i = 0 ; i < conn->conn_num ; i++) {
			sd = client_socket[i];
	
			if(sd > 0) FD_SET( sd , &readfds);
	
			if(sd > max_sd) max_sd = sd;			
		}
		
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		
		if ((activity < 0) && (errno!=EINTR)) {
			LOG("net_serial_server()","ERROR select");
			break;
		}
		
		if (FD_ISSET(conn->sockfd, &readfds)) {
			if(tcp_accept(conn) < 0) break;

			if(conn->newsockfd < 0) break;
			
			getpeername(conn->newsockfd,(struct sockaddr*)&serv_addr,(socklen_t*)&clilen);
			LOG("net_serial_server()","New connection,socket fd is %d,host is : %s:%d ",
				conn->newsockfd,inet_ntoa(serv_addr.sin_addr),ntohs(serv_addr.sin_port));	
			
			for(i = 0; i < conn->conn_num; i++) {
				if( client_socket[i] == 0 ) {
					client_socket[i] = conn->newsockfd;
					break;
				}
			}
		}
		
		conn->t = client;
		for(i = 0; i < conn->conn_num; i++) {
			conn->newsockfd = client_socket[i];
	
			if(FD_ISSET(conn->newsockfd,&readfds)) {
				net_clear_buffer(conn);
				
				n = tcp_recv(conn);
				if (n <= 0) {		
					tcp_close(conn);
					client_socket[i] = 0;
					
					getpeername(conn->newsockfd,(struct sockaddr*)&serv_addr,(socklen_t*)&clilen);
					LOG("net_serial_server()","Host disconnected [%s:%d]",inet_ntoa(serv_addr.sin_addr),ntohs(serv_addr.sin_port));
				} else {
					n = external_func(conn->buffer);
					DBG2("External function return %d",n);
					
					tcp_send(conn);
				}
			}
		}
	}

	conn->t = server;
	tcp_close(conn);
	net_buffer_free(conn);

	mem_free(client_socket);
	
	return 0;
}

int tcp_bind_api(net_funcs_t *ptr)
{
	if(ptr == NULL) return -1;
		
	ptr->open     = tcp_open;
	ptr->close    = tcp_close;
	ptr->accept   = tcp_accept;
	ptr->listen   = NULL;
	
	ptr->recv_msg = tcp_recv;
	ptr->send_msg = tcp_send;
	
	ptr->status   = tcp_socket_status;
	ptr->connect  = tcp_client_connect;
	
	ptr->s_server = tcp_serial_server;
	
	return 0;
}
