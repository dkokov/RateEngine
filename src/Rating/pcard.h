#ifndef PCARD_H
#define PCARD_H

#define DATE_LEN 32

#define DEBIT_CARD  1
#define CREDIT_CARD 2

#define PCARD_ACTIVE   1
#define PCARD_DEACTIVE 0
#define PCARD_BLOCK    2

typedef struct pcard
{
    int id;
    double amount;
    char start[DATE_LEN];
    char end[DATE_LEN];
    int status;
    int bacc;
    int type;
    int call_number;
}pcard;

//void pcard_manager(PGconn *conn,rating *pre,int mode);
void pcard_manager_v2(PGconn *conn,rating *pre);
void pcard_free(pcard *card);
void set_pcard_status(PGconn *conn,int id,int status);

#endif
