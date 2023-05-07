#ifndef CALC_FUNC_H
#define CALC_FUNC_H

int calculate2(tariff *tr,rating *pre);
int calc_cprice_group(tariff *tr,rating *pre);
void calc_maxsec(PGconn *conn,rating *pre,tariff *tr);
int calc_cprice_sms(tariff *tr,rating *pre);

#endif
