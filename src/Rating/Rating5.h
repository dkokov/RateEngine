#ifndef RATING5_H
#define RATING5_H

#define RT_MODE_CLG_NUM     1
#define RT_MODE_ACC_CODE    2
#define RT_MODE_SRC_CONTEXT 3
#define RT_MODE_DST_CONTEXT 4
#define RT_MODE_SRC_TGROUP  5
#define RT_MODE_DST_TGROUP  6

#define RT_MODE_CLG_NUM_SMS 7

int rating_double_rating(PGconn *conn,tariff *tr,rating *pre,char leg);
void rating_exec(PGconn *conn,rating *pre,char leg);
void rating_main(PGconn *conn,rating *pre,char leg,cdr_rec_type_t t);

#endif
