#ifndef TIME_FUNCS_H
#define TIME_FUNCS_H

#define T_STR(str) sprintf(str,"%.2d",str)

#define T_YEAR(year) (year + 1900)
#define T_MON(mon) (mon + 1) 

time_t convert_ts_to_epoch(const char *ts);
int get_weekday_from_epoch(time_t tt);
void convert_epoch_to_ts(time_t tt,char *ts);
int check_date_valid(char *str);

#endif
