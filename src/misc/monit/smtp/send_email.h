#ifndef SEND_EMAIL_H
#define SEND_EMAIL_H

#define SRC_MSG_F 10
#define SRC_MSG_S 256

#include <curl/curl.h>

typedef struct send_email {
	char username[255];
	char password[255];
	char smtp_url[255];
	
	char crt_path[255];
	
	char from[255];
	char to[255];
	char cc[255];
	char subject[255];
	char msg_id[255];
	
	CURL *curl;
	struct curl_slist *recipients;
	
	int lines_read;
	
	char src_msg[SRC_MSG_F][SRC_MSG_S];
} send_email_t;

void send_email_src_msg_init(send_email_t *msg);
int send_email_run(send_email_t *msg);

#endif
