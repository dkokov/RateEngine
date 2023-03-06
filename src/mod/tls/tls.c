/*
 * https://www.openssl.org/docs/manmaster/man3/
 * https://wiki.openssl.org/index.php/Simple_TLS_Server
 *  
 * */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../mod.h"
#include "../../mem/mem.h"
#include "../../net/net.h"

#include "tls.h"

tls_t *tls_init(void)
{
	tls_t *ptr;
	
	ptr = mem_alloc(sizeof(tls_t));
	
	if(ptr == NULL) return NULL;
	
	/* SSL init */
	SSL_load_error_strings();	
	OpenSSL_add_ssl_algorithms();
	
	/* create context */
    ptr->method = SSLv23_server_method();
    ptr->ctx = SSL_CTX_new(ptr->method);
    
    if(!ptr->ctx) {
		EVP_cleanup();

		mem_free(ptr);
		
		return NULL;
    }
	
	return ptr;
}

void tls_free(tls_t *ptr)
{
	if(ptr != NULL) {
		SSL_CTX_free(ptr->ctx);
		EVP_cleanup();
		
		mem_free(ptr);
	}
}

int tls_open(net_conn_t *conn)
{
	tls_t *ptr;
	
	if(conn == NULL) return NET_ERROR_SOCKET;
	
	net_open_socket(conn);

	ptr = tls_init();
	
	if(ptr == NULL) return NET_ERROR_SOCKET;

	/* Configure context */
    SSL_CTX_set_ecdh_auto(ptr->ctx, 1);

    if (SSL_CTX_use_certificate_file(ptr->ctx,conn->cert_filename, SSL_FILETYPE_PEM) <= 0) return NET_ERROR_SOCKET;
    if (SSL_CTX_use_PrivateKey_file(ptr->ctx,conn->pkey_filename, SSL_FILETYPE_PEM) <= 0 ) return NET_ERROR_SOCKET;
	
	conn->eng_tmp = (void *)ptr;

	return 0;
}

void tls_close(net_conn_t *conn)
{
	if(conn != NULL) { 
		tls_free((tls_t *)conn->eng_tmp);
		
		net_close_socket(conn);
	}
}
/*
int tls_listen(net_conn_t *conn)
{
    struct sockaddr_in serv_addr;

	if(conn == NULL) return NET_ERROR_SOCKET;

	if(conn->sockfd < 0) return NET_ERROR_SOCKET;  
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    
    if(strlen(conn->ip_str)) serv_addr.sin_addr.s_addr = inet_addr(conn->ip_str);
    else serv_addr.sin_addr.s_addr = INADDR_ANY;
	
	if(conn->port > 0) serv_addr.sin_port = htons(conn->port);
	else return NET_ERROR_PORT;
	
    if(setsockopt(conn->sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&conn->opt, sizeof(conn->opt)) < 0 ) {
		return NET_ERROR_SETSOCKOPT;
    }

    if(bind(conn->sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
		return NET_ERROR_BIND;
	
	if(conn->conn_num <= 0) conn->conn_num = NET_CONN_NUM;
	
    if(listen(conn->sockfd,conn->conn_num) < 0) return NET_ERROR_LISTEN;
    
	return 0;
}*/

int tls_accept(net_conn_t *conn)
{
	socklen_t clilen;
	struct sockaddr_in cli_addr;

	tls_t *ptr;

    clilen = sizeof(cli_addr);

	if(conn == NULL) return NET_ERROR_SOCKET;

	if(conn->eng_tmp == NULL) return NET_ERROR_ACCEPT;
	
	ptr = (tls_t *)conn->eng_tmp;

	conn->newsockfd = accept(conn->sockfd,(struct sockaddr *) &cli_addr,&clilen);
		
	if(conn->newsockfd < 0) return NET_ERROR_ACCEPT;
	
	ptr->ssl = SSL_new(ptr->ctx);
    SSL_set_fd(ptr->ssl,conn->newsockfd);

	if(SSL_accept(ptr->ssl) <= 0) return NET_ERROR_ACCEPT;
		
	return 0;
}

int tls_recv(net_conn_t *conn)
{
	int n;
	tls_t *ptr;
	
	if(conn == NULL) return NET_ERROR_SOCKET;

	if(conn->eng_tmp == NULL) return NET_ERROR_SEND;

	ptr = (tls_t *)conn->eng_tmp;

	n = SSL_read(ptr->ssl,conn->buffer,strlen(conn->buffer));
	
	SSL_free(ptr->ssl);
	
	return 0;
}

int tls_send(net_conn_t *conn)
{	
	int n;
	tls_t *ptr;
	
	if(conn == NULL) return NET_ERROR_SOCKET;

	if(conn->eng_tmp == NULL) return NET_ERROR_SEND;

	ptr = (tls_t *)conn->eng_tmp;

	n = SSL_write(ptr->ssl, conn->buffer, strlen(conn->buffer));
	
	SSL_free(ptr->ssl);
	
	return n;
}

/*
int tcp_client_connect(net_conn_t *conn)
{
	struct sockaddr_in serv_addr;
    struct hostent *server;
 
	if(conn == NULL) return TCP_ERROR_SOCKET;
    
    conn->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(conn->sockfd < 0) return TCP_ERROR_SOCKET;
	
    server = gethostbyname(conn->hostname);
    if(server == NULL) return TCP_ERROR_GETHOST;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    
    serv_addr.sin_port = htons(conn->port);
    
    if(connect(conn->sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		return TCP_ERROR_CONNECT;

    return 0;
}*/

int tls_bind_api(net_funcs_t *ptr)
{
	if(ptr == NULL) return -1;
		
	ptr->open     = tls_open;
	ptr->close    = tls_close;
	ptr->accept   = tls_accept;
	ptr->listen   = NULL;
	ptr->recv_msg = tls_recv;
	ptr->send_msg = tls_send;
	ptr->status   = NULL;
	ptr->connect  = NULL;
	
	return 0;
}
