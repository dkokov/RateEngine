#ifndef RT_LOG_H
#define RT_LOG_H

#define LOG_LEVEL_DISABLED   0  /* disabled logging,show in the display  */
#define LOG_LEVEL_INFO       1 	/* general information */
#define LOG_LEVEL_WARN       2 	/* warning,notice */
#define LOG_LEVEL_DEBUG      3 	/* debug */
#define LOG_LEVEL_TIME_DEBUG 4  /* debug + timing */

#define LOG_SEPARATOR '|'

#define LOG_MNG_PAUSE 2
#define LOG_MAX_FILE_SIZE 20480000

FILE *re_open_syslog(char *file);
int   re_open_syslog_2(char *file);

void re_write_syslog(FILE *fp,char *func,char *msg,...);
void re_write_syslog_2(int fp,char *func,char *msg,...);
void re_write_syslog_v3(int fp,const char *func,int line,char *msg,...);
void re_write_syslog_v4(char *func,char *msg,...);
void re_reload_syslog(int old_fd,char *old_fn,char *new_fn);
void re_close_2(int fd);

#define LOG(func,msg,args...) re_write_syslog_v4(func,msg,##args);
	
#define DBG(func,msg,args...) \
	if(log_debug_level >= LOG_LEVEL_DEBUG) re_write_syslog_2(config.log,func,msg,##args); \

#define DBG2(msg,args...) re_write_syslog_v3(config.log,__func__,__LINE__,msg,##args);

#define LOG_CLOSE re_close_2(config.log);

#endif
