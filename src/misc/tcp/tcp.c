#include "tcp.h"

int tcp_listen(char *ip_str,int port,int conn_num,int opt)
{
	int n;
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = tcp_open();
    if(sockfd < 0) return sockfd;
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    
    if(strlen(ip_str)) serv_addr.sin_addr.s_addr = inet_addr(ip_str);
    else serv_addr.sin_addr.s_addr = INADDR_ANY;
	
	if(port > 0) serv_addr.sin_port = htons(port);
	else return TCP_ERROR_PORT;
	
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
		return TCP_ERROR_SETSOCKOPT;
    }

	n = 5;
	again:
    if(bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		if(n == 0) return TCP_ERROR_BIND;
		else {
			n--;
			sleep(2);
			goto again;
		}
	}
	
	if(conn_num <= 0) conn_num = TCP_CONN_NUM;
	
    if(listen(sockfd,conn_num) < 0) return TCP_ERROR_LISTEN;
    
	return sockfd;
}

int tcp_accept(int sockfd)
{
    int newsockfd;
    socklen_t clilen;
	struct sockaddr_in cli_addr;

    clilen = sizeof(cli_addr);

	newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
		
	if(newsockfd < 0) return TCP_ERROR_ACCEPT;
		
	return newsockfd;
}

int tcp_recv(int sockfd,char *buffer,size_t sz)
{
	int n;
	
	/* ssize_t recv(int socket, void *buffer, size_t length, int flags); */
	n = recv(sockfd,buffer,sz,0);
	if(n < 0) return TCP_ERROR_RECV;
	
	return n;
}

int tcp_send(int sockfd,char *buffer,size_t sz)
{
	int n;
	
	/*  ssize_t send(int socket, const void *buffer, size_t length, int flags); */
	n = send(sockfd,buffer,sz,0);
	if(n < 0) return TCP_ERROR_SEND;
	
	return n;
}

int tcp_open(void)
{
	int sockfd;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) return TCP_ERROR_SOCKET;

	return sockfd;
}

void tcp_close(int sockfd)
{
	close(sockfd);
}

int tcp_socket_status(int sockfd)
{
	int error = 0;
	socklen_t len = sizeof(error);
	//int retval = 
	getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&len);

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


int tcp_client_connect(char *hostname,int portno)
{
	int sockfd;
	struct sockaddr_in serv_addr;
    struct hostent *server;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) return TCP_ERROR_SOCKET;
	
    server = gethostbyname(hostname);
    if(server == NULL) return TCP_ERROR_GETHOST;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    
    serv_addr.sin_port = htons(portno);
    
    if(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		return TCP_ERROR_CONNECT;

    return sockfd;
}

