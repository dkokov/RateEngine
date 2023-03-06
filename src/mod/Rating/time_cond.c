#include <time.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "rt_data.h"
#include "time_cond.h"

typedef struct wdays
{
    short dnum;
    char dname[4];
}wdays;

wdays week_days[] = 
{
    {1,"mon"},
    {2,"tue"},
    {3,"wed"},
    {4,"thu"},
    {5,"fri"},
    {6,"sat"},
    {7,"sun"}
};

int get_week_days_dnum(char *dway)
{
    int i;
    
    if(dway != NULL) {
		for(i=0;i<7;i++) {
			if(!strcmp(week_days[i].dname,dway)) return i;
		}
	}
    
    return -1;
}

int get_week_days_dname(short dnum)
{
    int i;
    
    for(i=0;i<7;i++) {
		if(week_days[i].dnum == dnum) return i;
    }
    
    return -1;
}

void parse_tm(time_t ts,timestamp *tt)
{
	struct tm  *tm;
	
	tm = localtime(&ts);
	
	sprintf(tt->time,"%.2d:%.2d:%.2d",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf(tt->date,"%.4d-%.2d-%.2d",(tm->tm_year+1900),(tm->tm_mon+1),tm->tm_mday);	
}

void parse_ts(char *ts,timestamp *tt)
{
    char *date;
    char *time2;
    char *time;
    char *tz;

    if(ts != NULL) {
		date = strtok(ts," ");
		time2 = strtok(NULL," ");
	
		time = strtok(time2,"+");
		tz = strtok(NULL,"+");
	
		strcpy(tt->date,date);
		strcpy(tt->time,time);
		strcpy(tt->timezone,tz);
    }
}

int f_time_cond_query(db_t *dbp,rating_t *pre)
{
	int i;
    char str[SQL_BUF_LEN];
    
	time_cond *ts;
 	db_sql_result_t *result;
 	
 	pre->tc = NULL;

	if(pre->tariff_id <= 0) return RE_ERROR_N;
	
	if(dbp == NULL) return RE_ERROR_N;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return RE_ERROR_N;
		
		sprintf(str,"SELECT tc_deff.id,tc_deff.hours,tc_deff.days_week,tc_deff.tc_name,"
                   "tc_deff.tc_date "
                   " from time_condition tc,time_condition_deff as tc_deff "
                   " where tc.tariff_id = %d  and "
                   " tc.time_condition_id = tc_deff.id order by tc.id",
                   pre->tariff_id);
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
		
			if(result->rows > 0) {
				ts = (time_cond *)mem_alloc_arr(sizeof(time_cond),(result->rows+1));
				if(ts == NULL) return RE_ERROR_N;

				for(i = 0;i < result->rows;i++) {
					ts[i].id = atoi(result->cols_list[0].rows_list[i].row);
					strcpy(ts[i].hours,result->cols_list[1].rows_list[i].row);
					strcpy(ts[i].dweek,result->cols_list[2].rows_list[i].row);
					strcpy(ts[i].tc,result->cols_list[3].rows_list[i].row);
					strcpy(ts[i].tc_date,result->cols_list[4].rows_list[i].row);
				}
				
				ts[i].id = 0;
				
				pre->tc = ts;
			}
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return RE_ERROR_N;
	    
	return RE_SUCCESS;
}

int f_time_cond_query_v2(db_t *dbp,rating_t *pre)
{
	int i;
    char str[SQL_BUF_LEN];
    
	time_cond *ts;
 	db_sql_result_t *result;
 	
 	pre->tc = NULL;

	if(pre->tariff_id <= 0) return RE_ERROR_N;
	
	if(dbp == NULL) return RE_ERROR_N;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return RE_ERROR_N;
		
		sprintf(str,"select df.id,tc.tariff_id,df.hours,df.days_week,df.tc_name,df.tc_date,tc.prior "
					"from time_condition as tc,time_condition_deff as df " 
					"where df.id = tc.time_condition_id and tc.tariff_id = tc.tariff_id and tc.tariff_id = %d order by tc.prior desc", pre->tariff_id);
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
		
			if(result->rows > 0) {
				ts = (time_cond *)mem_alloc_arr(sizeof(time_cond),(result->rows+1));
				if(ts == NULL) return RE_ERROR_N;

				for(i = 0;i < result->rows;i++) {
					ts[i].id = atoi(result->cols_list[0].rows_list[i].row);
					ts[i].tariff_id = atoi(result->cols_list[1].rows_list[i].row);
					strcpy(ts[i].hours,result->cols_list[2].rows_list[i].row);
					strcpy(ts[i].dweek,result->cols_list[3].rows_list[i].row);
					strcpy(ts[i].tc,result->cols_list[4].rows_list[i].row);
					strcpy(ts[i].tc_date,result->cols_list[5].rows_list[i].row);
				}
				
				ts[i].id = 0;
				
				pre->tc = ts;
			}
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return RE_ERROR_N;
	    
	return RE_SUCCESS;   
}

int f_time_cond_query_id(db_t *dbp,rating_t *pre)
{
	int id;
	char str[SQL_BUF_LEN];
	
 	db_sql_result_t *result;
 	
	if(pre->tariff_id <= 0) return 0;

	if(dbp == NULL) return 0;
	
	if(dbp->t == sql) {		
		if(dbp->u.sql == NULL) return 0;
		
		sprintf(str,"select id from time_condition where tariff_id = %d",pre->tariff_id);
		
		db_select(dbp,str);
		db_fetch(dbp);
				
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;

			if(result->rows == 1) id  = atoi(result->cols_list[0].rows_list[0].row);
						
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else return 0;
	
	return id;   
}

/*int compare_tc_ts(rating *pre)
{
    int p;
    int b,n,n2;
    int flag;
    int mon;
    time_t tt;
    time_cond *tc;
    timestamp ts;
    struct tm  *tm;
    char current[6];
    char *hour1;
    char *hour2;
    char *day1;
    char *day2;
    char ttt[21];
    short dweek_ind,dweek_ind_1,dweek_ind_2,dnum_1,dnum_2;
    char buf[512];

    flag = 0;

    bzero(ttt,sizeof(ttt));
    bzero(ts.date,sizeof(ts.date));
    bzero(ts.time,sizeof(ts.time));
    bzero(ts.year,sizeof(ts.year));
    bzero(ts.mon,sizeof(ts.mon));
    bzero(ts.day,sizeof(ts.day));

    tc = pre->tc;
    tt = pre->ts;
    if(tt == 0) strcpy(ttt,pre->timestamp);

    if(tc)
    {		
		if(log_debug_level == LOG_LEVEL_DEBUG)
		{
			bzero(buf,sizeof(buf));
			sprintf(buf,"[%s][1]timestamp %s , ts %d",pre->call_uid,ttt,(int)tt);
			re_write_syslog_2(config.log,"TimeCondition",buf);
		}
		
		if(!strcmp(ttt,"")) 
		{
			tm = localtime(&tt);
			
			if((tm->tm_hour) < 10) sprintf(ts.hour,"0%d",tm->tm_hour);
			else sprintf(ts.hour,"%d",tm->tm_hour);
			
			if((tm->tm_min) < 10) sprintf(ts.min,"0%d",tm->tm_min);
			else sprintf(ts.min,"%d",tm->tm_min);
			
			if((tm->tm_sec) < 10) sprintf(ts.sec,"0%d",tm->tm_sec);
			else sprintf(ts.sec,"%d",tm->tm_sec);
			
			sprintf(ts.year,"%d",(tm->tm_year+1900));
			
			mon = ((tm->tm_mon)+1);
			
			if(mon < 10) sprintf(ts.mon,"0%d",mon);
			else sprintf(ts.mon,"%d",mon);
			
			if(tm->tm_mday < 10) sprintf(ts.day,"0%d",tm->tm_mday);
			else sprintf(ts.day,"%d",tm->tm_mday);
		}
		else 
		{
			parse_ts(ttt,&ts);
			parse_time(&ts);
		}

		sprintf(current,"%s:%s",ts.hour,ts.min);
	
		if(log_debug_level == LOG_LEVEL_DEBUG)
		{
			bzero(buf,sizeof(buf));
			sprintf(buf,"[%s][2]current = %s",pre->call_uid,current);
			re_write_syslog_2(config.log,"TimeCondition",buf);
		}	
	
		p=0;
		while(tc[p].id)
		{
			if(strcmp(tc[p].year,""))
			{
				if(strcmp(tc[p].year,ts.year)) return 0;
				else flag = 1;
			}
			
			if(strcmp(tc[p].mon,""))
			{
				if(strcmp(tc[p].mon,ts.mon)) return 0;
				else flag = 1;
			}
	    
			if(strcmp(tc[p].dmon,""))
			{
				if(strcmp(tc[p].dmon,ts.day)) return 0;
				else flag = 1;
			}
	
			hour1 = strtok(tc[p].hours,"-");
			hour2 = strtok(NULL,"-");
	
			day1 = strtok(tc[p].dweek,"-");
			day2 = strtok(NULL,"-");
	
			dweek_ind = get_week_days_dname(pre->dow);
			
			if(log_debug_level == LOG_LEVEL_DEBUG)
			{
				bzero(buf,sizeof(buf));
				sprintf(buf,"[%s][3]tc[%d],hour1=%s,current=%s,hour2=%s,dow=%d(%s),year=%s,mon=%s,day=%s",
					    pre->call_uid,p,hour1,current,hour2,pre->dow,week_days[dweek_ind].dname,ts.year,ts.mon,ts.day);	
				re_write_syslog_2(config.log,"TimeCondition",buf);
			}
	
			dnum_1 = 0;
			dnum_2 = 8;
	
			dweek_ind_1 = get_week_days_dnum(day1);
			dnum_1 = week_days[dweek_ind_1].dnum ;
	
			dweek_ind_2 = get_week_days_dnum(day2);
			dnum_2 = week_days[dweek_ind_2].dnum ;
	
			if(log_debug_level == LOG_LEVEL_DEBUG)
			{
				bzero(buf,sizeof(buf));
				sprintf(buf,"[%s][4]week_days %d %d",pre->call_uid,dnum_1,dnum_2);
				re_write_syslog_2(config.log,"TimeCondition",buf);
			}		
			
			if((dnum_1 <= pre->dow)&&(dnum_2 >= pre->dow))
			{
				if((hour1 != NULL)&&(hour2 != NULL)&&(current != NULL))
				{
					b = strcmp(hour1,hour2);
					n = strcmp(hour1,current);
					n2 = strcmp(hour2,current);
					
					if(log_debug_level == LOG_LEVEL_DEBUG)
					{
						bzero(buf,sizeof(buf));
						sprintf(buf,"[%s][5]b=%d,n=%d,n2=%d",pre->call_uid,b,n,n2);
						re_write_syslog_2(config.log,"TimeCondition",buf);
					}	
					
					if(b<0)
					{
						if(n<0)
						{
							if(n2>=0) 
							{
								pre->tc_id = tc[p].id;
								return 1;
							}
						}
					}
					else
					{
						int n2a,n2b;
						n2a = strcmp("23:59",current);
						n2b = strcmp("00:00",current);
						if(n2a>=0)
						{
							if(n<0)
							{
								pre->tc_id = tc[p].id;
								return 1;
							}
						}
						
						if(n2b<=0)
						{
							if(n2>=0)
							{
								pre->tc_id = tc[p].id;
								return 1;
							}
						}
					}
				}
				else
				{
					pre->tc_id = tc[p].id;
					return 1;
				}
			}
			p++;
		}
    }
    return flag;
}
*/

/* new time condition - timestamp compare function */

int tc_ts_cmp(rating_t *pre)
{
    int p;
    int b,n,n2;
    int flag;
    time_t tt;
    time_cond *tc;
    timestamp ts;
    char *hour1;
    char *hour2;
    char *day1;
    char *day2;
    short dweek_ind,dweek_ind_1,dweek_ind_2,dnum_1,dnum_2;

    flag = 0;
	
	memset(&ts,0,sizeof(timestamp));
    
    tc = pre->tc;
    tt = pre->epoch;
    
    if(tc) {		
		if(log_debug_level >= LOG_LEVEL_DEBUG)
			LOG("tc_ts_cmp()","[%s][1]timestamp %s , ts %d",pre->call_uid,pre->timestamp,(int)tt);
		
		parse_tm(tt,&ts);

		if(log_debug_level >= LOG_LEVEL_DEBUG)
			LOG("tc_ts_cmp()","[%s][2]time = %s",pre->call_uid,ts.time);

		p=0;
		while(tc[p].id) {			
			hour1 = strtok(tc[p].hours,"-");
			hour2 = strtok(NULL,"-");
	
			day1 = strtok(tc[p].dweek,"-");
			day2 = strtok(NULL,"-");
	
			dweek_ind = get_week_days_dname(pre->dow);
			
			if(log_debug_level >= LOG_LEVEL_DEBUG) {	
				LOG("tc_ts_cmp()",
					"[%s][3]tc[%d],hour1=%s,time=%s,hour2=%s,dow=%d(%s),date=%s",
					pre->call_uid,p,hour1,ts.time,hour2,pre->dow,week_days[dweek_ind].dname,ts.date);
			}			
			
			/* condition prior 1 - tc_date */
			if(strcmp(tc[p].tc_date,""))
			{
				if(!strcmp(tc[p].tc_date,ts.date)) 
				{
					if(strcmp(tc[p].hours,"")) goto cond_3;
					else 
					{
						pre->tc_id  = tc[p].id;
						pre->tariff_id = tc[p].tariff_id;
						return 1;
					}
				}
			}
				
			dnum_1 = 0;
			dnum_2 = 8;
	
			dweek_ind_1 = get_week_days_dnum(day1);
			dnum_1 = week_days[dweek_ind_1].dnum ;
	
			dweek_ind_2 = get_week_days_dnum(day2);
			dnum_2 = week_days[dweek_ind_2].dnum ;
	
			if(log_debug_level >= LOG_LEVEL_DEBUG)
				LOG("tc_ts_cmp()","[%s][4]week_days %d %d",pre->call_uid,dnum_1,dnum_2);
			
			/* condition prior 2 - days_week */
			if((dnum_1 <= pre->dow)&&(dnum_2 >= pre->dow))
			{
				/* condition prior 3 - hours */
				cond_3:
				if((hour1 != NULL)&&(hour2 != NULL)&&(ts.time != NULL))
				{
					b = strcmp(hour1,hour2);
					n = strcmp(hour1,ts.time);
					n2 = strcmp(hour2,ts.time);
					
					if(log_debug_level >= LOG_LEVEL_DEBUG)
						LOG("tc_ts_cmp()","[%s][5]b=%d,n=%d,n2=%d",pre->call_uid,b,n,n2);
					
					if(b<0)
					{
						if(n<0)
						{
							if(n2>=0) 
							{
								pre->tc_id = tc[p].id;
								pre->tariff_id = tc[p].tariff_id;
								return 1;
							}
						}
					}
					else
					{
						int n2a,n2b;
						n2a = strcmp("23:59:59",ts.time);
						n2b = strcmp("00:00:00",ts.time);
						if(n2a>=0)
						{
							if(n<0)
							{
								pre->tc_id  = tc[p].id;
								pre->tariff_id = tc[p].tariff_id; // dobaveno na 02.10.2015 !
								
								return 1;
							}
						}
						
						if(n2b<=0)
						{
							if(n2>=0)
							{
								pre->tc_id  = tc[p].id;
								pre->tariff_id = tc[p].tariff_id;
								
								return 1;
							}
						}
					}
				}
				else
				{
					pre->tc_id  = tc[p].id;
					pre->tariff_id = tc[p].tariff_id;
					
					return 1;
				}
			}
			p++;
		}
    }
    
    return flag;
}
