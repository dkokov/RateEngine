#ifndef TIME_FUNCS_H
#define TIME_FUNCS_H

#define DT_LEN 20

/* timer in microsec */
#define RTIMER(times) ((times.tv_sec * 1000000)+(times.tv_usec))

/* microsec to sec macros */
#define RTIMER_SEC(usec) (usec/1000000) 

/* current time in seconds */
#define CUR_TIME()   time(NULL)

#define T_STR(str)   sprintf(str,"%.2d",str)
#define T_YEAR(year) (year + 1900)
#define T_MON(mon)   (mon + 1) 

typedef struct rtimer {
    unsigned short flag;
    double t1;
    double t2;
    double r_timer;
    struct timeval tim;
} rtimer_t;

time_t convert_ts_to_epoch(const char *ts);
int get_weekday_from_epoch(time_t tt);
void convert_epoch_to_ts(time_t tt,char *ts);
int check_date_valid(char *str);
void r_timer(rtimer_t *rt);
void current_datetime(char *ts);
void convert_datetime(char *dst,char *src);

#endif
