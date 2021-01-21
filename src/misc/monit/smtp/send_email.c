/*
 * https://curl.haxx.se/libcurl/c/smtp-tls.html
 * 
 * gcc -Wall -o test send_email.c -lcurl
 * 
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "send_email.h"

void send_email_src_msg_init(send_email_t *msg)
{
	time_t tt = time(NULL);
	struct tm *tm= localtime(&tt);
	srand(tt);
	
	sprintf(msg->src_msg[0],"Date: %s \r\n",asctime(tm));
	sprintf(msg->src_msg[1],"To: %s \r\n",msg->to);
	sprintf(msg->src_msg[2],"From: %s \r\n",msg->from);
	sprintf(msg->src_msg[3],"Cc: %s \r\n",msg->to);
	sprintf(msg->src_msg[4],"Message-ID: <%d-RE-monitor-%d-%d>@%s>\r\n",rand(),rand(),rand(),msg->from);
	sprintf(msg->src_msg[5],"Subject: %s \r\n",msg->subject);
	sprintf(msg->src_msg[6],"\r\n");
}

static size_t send_email_payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	send_email_t *msg = (send_email_t *)userp;
	char *data;
	
	if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
		return 0;
	}
	
	data = msg->src_msg[msg->lines_read];
	
	if(data) {
		size_t len = strlen(data);
		memcpy(ptr, data, len);
		msg->lines_read++;
		
		return len;
	}
	
	return 0;
}

void send_email_set_curl(send_email_t *msg)
{
	msg->recipients = NULL;
	msg->lines_read = 0;
	
	curl_easy_setopt(msg->curl, CURLOPT_USERNAME, msg->username);
	curl_easy_setopt(msg->curl, CURLOPT_PASSWORD, msg->password);
	curl_easy_setopt(msg->curl, CURLOPT_URL, msg->smtp_url);
	curl_easy_setopt(msg->curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
	//curl_easy_setopt(curl, CURLOPT_CAINFO, "/path/to/certificate.pem");
	curl_easy_setopt(msg->curl, CURLOPT_MAIL_FROM, msg->from);
	
	msg->recipients = curl_slist_append(msg->recipients, msg->to);
	if(strlen(msg->cc) > 0) msg->recipients = curl_slist_append(msg->recipients, msg->cc);
	
	curl_easy_setopt(msg->curl, CURLOPT_MAIL_RCPT, msg->recipients);
	curl_easy_setopt(msg->curl, CURLOPT_READFUNCTION, send_email_payload_source);
	curl_easy_setopt(msg->curl, CURLOPT_READDATA, msg);
	curl_easy_setopt(msg->curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(msg->curl, CURLOPT_VERBOSE, 1L);
}

int send_email_run(send_email_t *msg)
{
	int ret;
	CURLcode res = CURLE_OK;
	
	msg->curl = curl_easy_init();
	if(msg->curl) {
		send_email_set_curl(msg);
		
		res = curl_easy_perform(msg->curl);
		
		if(res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
			ret = 1;
		} else ret = 0;
		
		curl_slist_free_all(msg->recipients);
		curl_easy_cleanup(msg->curl);
	}
	
	return ret;
}
/*
int main(void)
{
	send_email_t test = {0};
	
	strcpy(test.username,"d.kokov");
	strcpy(test.password,"kokovd");
	strcpy(test.smtp_url,"smtp://mail.bulsat.com");
	
	strcpy(test.from,"<d.kokov@bulsat.com>");
	strcpy(test.to,"<d.kokov@bulsat.com>");
	strcpy(test.cc,"<d.kokov@bulsat.com>");
	strcpy(test.subject,"RE6 Monitor");
	
	send_email_src_msg_init(&test);
	
	strcpy(test.src_msg[7],"ERROR!RateEngine -> CallControl is stoped\r\n");
	
	send_email_run(&test);
	
	return 0;
}

*/
