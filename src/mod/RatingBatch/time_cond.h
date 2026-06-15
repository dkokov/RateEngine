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

int f_time_cond_query(db_t *dbp,rating_t *pre);
int f_time_cond_query_v2(db_t *dbp,rating_t *pre);
int f_time_cond_query_id(db_t *dbp,rating_t *pre);
int tc_ts_cmp(rating_t *pre);
void parse_ts(char *ts,timestamp *tt);

#endif
