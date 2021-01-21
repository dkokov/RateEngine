#ifndef NET_H
#define NET_H

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

#define PROTO_LEN      32
#define IP_ADDR_LEN   255
#define HOSTNAME_LEN  255
#define FILENAME_LEN  255
#define NET_BUF_LEN  2048

/* Network connection init reply params: number and interval(sec.) */
#define NET_CONN_REPLY_NUM 10
#define NET_CONN_REPLY_INT 2

/* Network connections number */
#define NET_CONN_NUM 5

#define NET_OK 0

#define NET_ERROR_SOCKET      -1  
#define NET_ERROR_GETHOST     -2
#define NET_ERROR_CONNECT     -3
#define NET_ERROR_PORT        -4
#define NET_ERROR_BIND        -5
#define NET_ERROR_LISTEN      -6
#define NET_ERROR_ACCEPT      -7
#define NET_ERROR_SEND        -8
#define NET_ERROR_RECV        -9
#define NET_ERROR_SETSOCKOPT -10

#define NET_ERROR_NOTCONN    104

#define NET_T_POINTER_NUL      -100
#define NET_T_CONN_POINTER_NUL -101
#define NET_T_API_POINTER_NUL  -102
#define NET_T_API_OPEN_P_NUL   -103
#define NET_T_API_CLOSE_P_NUL  -104
#define NET_T_API_ACCEPT_P_NUL -105
#define NET_T_API_SEND_P_NUL   -106
#define NET_T_API_RECV_P_NUL   -107
#define NET_T_API_SSERV_P_NUL  -108

#define NET_UDP_PROTO_STR  "udp"
#define NET_TCP_PROTO_STR  "tcp"
#define NET_SCTP_PROTO_STR "sctp"
#define NET_TLS_PROTO_STR  "tls"
#define NET_STLS_PROTO_STR "tls_sctp"

typedef enum {
	server = 1,
	client = 2
} net_type_t;

typedef enum {
	ipv4 = AF_INET,
	ipv6 = AF_INET6,
	loc  = PF_UNIX
} net_dom_t;

typedef enum {
	udp  = 1,
	tcp  = 2,
	tls  = 3,
	sctp = 4,
	tls_sctp = 5
} net_proto_t;

typedef struct net_conn {
	/* IP version id , as number */
	net_dom_t domain;
	
	/* connection type */
	net_type_t  t;
	
	/* protocol id, type number */
	net_proto_t proto_id;
	
	/* socket file descriptors */
	int sockfd;
	int newsockfd;
	
	unsigned short port;
	unsigned short conn_num;

	/* set socket options */
	int opt;
	
	/* bytes in the buffer */
//	size_t sz;

	/* protocol as name , string */
	char proto[PROTO_LEN];
	
	/* ip address as name , string */
	char ip_str[IP_ADDR_LEN];
	char hostname[HOSTNAME_LEN];
	
	/* read & write buffer */
	char *buffer;
	unsigned int buf_size;
	
	void *eng_tmp;
	
	char unix_socket_filename[FILENAME_LEN];
	char cert_filename[FILENAME_LEN];
	char pkey_filename[FILENAME_LEN];
} net_conn_t;

typedef int  (*net_open_f)     (net_conn_t *conn);
typedef void (*net_close_f)    (net_conn_t *conn);
typedef int  (*net_accept_f)   (net_conn_t *conn);
typedef int  (*net_listen_f)   (net_conn_t *conn);
typedef int  (*net_recv_msg_f) (net_conn_t *conn);
typedef int  (*net_send_msg_f) (net_conn_t *conn);
typedef int  (*net_status_f)   (net_conn_t *conn);
typedef int  (*net_remote_conn_f) (net_conn_t *conn);

typedef int  (*net_s_server_f) (net_conn_t *conn,int (*external_func)(char *));


typedef struct net_funcs {
	net_open_f        open;
	net_close_f       close;
	net_accept_f      accept;
	net_listen_f      listen;
	net_recv_msg_f    recv_msg;
	net_send_msg_f    send_msg;
	net_status_f      status;
	net_remote_conn_f connect;
	net_s_server_f    s_server;
} net_funcs_t;

typedef struct net {
	net_conn_t  *conn;
	net_funcs_t *api;	
} net_t;

int net_open(net_t *np);
int net_accept(net_t *np);
//int net_listen(net_t *np);
int net_recv_msg(net_t *np);
int net_send_msg(net_t *np);

int net_close(net_t *np);

void net_buffer_init(net_conn_t *conn);
void net_clear_buffer(net_conn_t *conn);
void net_buffer_free(net_conn_t *conn);

net_t *net_init(void);
void net_free(net_t *np);

int net_open_socket(net_conn_t *conn);
int net_close_socket(net_conn_t *conn);
int net_listen(net_conn_t *conn);

int net_proto_bind(net_t *np);
int net_serial_server(net_t *np,int (*external_func)(char *));

void net_test(void);

#endif
