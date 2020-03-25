#include "../../misc/globals.h"
#include "../../misc/tcp/tcp.h"
#include "../../misc/mem/mem.h"

#include "../cc_cfg.h"

#include "my_cc.h" 

#include <pthread.h>
#include <unistd.h>
#include <errno.h>

void my_cc_ack(my_cc_t *my,char *data)
{
	char buf[512];
	
	if(my != NULL) {
		sprintf(buf,"%d%s%d%s%s",
				my->cdr_server_id,MY_CC_SEPARATOR,my->transaction_id,MY_CC_SEPARATOR,data);
	} else {
		sprintf(buf,"%s",MY_CC_ERR);
	}
	
	mem_free(data);
	
	my->ack = strdup(buf);
}

char *my_cc_comp_ack(cc_t *cc_ptr)
{
	char *data;
	char buf[512];
	
	memset(buf,0,512);
	
	switch(cc_ptr->t) {
		case maxsec_event:
			if(cc_ptr->max != NULL){
				sprintf(buf,"%d",cc_ptr->max->maxsec);
				
				if(cc_ptr->max->maxsec < 0) cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			} else {
				strcpy(buf,"error");
				cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			}
			
			break;
		case balance_event:
			if(cc_ptr->bal != NULL){
				sprintf(buf,"%f",cc_ptr->bal->amount);
			} else strcpy(buf,"error");
			
			cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			
			break;
		case rate_event:
			if(cc_ptr->rate != NULL){
				sprintf(buf,"%f",cc_ptr->rate->rate_price);
			} else strcpy(buf,"error");
			
			cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			
			break;
		case term_event:
			if(cc_ptr->term != NULL){
				if(cc_ptr->term->tbl_idx >= 0) sprintf(buf,"%s",MY_CC_OK);
				else {
					sprintf(buf,"%s",MY_CC_NOK);
					cc_ptr->cc_status = CC_STATUS_DEACTIVE;
				}
			} else {
				strcpy(buf,"error");
				cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			}
			
			break;
		case cprice_event:
		case status_event:
			strcpy(buf,MY_CC_OK);
			cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			
			break;
		case unkn_event:
			strcpy(buf,MY_CC_ERR);
			cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			
			break;
	}
	
	data = strdup(buf);
	
	return data;
}

my_cc_t *my_cc_parser(char *buffer)
{
	char *buf;
	my_cc_t *my_request;
	
	my_request = (my_cc_t *)mem_alloc(sizeof(my_cc_t));
	my_request->cc_ptr = cc_alloc();
	
	if((my_request != NULL)&&(my_request->cc_ptr != NULL)) {
		my_request->cc_ptr->t = unkn_event;
		
		if(strlen(buffer) > 0) {
			buf = strtok(buffer,MY_CC_SEPARATOR);
			if(buf != NULL) my_request->cdr_server_id = atoi(buf);
			else goto end_func;
			
			buf = strtok(NULL,MY_CC_SEPARATOR);
			if(buf != NULL) my_request->transaction_id = atoi(buf);
			else goto end_func;
			
			buf = strtok(NULL,MY_CC_SEPARATOR);
			if(buf != NULL) strcpy(my_request->command,buf);
			else goto end_func;

			buf = strtok(NULL,MY_CC_SEPARATOR);
			if(buf != NULL) my_request->timestamp = atoi(buf);
			else goto end_func;
			
			if(strcmp(my_request->command,MY_CC_STATUS) == 0) {
				my_request->cc_ptr->t = status_event;
				my_request->cc_ptr->cdr_server_id = my_request->cdr_server_id;
				
				return my_request;
			}
			
			if(strcmp(my_request->command,MY_CC_MAXSEC) == 0) {
				my_request->cc_ptr->max = (cc_maxsec_t *)mem_alloc(sizeof(cc_maxsec_t));
				
				if(my_request->cc_ptr->max != NULL) {
					my_request->cc_ptr->t = maxsec_event;
					my_request->cc_ptr->cdr_server_id = my_request->cdr_server_id;
					
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) strcpy(my_request->cc_ptr->max->clg,buf);
					else goto end_func;
				
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) strcpy(my_request->cc_ptr->max->cld,buf);
					else goto end_func;
				
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) strcpy(my_request->cc_ptr->max->call_uid,buf); 
					else goto end_func;
					
					my_request->cc_ptr->max->ts = time(NULL);
				}
				
				return my_request;
			}
			
			if(strcmp(my_request->command,MY_CC_RATE) == 0) {	
				my_request->cc_ptr->rate = (cc_rate_t *)mem_alloc(sizeof(cc_rate_t));
			
				if(my_request->cc_ptr->rate != NULL) {
					my_request->cc_ptr->t = rate_event;
					my_request->cc_ptr->cdr_server_id = my_request->cdr_server_id;
				
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) strcpy(my_request->cc_ptr->rate->clg,buf);
					else goto end_func;
				
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) strcpy(my_request->cc_ptr->rate->cld,buf);
					else goto end_func;
				}
			
				return my_request;
			}
			
			if(strcmp(my_request->command,MY_CC_BALANCE) == 0) {
				my_request->cc_ptr->bal = (cc_balance_t *)mem_alloc(sizeof(cc_balance_t));
				
				if(my_request->cc_ptr->bal != NULL) {
					my_request->cc_ptr->t = balance_event;
					my_request->cc_ptr->cdr_server_id = my_request->cdr_server_id;
				
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) strcpy(my_request->cc_ptr->bal->clg,buf);
					else goto end_func;
				}
				
				return my_request;
			}
			
			if(strcmp(my_request->command,MY_CC_CPRICE) == 0) {
				my_request->cc_ptr->cprice = (cc_cprice_t *)mem_alloc(sizeof(cc_cprice_t));
				
				if(my_request->cc_ptr->cprice != NULL) {
					my_request->cc_ptr->t = cprice_event;
					my_request->cc_ptr->cdr_server_id = my_request->cdr_server_id;
					
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) strcpy(my_request->cc_ptr->cprice->clg,buf);
					else goto end_func;
				
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) strcpy(my_request->cc_ptr->cprice->cld,buf);
					else goto end_func;
				
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) my_request->cc_ptr->cprice->billsec = atoi(buf);
					else goto end_func;
				}
			
				return my_request;
			}
			
			if(strcmp(my_request->command,MY_CC_TERM) == 0) {
				my_request->cc_ptr->term = (cc_term_t *)mem_alloc(sizeof(cc_term_t));
			
				if(my_request->cc_ptr->term != NULL) {
					my_request->cc_ptr->t = term_event;
					my_request->cc_ptr->cdr_server_id = my_request->cdr_server_id;
					
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) {
						if(strcmp(buf,MY_CC_TERM_NC) == 0) my_request->cc_ptr->term->status = normal_clear;
						else if(strcmp(buf,MY_CC_TERM_C) == 0) my_request->cc_ptr->term->status = cancel;
						else if(strcmp(buf,MY_CC_TERM_B) == 0) my_request->cc_ptr->term->status = busy;
						else if(strcmp(buf,MY_CC_TERM_E) == 0) my_request->cc_ptr->term->status = error;
						else my_request->cc_ptr->term->status = unkn_status;
					} else goto end_func;
					
					buf = strtok(NULL,MY_CC_SEPARATOR);
					if(buf != NULL) strcpy(my_request->cc_ptr->term->call_uid,buf);					
					else goto end_func;
					
					if(my_request->cc_ptr->term->status == normal_clear) {
						buf = strtok(NULL,MY_CC_SEPARATOR);
						if(buf != NULL) my_request->cc_ptr->term->billsec = atoi(buf);					
						else goto end_func;
					
						buf = strtok(NULL,MY_CC_SEPARATOR);
						if(buf != NULL) my_request->cc_ptr->term->duration = atoi(buf);					
						else goto end_func;
					}
				}

				return my_request;
			}
		}
		
		return my_request;
	}
	
	end_func:
	return NULL;
}

my_cc_thread_dt_t *my_cc_thread_dt_tbl(int n)
{
	my_cc_thread_dt_t *tbl;
	
	if(n <= 0) return NULL;
	
	tbl = (my_cc_thread_dt_t *)mem_alloc_arr(n,sizeof(my_cc_thread_dt_t));
		
	return tbl;
}

void *my_cc_thread(void *dt)
{
	int n,i;
	my_cc_t *my;
		
	char *data;
	char readbuf[TCP_READBUF_LEN];

    fd_set readfds;
    int client_socket[TCP_SOCKETS_MAX];
    
    socklen_t clilen;
	int sd,max_sd,activity;

    struct sockaddr_in serv_addr;
    int newsockfd;

#if CC_NOLOOP
	int c = 0;
#endif

	my_cc_thread_dt_t *tmp = (my_cc_thread_dt_t *)dt;

	LOG("my_cc_thread()","The thread is started!");

    for (i = 0; i < TCP_SOCKETS_MAX; i++) {
        client_socket[i] = 0;
    }
    
    clilen = sizeof(serv_addr);

#if CC_NOLOOP
	while(c < 2) {
#else
	while(1) {
#endif
		if(tmp->server_sockfd <= 0) {
			LOG("my_cc_thread()","The server socket is closed!");
			break;
		}

		FD_ZERO(&readfds);

		FD_SET(tmp->server_sockfd, &readfds);
		max_sd = tmp->server_sockfd;
		
		for ( i = 0 ; i < TCP_SOCKETS_MAX ; i++) {
			sd = client_socket[i];
	
			if(sd > 0) FD_SET( sd , &readfds);
	
			if(sd > max_sd) max_sd = sd;
		}
		
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		
		if ((activity < 0) && (errno!=EINTR)) {
			LOG("my_cc_thread()","ERROR on select");
			break;
		}
		
		if (FD_ISSET(tmp->server_sockfd, &readfds)) {
			newsockfd = tcp_accept(tmp->server_sockfd);
			if (newsockfd < 0) {
				LOG("my_cc_thread()","ERROR on accept");
				break;
			}
			
			getpeername(newsockfd, (struct sockaddr*)&serv_addr , (socklen_t*)&clilen);
			LOG("my_cc_thread()","New connection,socket fd is %d,host is : %s:%d ", newsockfd,inet_ntoa(serv_addr.sin_addr),ntohs(serv_addr.sin_port));
	
			for (i = 0; i < TCP_SOCKETS_MAX; i++) {
				if( client_socket[i] == 0 ) {
					client_socket[i] = newsockfd;
					break;
				}
			}
		}
		
		for (i = 0; i < TCP_SOCKETS_MAX; i++) {
			sd = client_socket[i];
	
			if(FD_ISSET(sd,&readfds)) {
				memset(readbuf,0,TCP_READBUF_LEN);
				
				n = tcp_recv(sd,readbuf,TCP_READBUF_LEN);
				if (n <= 0) {
					getpeername(sd,(struct sockaddr*)&serv_addr,(socklen_t*)&clilen);
					LOG("my_cc_thread()","Host disconnected [%s:%d]",inet_ntoa(serv_addr.sin_addr),ntohs(serv_addr.sin_port));
		
					close( sd );
					client_socket[i] = 0;
				} else {
					my = my_cc_parser(readbuf);

					if(my != NULL) {
						if(my->cc_ptr != NULL) {
							cc_event_manager(my->cc_ptr);

							data = my_cc_comp_ack(my->cc_ptr);
				
							if(data != NULL) my_cc_ack(my,data);
							else my->ack = strdup("error");
				
							if(my->cc_ptr->cc_status == CC_STATUS_DEACTIVE) {
								cc_free(my->cc_ptr);
								LOG("my_cc_thread()","CC_STATUS_DEACTIVE ... cc_free(my->cc_ptr)");
							} else {
								if(my->cc_ptr->t == term_event) {
									LOG("my_cc_thread()","TERM_EVENT ... mem_free(my->cc_ptr)");
									mem_free(my->cc_ptr);
								}
							}
						} else my->ack = strdup("error");
					
						tcp_send(sd,my->ack,strlen(my->ack));

						mem_free(my->ack);
						mem_free(my);
#if CC_NOLOOP
						c++;
#endif
					} else {
						tcp_send(sd,"error",5);
					}
				}
			}
		}
	}

	tcp_close(tmp->server_sockfd);
	free(tmp);

	LOG("my_cc_thread()","The tcp socket is closed and the MyCC thread is stoped!");
	
	pthread_exit(NULL);
}

void my_cc_thread_run(my_cc_thread_dt_t *tmp)
{
	if(tmp != NULL) {
		tmp->proc.args = (void *)tmp;
		tmp->proc.thread_func = my_cc_thread;
		//tmp->proc.mode = PROC_THREAD_JOINABLE;
		tmp->proc.mode = PROC_THREAD_DETACHED;
		proc_thread_run(&tmp->proc);
	}
}

int my_cc_main(cc_cfg_int_t *_int)
{
	int opt;
	my_cc_thread_dt_t *tbl;
		
	tbl = my_cc_thread_dt_tbl(1);
		
	if(tbl == NULL) return 1;
	
	tbl->server_sockfd = tcp_listen(_int->ip,_int->port,0,1);
	if(tbl->server_sockfd > 0) {		
		LOG("my_cc_main()","tcp server socket is opened!");
		
		opt=1;
		if(setsockopt(tbl->server_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
			LOG("my_cc_main()","ERROR setsockopt");
			
			tcp_close(tbl->server_sockfd);
			free(tbl);
			
			return 2;
		}
		
		my_cc_thread_run(tbl);

	} else return tbl->server_sockfd;
	
	mem_free(_int);
		
	return 0;
}
