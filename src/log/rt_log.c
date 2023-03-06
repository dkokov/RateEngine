/* 
 * http://stackoverflow.com/questions/8884335/print-the-file-name-line-number-and-function-name-of-a-calling-function-c-pro
 * 
 * https://pmihaylov.com/macros-in-c/
 */

#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include "../misc/globals.h"

// strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&(ptr->ts))); ???
void re5_timestamp(char *ts_str)
{
	time_t ts;
	struct tm *tm;
	struct timeval times;
	
	gettimeofday(&times, NULL);

	ts = times.tv_sec;

	tm = localtime(&ts);
	
	sprintf(ts_str,"%.4d-%.2d-%.2d %.2d:%.2d:%.2d.%.6d",(tm->tm_year + 1900),(tm->tm_mon + 1),tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec,((int)times.tv_usec));
}

FILE *re_open_syslog(char *file)
{
	FILE *fp;
  
	fp = fopen (file,"a");
  
	return fp;
}

int re_open_syslog_2(char *file)
{
	int fp;
  
	fp = open (file,O_CREAT|O_RDWR|O_APPEND,0600);

	return fp;
}

void re_close_2(int fd)
{
	close(fd);
}

void re_reload_syslog(int old_fd,char *old_fn,char *new_fn)
{
	int status;
	int new_fd;
	
	status = rename(old_fn,new_fn);
	
	if(status == 0) {
		new_fd = re_open_syslog_2(old_fn);
		dup2(new_fd,old_fd);
		close(new_fd);
	}
}

char *re_create_log_msg(int len,char *msg)
{
	int msize;
	char *log_msg_pt = NULL;
	
	msize = ((len+1)*sizeof(char));
	log_msg_pt = (char *)malloc(msize);
	
	if(log_msg_pt != NULL) {
		memset(log_msg_pt,0,msize);
		strcpy(log_msg_pt,msg);
	}
	
	return log_msg_pt;
}

void re_put_in_syslog(FILE *fp,char *func,char *msg,va_list ap)
{
	char dt[2048] = "";
	char va_msg[1024] = "";
	
    char *dtp;
    int len;
	
	if(fp != NULL) {
		char re5_ts[21];
				
		re5_timestamp(re5_ts);
		
		if(strchr(msg,'%') != NULL) vsprintf(va_msg,msg,ap);
		else strcpy(va_msg,msg);

		sprintf(dt,"%c%s%c%s%c%s%c\n",log_separator,re5_ts,log_separator,func,log_separator,va_msg,log_separator);
	           
		len = strlen(dt);

		dtp = re_create_log_msg(len,dt);

		fprintf(fp,"%s",dtp);
		
		free(dtp);
    }
}

void re_put_in_syslog_v2(int fp,char *func,char *msg,va_list ap)
{
	char dt[2048] = "";
	char va_msg[1024] = "";
	
    char *dtp;
    int len;
	
	if(fp) {
		char re5_ts[21];
				
		re5_timestamp(re5_ts);
		
		if(strchr(msg,'%') != NULL) vsprintf(va_msg,msg,ap);
		else strcpy(va_msg,msg);

		sprintf(dt,
				"%c%s%c%s%c%s%c\n",
				log_separator,re5_ts,log_separator,func,log_separator,va_msg,log_separator);
	           
		len = strlen(dt);

		dtp = re_create_log_msg(len,dt);

		write(fp,dtp,len);

		free(dtp);
    }
}

/* 
 * https://stackoverflow.com/questions/2849832/c-c-line-number 
 * __func__
 * __LINE__
 * __DATE__
 * __TIME__
 * ##__VA_ARGS__
 * 
 * */
void re_put_in_syslog_v3(int fp,const char *func,int line,char *msg,va_list ap)
{
	char dt[2048] = "";
	char va_msg[1024] = "";
	
    char *dtp;
    int len;
	
	if(fp) {
		char re5_ts[21];
				
		re5_timestamp(re5_ts);
		
		if(strchr(msg,'%') != NULL) vsprintf(va_msg,msg,ap);
		else strcpy(va_msg,msg);

		sprintf(dt,"%c%s%c%s(),line: %d%c%s%c\n",
				log_separator,re5_ts,log_separator,func,line,log_separator,va_msg,log_separator);
	           
		len = strlen(dt);

		dtp = re_create_log_msg(len,dt);

		write(fp,dtp,len);

		free(dtp);
    }
}

void re_write_syslog(FILE *fp,char *func,char *msg,...)
{
	va_list ap;

	va_start(ap,msg);

	re_put_in_syslog(fp,func,msg,ap);
	
	va_end(ap);
}

void re_write_syslog_2(int fp,char *func,char *msg,...)
{
	va_list ap;

	va_start(ap,msg);

	re_put_in_syslog_v2(fp,func,msg,ap);
	
	va_end(ap);
}

void re_write_syslog_v3(int fp,const char *func,int line,char *msg,...)
{
	va_list ap;

	va_start(ap,msg);

	re_put_in_syslog_v3(fp,func,line,msg,ap);
	
	va_end(ap);		
}

void re_write_syslog_v4(char *func,char *msg,...)
{
	va_list ap;

	va_start(ap,msg);

	if(log_debug_level == LOG_LEVEL_DISABLED) {
		re_put_in_syslog(stderr,func,msg,ap);
		fflush(stderr);
	} else { 
		re_put_in_syslog_v2(config.log,func,msg,ap);
	}
	
	va_end(ap);
}
