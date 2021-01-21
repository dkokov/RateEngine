#ifndef TCP_H
#define TCP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
//#include <netinet/in.h>
#include <netdb.h>

#define TCP_CONN_NUM 5
#define TCP_READBUF_LEN 1024

#define TCP_ERROR_SOCKET  -1  
#define TCP_ERROR_GETHOST -2
#define TCP_ERROR_CONNECT -3
#define TCP_ERROR_PORT    -4
#define TCP_ERROR_BIND    -5
#define TCP_ERROR_LISTEN  -6
#define TCP_ERROR_ACCEPT  -7
#define TCP_ERROR_SEND    -8
#define TCP_ERROR_RECV    -9
#define TCP_ERROR_SETSOCKOPT -10

#define TCP_ERROR_NOTCONN 104

int tcp_open(void);
void tcp_close(int sockfd);
int tcp_accept(int sockfd);
int tcp_listen(char *ip_str,int port,int conn_num,int opt);
int tcp_recv(int sockfd,char *buffer,size_t sz);
int tcp_send(int sockfd,char *buffer,size_t sz);
int tcp_socket_status(int sockfd);
int tcp_client_connect(char *hostname,int portno);

#endif 
