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
/* Writes exactly 26 chars + NUL, e.g. "2026-08-05 17:47:06.739580".
 * The callers' buffers were 'char re5_ts[21]', so every single log line
 * overflowed the stack by 6 bytes; at -O2 the 21-byte array is padded to a
 * 32-byte slot, which is why it stayed invisible instead of crashing. Buffers
 * are now RE5_TS_LEN and this uses snprintf with the size so it cannot recur. */
void re5_timestamp(char *ts_str,size_t size)
{
	time_t ts;
	struct tm tmv;
	struct timeval times;

	gettimeofday(&times, NULL);

	ts = times.tv_sec;

	/* localtime() returns a pointer to a shared static struct tm - with the
	 * rating workers, CDR profile threads and CallControl all logging
	 * concurrently that is a data race; localtime_r keeps it per-thread. */
	if(localtime_r(&ts,&tmv) == NULL) {
		snprintf(ts_str,size,"0000-00-00 00:00:00.000000");
		return;
	}

	snprintf(ts_str,size,"%.4d-%.2d-%.2d %.2d:%.2d:%.2d.%.6d",
			(tmv.tm_year + 1900),(tmv.tm_mon + 1),tmv.tm_mday,
			tmv.tm_hour,tmv.tm_min,tmv.tm_sec,((int)times.tv_usec));
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
		char re5_ts[RE5_TS_LEN];
				
		re5_timestamp(re5_ts,sizeof(re5_ts));
		
		if(strchr(msg,'%') != NULL) vsnprintf(va_msg,sizeof(va_msg)-1,msg,ap);
		else strcpy(va_msg,msg);

		snprintf(dt,sizeof(dt)-1,"%c%s%c%s%c%s%c\n",log_separator,re5_ts,log_separator,func,log_separator,va_msg,log_separator);
	           
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
		char re5_ts[RE5_TS_LEN];
				
		re5_timestamp(re5_ts,sizeof(re5_ts));
		
		if(strchr(msg,'%') != NULL) vsnprintf(va_msg,sizeof(va_msg)-1,msg,ap);
		else strcpy(va_msg,msg);

		snprintf(dt,sizeof(dt)-1,
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
		char re5_ts[RE5_TS_LEN];
				
		re5_timestamp(re5_ts,sizeof(re5_ts));
		
		if(strchr(msg,'%') != NULL) vsnprintf(va_msg,sizeof(va_msg)-1,msg,ap);
		else strcpy(va_msg,msg);

		snprintf(dt,sizeof(dt)-1,"%c%s%c%s(),line: %d%c%s%c\n",
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
