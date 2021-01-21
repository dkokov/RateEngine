#ifndef TIME_COND_H
#define TIME_COND_H

typedef struct timestamp
{
    char date[11];
    char time[9];
    char timezone[4];
}timestamp;

typedef struct time_cond
{
    int id;
    int tariff_id;
    char hours[12];
    char dweek[8];
    char tc_date[11];
    char tc[64];
}time_cond;

void f_time_cond_query(PGconn *conn,rating *pre);
void f_time_cond_query_v2(PGconn *conn,rating *pre);
int f_time_cond_query_id(PGconn *conn,rating *pre);
int tc_ts_cmp(rating *pre);
void parse_ts(char *ts,timestamp *tt);

#endif
