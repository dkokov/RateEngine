#ifndef FREE_BILLSEC_H
#define FREE_BILLSEC_H

void f_free_billsec_query(PGconn *conn,rating *pre);
void f_free_billsec_query_2(PGconn *conn,rating *pre);
void f_free_billsec_bal_query(PGconn *conn,rating *pre);
void create_free_billsec_balance_pgsql(PGconn *conn,rating *pre);
void update_free_billsec_balance(PGconn *conn,rating *pre);
void free_billsec_balance(PGconn *conn,rating *pre,char *time1,char *time2);
void free_billsec_balance_v2(PGconn *conn,rating *pre,char *time1,char *time2);
void f_free_bal_id_query(PGconn *conn,rating *pre);
void f_free_billsec_bal_query_2(PGconn *conn,rating *pre);
void free_billsec_exec(PGconn *conn,rating *pre,char *start,char *end);
void free_billsec_sms_balance(PGconn *conn,rating *pre,char *time1,char *time2);

#endif
