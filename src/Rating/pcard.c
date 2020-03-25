#include <time.h>

#include "../misc/globals.h"
#include "../misc/mem/mem.h"

#include "rt_cfg.h"
#include "pcard.h"

void pcard_free(pcard *card)
{
	mem_free(card);
}

pcard *f_pcard_query(PGconn *conn,rating *pre,int pcard_status_id)
{
    PGresult *res;
    pcard *card;

    char str[512];
    int i,ntuples;
    int fnum[10];
    
    card = 0;
    
    sprintf(str,"SELECT id,amount,start_date,end_date,pcard_status_id,pcard_type_id,call_number"
                " from pcard where billing_account_id = %d and pcard_status_id = %d "
                "order by %s %s",
                pre->bacc,pcard_status_id,rt_eng.pcard_sort_key,rt_eng.pcard_sort_mode);

	res = db_pgsql_exec(conn,str);
    if(res != NULL) {
		ntuples = PQntuples(res);
	
		if(ntuples > 0) {
			fnum[0] = PQfnumber(res, "id");
			fnum[1] = PQfnumber(res, "amount");
			fnum[2] = PQfnumber(res, "start_date");
			fnum[3] = PQfnumber(res, "end_date");
			fnum[4] = PQfnumber(res, "pcard_status_id");
			fnum[5] = PQfnumber(res, "pcard_type_id");
			fnum[6] = PQfnumber(res, "call_number");

			card = (pcard *)mem_alloc_arr((ntuples),sizeof(pcard));

			if(card != NULL) {
				for (i = 0; i < ntuples; i++) {	
					card[i].id = atoi(PQgetvalue(res, i, fnum[0]));
					card[i].amount = atof(PQgetvalue(res, i, fnum[1]));
					strcpy(card[i].start,PQgetvalue(res, i, fnum[2]));
					strcpy(card[i].end,PQgetvalue(res, i, fnum[3]));
					card[i].status = atoi(PQgetvalue(res, i, fnum[4]));
					card[i].type = atoi(PQgetvalue(res, i, fnum[5]));
					card[i].call_number = atoi(PQgetvalue(res, i, fnum[6]));
				}
			}
		}
	
		PQclear(res);
	}
    
    return card;
}

void set_pcard_status(PGconn *conn,int id,int status)
{
    PGresult *res;

    char str[512];
    
    sprintf(str,"update pcard set pcard_status_id = %d,last_update = 'now()' "
                "where id = %d ",
                 status,id);
	
    res = db_pgsql_exec(conn,str);
    if(res != NULL) PQclear(res);
}

/*
void pcard_manager(PGconn *conn,rating *pre,int mode)
{
    int i,n,n2,year,next_year,day,mon,mon_1;
    char mon2[2],mon3[2],end2[2],day1[2],bday[2];
    pcard *card;
    char current[11],end_date[11],start_date[11];
    time_t tt;
    struct tm  *tm;
    
    card = 0;
    
    if(mode == 1) {
		card = f_pcard_query(conn,pre,1);
		
		if(card > 0) {
			if(pre->epoch) tt = pre->epoch;
			else tt = pre->ts;
			
			tm = localtime(&tt);

			year = TIMEYEAR(tm->tm_year);

			mon = (tm->tm_mon+1);
			TIMESTR(mon,mon2);

			day = tm->tm_mday;
			TIMESTR(day,day1);

			TIMESTR((tm->tm_mon),mon3);
					
			if(strlen(pre->billing_day)) strcpy(bday,pre->billing_day);
			else if(strlen(billing_day)) strcpy(bday,billing_day);
			else strcpy(bday,BILLING_DAY);
						
			mon_1 = mon+1;
			
			if(mon_1 < 10) {
				sprintf(end2,"0%d",mon_1);
				next_year = year;
			} else {
				if(mon_1 <= 12) {
					sprintf(end2,"%d",mon_1);
					next_year = year;
				} else {
					sprintf(end2,"01");
					next_year = year+1;
				}
			}
		
			sprintf(current,"%d-%s-%s",year,mon2,day1);

			if(atoi(day1) >= atoi(bday)) {
				sprintf(start_date,"%d-%s-%s",year,mon2,bday);
				sprintf(end_date,"%d-%s-%s",next_year,end2,bday);				
			} else {
				if(!strcmp(mon2,"01")) sprintf(start_date,"%d-12-%s",(year-1),bday);
				else sprintf(start_date,"%d-%s-%s",year,mon3,bday);
				
				sprintf(end_date,"%d-%s-%s",year,mon2,bday);
			}
						
			i=0;
			while(card[i].id) {
				if(card[i].status == PCARD_ACTIVE) {				
					if(!strcmp(card[i].end,"")) strcpy(card[i].end,end_date);
					
					if(card[i].type == CREDIT_CARD) {	
						n2 = (strcmp(card[i].start,start_date));
						
						if(n2 < 0) strcpy(card[i].start,start_date);
					}				
				
					n = (strcmp(card[i].start,current));
					if(n <= 0) {
						if(((strcmp(card[i].end,current)) > 0)) {
							//pre->pcard = card[i].id;
							pre->card = &card[i];
							break;
						} else bzero(card[i].end,sizeof(card[i].end));
					} else bzero(card[i].start,sizeof(card[i].start));
				}
				i++;
			}
		
			if(log_debug_level >= LOG_LEVEL_DEBUG) {
				LOG("PCardManager","call_uid %s,card_id %d,start %s,current %s,end %s",
					pre->call_uid,card[i].id,card[i].start,current,card[i].end);
			}
		}
    }
    
    pre->card = card; 
}*/

void pcard_manager_v2(PGconn *conn,rating *pre)
{
    int i,n,n2,year,next_year,day,mon,mon_1,end,prev_mon;    
    char current[12],end_date[12],start_date[12],bday[3];
    
    time_t tt;
    struct tm  *tm;
    
    pcard *card = 0;
    
    /* get only active pcards for this billing_account */
	card = f_pcard_query(conn,pre,PCARD_ACTIVE);
		
	if(card > 0) {
		if(pre->epoch) tt = pre->epoch;
		else tt = pre->ts;
		
		memset(current,0,sizeof(current));
		memset(start_date,0,sizeof(start_date));
		memset(end_date,0,sizeof(end_date));
		memset(bday,0,sizeof(bday));
		
		/* Current localtime */
		tm = localtime(&tt);

		/* Current year */
		year = (tm->tm_year+1900);

		/* Current month */
		mon = (tm->tm_mon+1);

		/* Current day */
		day = tm->tm_mday;

		/* Prev month */
		prev_mon = tm->tm_mon;
					
		/* Billing Day */
		if(strlen(pre->billing_day)) strcpy(bday,pre->billing_day);
		else if(strlen(billing_day)) strcpy(bday,billing_day);
		else strcpy(bday,BILLING_DAY);
						
		/* Define next month and next year */
		mon_1 = mon+1;
			
		if(mon_1 < 10) {
			end = mon_1;
			next_year = year;
		} else {
			if(mon_1 <= 12) {
				end = mon_1;
				next_year = year;
			} else {
				end = 1;
				next_year = year+1;
			}
		}

		/* Current timestamp */
		sprintf(current,"%d-%.2d-%.2d",year,mon,day);

		/* Compare current and billing days + define start/end periods */
		if(day >= atoi(bday)) {			
			sprintf(start_date,"%d-%.2d-%s",year,mon,bday);
			sprintf(end_date,"%d-%.2d-%s",next_year,end,bday);
		} else {
			if(mon == 1) sprintf(start_date,"%d-12-%s",(year-1),bday);
			else sprintf(start_date,"%d-%.2d-%s",year,prev_mon,bday);
				
			sprintf(end_date,"%d-%.2d-%s",year,mon,bday);
		}
						
		/* Compare start/end periods */
		i=0;
		while(card[i].id) {
			if(card[i].type == CREDIT_CARD) {
				/* credit card */				
				if(!strcmp(card[i].end,"")) strcpy(card[i].end,end_date);
					
				n2 = (strcmp(card[i].start,start_date));
				if(n2 < 0) strcpy(card[i].start,start_date);
			} else if(card[i].type == DEBIT_CARD) {
				/* debit card */
				if((strlen(card[i].end) == 0)||(strlen(card[i].start) == 0)) {
					i++;
					continue;
				}
			} else {
				/* unkn type card */
				LOG("PCardManagerV2","UNKN TYPE CARD(type_id:%d),card_id: %d",card[i].type,card[i].id);
				i++;
				continue;
			}		
			
			n = (strcmp(card[i].start,current));
			if(n <= 0) {
				if(((strcmp(card[i].end,current)) > 0)) {
					pre->card = (pcard *)mem_alloc(sizeof(pcard));
					
					if(pre->card) {
						memcpy(pre->card,&card[i],sizeof(pcard));
						
						if((log_debug_level >= LOG_LEVEL_DEBUG)) {
							LOG("PCardManagerV2","Call UID: %s,PCard ID: %d,"
							"Start Date(PCard Active Period): %s,Current Date: %s,End Date(PCard Active Period):%s",
							pre->call_uid,pre->card->id,pre->card->start,current,pre->card->end);
						}
					} else LOG("PCardManagerV2","memory error");
								
					break;
				}
			}
			
			if((log_debug_level >= LOG_LEVEL_DEBUG)) {
				LOG("PCardManagerV2","Out of PCard Active Period,PCard ID: %d,Start: %s,Current: %s,End: %s",
				card[i].id,card[i].start,current,card[i].end);
			}
			
			i++;
		}
		
		mem_free(card);
		
	} else {
		pre->card = 0;
		LOG("PCardManagerV2","The PCard is NULL!");
	}
}
