#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

void error(const char *msg)
{
	perror(msg);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    fd_set fdset;
	struct timeval tv;
	int rc;
	int so_error;
    socklen_t len;
	char *addr;
	int port;

	int erron;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port command(cdr_server_id,clg,cld,call_id)\n", argv[0]);
       exit(0);
    }

    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
		exit(0);
	}

//	fcntl(sockfd, F_SETFL, O_NONBLOCK);

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    
	connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
    
//    FD_ZERO(&fdset);
//    FD_SET(sockfd, &fdset);
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    
//    rc = select(sockfd + 1, NULL, &fdset, NULL, &tv);
//    switch(rc) {
//		case 1: // data to read
//        len = sizeof(so_error);

//        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);

//        if (so_error == 0) {
//            break;
//        } else { // error
            //printf("socket %s:%d NOT connected: %s\n", addr, port, strerror(so_error));
//        }
//        break;
//		case 0: //timeout
//        fprintf(stderr, "connection timeout\n");
//        goto _end_;
//    }
    
    bzero(buffer,255);
    strcpy(buffer,argv[3]);
    n = write(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR writing to socket");

    printf("\n%d %s\n",n,buffer);

//	usleep(500);

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

    bzero(buffer,255);
    n = read(sockfd,buffer,255);
    if (n < 0) {
         error("ERROR reading from socket");
         printf("0\n");
    } else printf("%s\n",buffer);

	_end_:
    close(sockfd);
    return 0;
}
