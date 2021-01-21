#ifndef TIMESTR_H
#define TIMESTR_H

#define RTS_LEN 21

#define TIMESTR(dig,str) (dig >= 10) ? sprintf(str,"%d",dig) : sprintf(str,"0%d",dig)

/* year correct macros */
#define TIMEYEAR(year) (year+1900)

#define RTIMER(times) ((times.tv_sec * 1000000)+(times.tv_usec))

/* microsec to sec macros */
#define RTIMER_SEC(usec) (usec/1000000) 

/* current local time macros */
#define CUR_TIME() time(NULL);

#endif
