#ifndef RT_MAXSEC_H
#define RT_MAXSEC_H

#define RT_MAXSEC_NO_BACC   -1
#define RT_MAXSEC_NO_BPLAN  -2
#define RT_MAXSEC_NO_PCARD  -3
#define RT_MAXSEC_NO_CLIMIT -4
#define RT_MAXSEC_CRESTICT  -6
#define RT_MAXSEC_NO_TARIFF -7
#define RT_MAXSEC_NO_CREDIT -8
#define RT_MAXSEC_NO_PRE    -9
#define RT_MAXSEC_NO_CCTBL  -10

int rt_maxsec(db_t *dbp,rating_t *pre);

#endif 
