#include <stdio.h>
#include <time.h>
#include <sys/types.h>

#include <MyCC.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

#define HOST "Test1"
#define SERVER 42

int main(int argc, char *argv[])
{
	int portno;

	int sockfd,n;
	struct sockaddr_in serv_addr;
    struct hostent *server;
    
	char buffer[512];
	char *write_buf;
	size_t write_buf_size;
	
	empty_t stat;
	balance_t bal;
	maxsec_t max;
	term_t tr;
	rate_t rt;

	MyCC_PDU_t *myCC = 0;
	MyCC_PDU_t *myCC_Dec = 0;

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port command(maxsec,term,status,balance,rate) \n", argv[0]);
       exit(0);
    }
   
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

	portno = atoi(argv[2]);

	if(strcmp(argv[3],"maxsec") == 0) {
			max.ts = time(NULL);
			max.tid = 1;
			max.cdr_server_id = SERVER;
			strcpy(max.host,HOST);
			
			strcpy(max.clg,argv[4]);
			strcpy(max.cld,argv[5]);
			strcpy(max.call_uid,argv[6]);		
			
			myCC = my_request_maxsec(&max);
	} else if(strcmp(argv[3],"balance") == 0) {
			bal.ts = time(NULL);
			bal.tid = 1;
			bal.cdr_server_id = SERVER;
			strcpy(bal.host,HOST);
			
			strcpy(bal.clg,argv[4]);
	
			myCC = my_request_balance(&bal);		
	} else if(strcmp(argv[3],"rate") == 0) {
			rt.ts = time(NULL);
			rt.tid = 1;
			rt.cdr_server_id = SERVER;
			strcpy(rt.host,HOST);
			
			strcpy(rt.clg,argv[4]);
			strcpy(rt.cld,argv[5]);
			
			myCC = my_request_rate(&rt);		
	} else if(strcmp(argv[3],"term") == 0) {
			tr.ts = time(NULL);
			tr.tid = 1;
			tr.cdr_server_id = SERVER;
			strcpy(tr.host,HOST);
			strcpy(tr.clg,argv[4]);
			strcpy(tr.cld,argv[5]);
			strcpy(tr.call_uid,argv[6]);
			tr.billsec = 123;
			
			myCC = my_request_term(&tr);
	} else if(strcmp(argv[3],"status") == 0) {
			stat.ts = time(NULL);
			stat.tid = 1;
			stat.cdr_server_id = SERVER;
			strcpy(stat.host,HOST);
			stat.type = MyRequestPayload_PR_status;
			
			myCC = my_request_empty(&stat);
	} else {
        fprintf(stderr,"\nInvalid command: %s\n",argv[3]);
		exit(0);
	}

	if(myCC == NULL) {
		fprintf(stderr,"\nEncoding error!\n");
		exit(0);
	}
	
	memset(buffer,0,sizeof(buffer));

	/* Encoding */	
	encode_MyCC_PDU(myCC,buffer,sizeof(buffer));

	write_buf_size = recognize_buffer_size(buffer,sizeof(buffer));

	/* Open socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
        fprintf(stderr,"\nERROR opening socket\n");
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    
    connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));

	write_buf = malloc(write_buf_size);
	memcpy(write_buf,buffer,write_buf_size);

	n = write(sockfd,write_buf,write_buf_size);
    if (n < 0) {
         fprintf(stderr,"\nERROR reading from socket\n");
    }
        
    memset(buffer,0,sizeof(buffer));
    n = read(sockfd,buffer,sizeof(buffer));
    if (n < 0) {
        fprintf(stderr,"\nERROR reading from socket\n");
    } else {
		fprintf(stderr,"\nRead buffer size: %d\n",n);
	}

	/* Decoding */
	myCC_Dec = calloc(1, sizeof(MyCC_PDU_t));
	decode_MyCC_PDU(myCC_Dec,buffer,sizeof(buffer));

	if(myCC_Dec != NULL) {
		fprintf(stderr, "\nDEBUG: %ld , %ld \n",myCC_Dec->choice.myResponse.header.timestamp,myCC_Dec->choice.myResponse.payload.choice.maxsec.maxsec);
	} else {
		fprintf(stderr, "\nMyCC_Dec is NULL: \n");		
	}

	free(write_buf);
	free_MyCC_PDU(myCC);
	free_MyCC_PDU(myCC_Dec);
	
	close(sockfd);

	return 0;
}
