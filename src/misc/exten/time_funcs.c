#include <stdio.h>
#include <time.h>

/*
struct tm {
   int tm_sec;         // seconds,  range 0 to 59
   int tm_min;         // minutes, range 0 to 59
   int tm_hour;        // hours, range 0 to 23
   int tm_mday;        // day of the month, range 1 to 31
   int tm_mon;         // month, range 0 to 11
   int tm_year;        // The number of years since 1900
   int tm_wday;        // day of the week, range 0 to 6
   int tm_yday;        // day in the year, range 0 to 365
   int tm_isdst;       // daylight saving time
};
*/

void convert_epoch_to_ts(time_t tt,char *ts)
{
	struct tm *tm;
	
	tm = localtime(&tt);
	
	sprintf(ts,"%.4d-%.2d-%.2d %.2d:%.2d:%.2d",(tm->tm_year+1900),(tm->tm_mon+1),tm->tm_mday,
								tm->tm_hour,tm->tm_min,tm->tm_sec);
}

time_t convert_ts_to_epoch(const char *ts)
{
	struct tm ti={0};
	
	/* Timestamp format is 'year-month-day hour:minutes:seconds' */
    if(sscanf(ts,"%d-%d-%d %d:%d:%d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday,&ti.tm_hour,&ti.tm_min,&ti.tm_sec)!=6)
    {
		return 0;
    }
    
    ti.tm_year-=1900;
    ti.tm_mon-=1;
    ti.tm_isdst = -1;
    
    return mktime(&ti);
}

int get_weekday_from_epoch(time_t tt)
{
	struct tm *info;
	info = localtime(&tt);
	return info->tm_wday;
}

/* Date Validation : valid = 1,invalid = 0 */
int check_date_valid(char *str)
{
	int d,m,y;
	int daysinmonth[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
 
	sscanf(str,"%d-%d-%d",&y,&m,&d);
 
	if(y % 400 == 0 || (y % 100 != 0 && y % 4 == 0)) daysinmonth[1] = 29;
 
	if( m < 13 )
      if( d <= daysinmonth[m-1] ) return 1;
 
	return 0;
}
/*
int main(int argc,char *argv[])
{
	printf("\n %s , %d\n",argv[1],check_date_valid(argv[1]));
	return 0;
}
*/
