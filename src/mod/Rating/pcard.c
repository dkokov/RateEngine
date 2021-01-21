#include <time.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "rt_cfg.h"
#include "rt_data.h"
#include "rt_data_q.h"
#include "pcard.h"

void pcard_free(pcard_t *card)
{
	mem_free(card);
}

void pcard_manager(db_t *dbp,racc_t *rtp)
{
    int i,n,n2,year,next_year,day,mon,mon_1,end,prev_mon;    
    char current[12],end_date[12],start_date[12];
    
    int bday;
    
    time_t tt;
    struct tm  *tm;
    
    rating_t *pre;
    bacc_t *bpt;
    
    pre = rtp->pre;
    bpt = rtp->bacc_ptr;
    
    pcard_t *card,*tmp;
    
    /* get only active pcards for this billing_account */	
	rt_data_q_pcard(dbp,bpt,pcard_active);
	card = bpt->pcard_ptr;
	
	tmp = NULL;
	if(card != NULL) {
		if(pre->epoch) tt = pre->epoch;
		else tt = pre->ts;
		
		memset(current,0,sizeof(current));
		memset(start_date,0,sizeof(start_date));
		memset(end_date,0,sizeof(end_date));
		
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
		if(bpt->billing_day > 0) bday = bpt->billing_day;
		else bday = 1;
				
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
		if(day >= bday) {			
			sprintf(start_date,"%d-%.2d-%.2d",year,mon,bday);
			sprintf(end_date,"%d-%.2d-%.2d",next_year,end,bday);
		} else {
			if(mon == 1) sprintf(start_date,"%d-12-%.2d",(year-1),bday);
			else sprintf(start_date,"%d-%.2d-%.2d",year,prev_mon,bday);
				
			sprintf(end_date,"%d-%.2d-%.2d",year,mon,bday);
		}
						
		/* Compare start/end periods */
		i=0;
		while(card[i].id) {
			if(card[i].type == credit_card) {
				/* credit card */	
				if(!strcmp(card[i].end,"")) strcpy(card[i].end,end_date);
					
				n2 = (strcmp(card[i].start,start_date));
				if(n2 < 0) strcpy(card[i].start,start_date);
			} else if(card[i].type == debit_card) {
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
					tmp = (pcard_t *)mem_alloc(sizeof(pcard_t));
					
					if(tmp) {
						memcpy(tmp,&card[i],sizeof(pcard_t));
						
						if((log_debug_level >= LOG_LEVEL_DEBUG)) {
							LOG("PCardManagerV2","Call UID: %s,PCard ID: %d,"
							"Start Date(PCard Active Period): %s,Current Date: %s,End Date(PCard Active Period):%s",
							pre->call_uid,tmp->id,tmp->start,current,tmp->end);
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
		
		bpt->pcard_ptr = tmp;
		mem_free(card);
	} else {
		bpt->pcard_ptr = NULL;
		LOG("PCardManagerV2","The PCard is NULL!");
	}
}
