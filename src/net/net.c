/*
 * http://www.linuxhowtos.org/C_C++/socket.htm
 * https://www.ibm.com/support/knowledgecenter/en/SSB23S_1.1.0.14/gtpc1/unixsock.html
 * 
 * */

#include <unistd.h>
#include <errno.h>

#include "../misc/globals.h"
#include "../mod/mod.h"
#include "../mem/mem.h"

#include "net.h"

net_t *net_init(void)
{
	net_t *np;
	
	np = mem_alloc(sizeof(net_t));
	if(np != NULL) {
		np->conn = mem_alloc(sizeof(net_conn_t));
		
		if(np->conn == NULL) {
			mem_free(np);
			return NULL;
		}
		
		np->api = mem_alloc(sizeof(net_funcs_t));
		if(np->api == NULL) {
			mem_free(np->conn);
			mem_free(np);
			return NULL;
		}	
	}
	
	return np;
}

void net_free(net_t *np)
{
	if(np != NULL) {
		if(np->conn != NULL) {
			if(np->conn->buffer != NULL) mem_free(np->conn->buffer);
			
			mem_free(np->conn);
		}
	
		if(np->api != NULL) mem_free(np->api);
	}
}

void net_buffer_init(net_conn_t *conn)
{
	if(conn != NULL) {
		if(conn->buf_size > 0) conn->buffer = mem_alloc(conn->buf_size);
	}
}

void net_buffer_free(net_conn_t *conn)
{
	if(conn != NULL) {
		if(conn->buffer != NULL) {
			mem_free(conn->buffer);
			conn->buffer = NULL;
		}
	}
}

void net_clear_buffer(net_conn_t *conn)
{
	if(conn != NULL) 
		if((conn->buffer != NULL)&&(conn->buf_size > 0)) memset(conn->buffer,0,conn->buf_size);
}

/* http://man7.org/linux/man-pages/man2/socket.2.html */
int net_open_socket(net_conn_t *conn)
{
	int sd;
	int type;
	int protocol;
	
	if(conn == NULL) return NET_ERROR_SOCKET;
	
	protocol = 0;

	switch(conn->proto_id) {
		case udp:
			type = SOCK_DGRAM;
			break;
		case tcp:
			type = SOCK_STREAM;
			break;
		case tls:
			type = SOCK_STREAM;
			break;
		case sctp:
			type = SOCK_STREAM; // SEQPACKET
			protocol = IPPROTO_SCTP;
			break;
		case tls_sctp:
			type = SOCK_STREAM;
			protocol = IPPROTO_SCTP;
			break;
		default:
			return NET_ERROR_SOCKET;
	}

	sd = socket(conn->domain,type,protocol);
	if(sd < 0) return NET_ERROR_SOCKET;
	
	if(conn->t == server) conn->sockfd = sd; 
	else conn->newsockfd = sd;
	
	return NET_OK;
}

int net_close_socket(net_conn_t *conn)
{	
	if(conn == NULL) return NET_ERROR_NOTCONN;
	
	if(conn->t == server) close(conn->sockfd); 
	else close(conn->newsockfd);

	return NET_OK;
}

int net_socket_status(net_conn_t *conn)
{
	int error = 0;
	socklen_t len = sizeof(error);
	
	if(conn == NULL) return -1;
	
	//int retval = 
	getsockopt(conn->sockfd,SOL_SOCKET,SO_ERROR,&error,&len);



	return error;
}

int net_open(net_t *np)
{
	int i,ret;
	net_funcs_t *api;
	
	if(np == NULL) return NET_T_POINTER_NUL;
	if(np->conn == NULL) return NET_T_CONN_POINTER_NUL;
	if(np->api == NULL) return NET_T_API_POINTER_NUL;
	
	api = np->api;
	
	if(api->open == NULL) return NET_T_API_OPEN_P_NUL;
	
	i=0;
	
	loop:
	ret = api->open(np->conn);
	if(ret < 0) {
		LOG("net_open()","ERROR open socket /ret: %d/",ret);
		
		if(i < NET_CONN_REPLY_NUM) {
			i++;
			sleep(NET_CONN_REPLY_INT);
			goto loop;
		} else return NET_ERROR_SOCKET;
	} else return NET_OK;  
}

int net_accept(net_t *np)
{
	net_funcs_t *api;
	
	if(np == NULL) return NET_T_POINTER_NUL;
	if(np->conn == NULL) return NET_T_CONN_POINTER_NUL;
	if(np->api == NULL) return NET_T_API_POINTER_NUL;
	
	api = np->api;
	
	if(api->accept == NULL) return NET_T_API_ACCEPT_P_NUL;
	
	return api->accept(np->conn);
}

/*
int net_listen(net_t *np)
{
	net_funcs_t *api;
	
	if(np == NULL) return -1;
	if(np->conn == NULL) return -2;
	if(np->api == NULL) return -3;
	
	api = np->api;
	
	if(api->listen == NULL) return -4;
	
	return api->listen(np->conn); 
}*/

int net_listen(net_conn_t *conn)
{
	size_t str_sz;
	
	struct sockaddr_in  serv_addr;
	struct sockaddr_in6 serv_addr6;
	struct sockaddr_un  serv_loc;
	
	struct sockaddr *tmp;

	if(conn == NULL) return NET_ERROR_SOCKET;

	if(conn->sockfd < 0) return NET_ERROR_SOCKET;  

	if(conn->domain == ipv4) {
		str_sz = sizeof(serv_addr);
    
		bzero((char *) &serv_addr, sizeof(serv_addr));
    
		serv_addr.sin_family = conn->domain;
    
		if(strlen(conn->ip_str)) serv_addr.sin_addr.s_addr = inet_addr(conn->ip_str);
		else serv_addr.sin_addr.s_addr = INADDR_ANY;
	
		if(conn->port > 0) serv_addr.sin_port = htons(conn->port);
		else return NET_ERROR_PORT;

		tmp = (struct sockaddr *)&serv_addr;
	} else if(conn->domain == ipv6) {
		str_sz = sizeof(serv_addr6);
    
		bzero((char *) &serv_addr6, sizeof(serv_addr6));
    
		serv_addr6.sin6_family = conn->domain;
    
		if(strlen(conn->ip_str)) inet_pton(conn->domain,conn->ip_str,&serv_addr6.sin6_addr); // ? ne raboti
		else serv_addr6.sin6_addr = in6addr_any;
		
		if(conn->port > 0) serv_addr6.sin6_port = htons(conn->port);
		else return NET_ERROR_PORT;

		tmp = (struct sockaddr *)&serv_addr6;		
	} else if(conn->domain == loc) {
		str_sz = sizeof(serv_loc);
		
		bzero((char *) &serv_loc, sizeof(serv_loc));

		serv_loc.sun_family = conn->domain;   
		strcpy(serv_loc.sun_path,conn->unix_socket_filename);

		tmp = (struct sockaddr *)&serv_loc;
	} else return NET_ERROR_BIND;
	
	if(setsockopt(conn->sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&conn->opt, sizeof(conn->opt)) < 0 )
		return NET_ERROR_SETSOCKOPT;
		
	if(bind(conn->sockfd,tmp,str_sz) < 0)
		return NET_ERROR_BIND;			
	
	if(conn->proto_id != udp) {
		if(conn->conn_num <= 0) conn->conn_num = NET_CONN_NUM;
	
		if(listen(conn->sockfd,conn->conn_num) < 0) return NET_ERROR_LISTEN;
	}
	
	return NET_OK;
}

int net_recv_msg(net_t *np)
{
	net_funcs_t *api;
	
	if(np == NULL) return NET_T_POINTER_NUL;
	if(np->conn == NULL) return NET_T_CONN_POINTER_NUL;
	if(np->api == NULL) return NET_T_API_POINTER_NUL;
	
	api = np->api;
	
	if(api->recv_msg == NULL) return NET_T_API_RECV_P_NUL;
	
	return api->recv_msg(np->conn); 
}

int net_send_msg(net_t *np)
{
	net_funcs_t *api;
	
	if(np == NULL) return NET_T_POINTER_NUL;
	if(np->conn == NULL) return NET_T_CONN_POINTER_NUL;
	if(np->api == NULL) return NET_T_API_POINTER_NUL;
	
	api = np->api;
	
	if(api->send_msg == NULL) return NET_T_API_SEND_P_NUL;
	
	return api->send_msg(np->conn); 
}

int net_close(net_t *np)
{
	net_funcs_t *api;
	
	if(np == NULL) return -1;
	if(np->conn == NULL) return -2;
	if(np->api == NULL) return -3;
	
	api = np->api;
	
	if(api->close == NULL) return -4;
	
	api->close(np->conn); 

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
 * 
int net_serial_server(net_t *np,int (*external_func)(char *))
{
	int n,i;
    fd_set readfds;
    int *client_socket;
	int sd,max_sd,activity;

    struct sockaddr_in serv_addr;
    socklen_t clilen;

	if(np == NULL) return NET_T_POINTER_NUL;
	if(np->conn == NULL) return NET_T_CONN_POINTER_NUL;
	if(np->conn->conn_num <= 0) np->conn->conn_num = NET_CONN_NUM;
	if(np->api == NULL) return NET_T_API_POINTER_NUL;

	client_socket = mem_alloc(np->conn->conn_num);
	if(client_socket == NULL) return -1001;

	net_buffer_init(np->conn);
	if(np->conn->buffer == NULL) return -1002;

    for(i = 0; i < np->conn->conn_num; i++) client_socket[i] = 0;
    
	clilen = sizeof(serv_addr);

	while(TRUE) {
		if(np->conn->sockfd <= 0) {
			LOG("net_serial_server()","ERROR socket");
			break;
		}
		
		FD_ZERO(&readfds);

		FD_SET(np->conn->sockfd, &readfds);
		max_sd = np->conn->sockfd;
		
		for(i = 0 ; i < np->conn->conn_num ; i++) {
			sd = client_socket[i];
	
			if(sd > 0) FD_SET( sd , &readfds);
	
			if(sd > max_sd) max_sd = sd;			
		}
		
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		
		if ((activity < 0) && (errno!=EINTR)) {
			LOG("net_serial_server()","ERROR select");
			break;
		}
		
		if (FD_ISSET(np->conn->sockfd, &readfds)) {
			if(np->api->accept(np->conn) < 0) break;

			if(np->conn->newsockfd < 0) break;
			
			getpeername(np->conn->newsockfd,(struct sockaddr*)&serv_addr,(socklen_t*)&clilen);
			LOG("net_serial_server()","New connection,socket fd is %d,host is : %s:%d ",
				np->conn->newsockfd,inet_ntoa(serv_addr.sin_addr),ntohs(serv_addr.sin_port));	
			
			for(i = 0; i < np->conn->conn_num; i++) {
				if( client_socket[i] == 0 ) {
					client_socket[i] = np->conn->newsockfd;
					break;
				}
			}
		}
		
		np->conn->t = client;
		for(i = 0; i < np->conn->conn_num; i++) {
			np->conn->newsockfd = client_socket[i];
	
			if(FD_ISSET(np->conn->newsockfd,&readfds)) {
				net_clear_buffer(np->conn);
				
				n = net_recv_msg(np);
				if (n <= 0) {		
					net_close(np);
					client_socket[i] = 0;
					
					getpeername(np->conn->newsockfd,(struct sockaddr*)&serv_addr,(socklen_t*)&clilen);
					LOG("net_serial_server()","Host disconnected [%s:%d]",inet_ntoa(serv_addr.sin_addr),ntohs(serv_addr.sin_port));
				} else {
					n = external_func(np->conn->buffer);
					DBG2("External function return %d",n);
					
					net_send_msg(np);
				}
			}
		}
	}

	np->conn->t = server;
	net_close(np);
	net_buffer_free(np->conn);

	mem_free(client_socket);
	
	return 0;
}*/

int net_serial_server(net_t *np,int (*external_func)(char *))
{
	if(np == NULL) return NET_T_POINTER_NUL;
	if(np->conn == NULL) return NET_T_CONN_POINTER_NUL;
	if(np->conn->conn_num <= 0) np->conn->conn_num = NET_CONN_NUM;
	if(np->api == NULL) return NET_T_API_POINTER_NUL;
	if(np->api->s_server == NULL) return NET_T_API_SSERV_P_NUL;

	return np->api->s_server(np->conn,external_func);	
}

/* 
 * Parallel Network Server / independent by the proto /;
 * 
 * */
int net_parallel_server(net_t *np)
{
	
	return NET_OK;
}

int net_proto_bind(net_t *np)
{
	int ret;
	void *func;
	mod_t *mod_ptr;
	
	char libname[255];
	char funcname[255];
		
	int (*fptr)(net_funcs_t *);

	if(mod_lst == NULL) return -1;
	
	if(np->api == NULL) return -2;
		
	if(strlen(np->conn->proto) == 0) return -3;
	
	memset(libname,0,sizeof(libname));
	sprintf(libname,"%s.so",np->conn->proto);
	
	memset(funcname,0,sizeof(funcname));
	sprintf(funcname,"%s_bind_api",np->conn->proto);
	
	mod_ptr = mod_find_module(libname);
	if(mod_ptr == NULL) return -4;

	ret = -6;
	
	if(mod_ptr->handle != NULL) {
		func = mod_find_sim(mod_ptr->handle,funcname);		
		if(func == NULL) return -5;
	
		fptr = func;
		ret = fptr(np->api);
	}
	
	return ret;
}

int net_test_ext_func(char *msg)
{
	if((msg != NULL)&&(strlen(msg) > 0)) {
		LOG("net_test_ext_func()","recv_msg: %s",msg);
	} else return NET_ERROR_RECV;
	
	memset(msg,0,strlen(msg));
	strcpy(msg,"{\"jsonrpc\":\"2.0\",\"result\":\"OK\",\"id\":1}");
	
	LOG("net_test_ext_func()","send_msg: %s",msg);
	
	return NET_OK;
}

void net_test(void)
{
	int ret;
	net_t *np;
	
	int (*ext_func)(char *) = net_test_ext_func;
	
	np = net_init();

	np->conn->t = server;
	np->conn->domain   = ipv4;
	np->conn->proto_id = sctp;
	np->conn->port  = 9999;
	np->conn->buf_size = 2048;
	strcpy(np->conn->proto,"sctp");
	//strcpy(np->conn->unix_socket_filename,"/tmp/test_unix_socket.server");
	strcpy(np->conn->ip_str,"127.0.0.1");
	//strcpy(np->conn->ip_str,"::2");
	
	ret = net_proto_bind(np);
	if(ret == NET_OK) {
		ret = net_open(np);
		if(ret == NET_OK) {
			ret = net_listen(np->conn);
			if(ret == NET_OK) {				
				//net_buffer_init(np->conn);
								
				ret = net_serial_server(np,ext_func);
				if(ret == NET_OK) {
					
				} else LOG("net_test() - net_serial_server()","error: %d",ret);
			} else LOG("net_test() - net_listen()","error: %d",ret);
		} else LOG("net_test() - net_open()","error: %d",ret);
	} else LOG("net_test() - net_proto_bind()","error: %d",ret);
	
	net_close(np);
	net_free(np);
}
