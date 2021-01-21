#include "../misc/globals.h"
#include "../misc/mem/mem.h"

#include "cc.h"
#include "cc_server.h"

#include "../Rating/Rating5.h"

#include <pthread.h>
#include <unistd.h>

cc_t *cc_alloc(void)
{
	cc_t *cc_ptr = NULL;

	cc_ptr = (cc_t *)mem_alloc(sizeof(cc_t));
	
	if(cc_ptr != NULL) {
		cc_ptr->cc_status = CC_STATUS_ACTIVE;
	}
	
	return cc_ptr;
}

void cc_free(cc_t *cc_ptr)
{
	if(cc_ptr != NULL) {
		if(cc_ptr->cc_status == CC_STATUS_DEACTIVE) {
			if(cc_ptr->max != NULL) {
				LOG("cc_free()","free 'cc_ptr->max',call_uid: %s",cc_ptr->max->call_uid);
				mem_free(cc_ptr->max);
				cc_ptr->max = NULL;
			}
			
			if(cc_ptr->bal != NULL) {
				mem_free(cc_ptr->bal);
				cc_ptr->bal = NULL;
			}
			
			if(cc_ptr->cprice != NULL) {
				mem_free(cc_ptr->cprice);
				cc_ptr->cprice = NULL;
			}
			
			if(cc_ptr->rate != NULL) {
				mem_free(cc_ptr->rate);
				cc_ptr->rate = NULL;
			}
			
			if(cc_ptr->term != NULL) {
				LOG("cc_free()","free 'cc_ptr->term',call_uid: %s",cc_ptr->term->call_uid);
				mem_free(cc_ptr->term);
				cc_ptr->term = NULL;
			}
			
			LOG("cc_free()","free 'cc_ptr'");
			
			mem_free(cc_ptr);
			cc_ptr = NULL;
		}
	}
}

rating *cc_maxsec_pre(cc_t *cc_ptr)
{
	rating *pre;
	
	pre = (rating *)mem_alloc(sizeof(rating));
	
	if(pre != NULL) {		
		pre->cdr_server_id = cc_ptr->cdr_server_id;
		strcpy(pre->clg,cc_ptr->max->clg);
		strcpy(pre->cld,cc_ptr->max->cld);
		strcpy(pre->call_uid,cc_ptr->max->call_uid);
		pre->ts = cc_ptr->max->ts;
	}
	
	return pre;
}

void cc_maxsec_f(cc_t *cc_ptr)
{
	int cc_f,sim;
	tariff *tr;
    pcard *card;
    rating *pre;
	
	tr = NULL;
	pre = NULL;
	card = NULL;
	
	if(cc_ptr->max != NULL) {
		if(strlen(cc_ptr->max->clg) > 0) {
			pre = cc_maxsec_pre(cc_ptr);
			
			db_pgsql_res_clear(db_pgsql_exec(ccserver.conn,"BEGIN"));
			
			if(pre == NULL) {
				LOG("cc_maxsec_f()","Memory alloc error!");
				cc_ptr->max->maxsec = CC_MAXSEC_NO_PRE;
				goto end_func;
			}

			sim = 0;
						
			f_bacc_query(ccserver.conn,pre);
			chk_bplan_periods(pre);

			if(pre->bacc == 0) {
				LOG("cc_maxsec_f()","The bacc is not found!");
				pre->maxsec = CC_MAXSEC_NO_BACC;
			} else {
				if(pre->bplan == 0) {
					LOG("cc_maxsec_f()","The bplan is not found!");
					pre->maxsec = CC_MAXSEC_NO_BPLAN;
				} else {					
					pcard_manager_v2(ccserver.conn,pre);
					
					if(pre->card == 0) {
						/* Nerazre6eno nabirane poradi neplatena smetka ili dostignat krediten limit! */
						LOG("cc_maxsec_f()","The pcard is not found or actived");
						pre->maxsec = CC_MAXSEC_NO_PCARD;
					} else {
						card = pre->card;
						tr = rate_searching(ccserver.conn,pre);
						pre->limit = ((card->amount)-(pre->balanse));
				
						if(pre->limit <= 0) {
							/* Nerazre6eno nabirane poradi dostignat krediten limit */
							pre->maxsec = CC_MAXSEC_NO_CLIMIT;
							goto end_func;
						}
				
						if(card->call_number <= 0) goto end_func;
						
						sim = cc_server_call_search_racc(pre->clg);
						LOG("cc_maxsec_f()","clg: %s,call_uid: %s,sim: %d",pre->clg,pre->call_uid,sim);
						
						/* compare sim calls with define call number in the PCard */
						if(card->call_number <= sim) {
							LOG("cc_maxsec_f()","call number restriction %s,call number %d,sim %d",
								pre->call_uid,card->call_number,sim);
					
							pre->maxsec = CC_MAXSEC_CRESTICT;
							goto end_func;
						}
						
						/* one user,a lot of calls */
						if(card->call_number > 1) {
							double test,k;
					
							k = (pre->limit/card->amount);
					
							if((k > k_limit_min)) {
								test = ((pre->limit)/((card->call_number)));
								pre->limit = test;
							} else {						
								if((sim) > 0) {
									pre->limit = 0;
									pre->maxsec = CC_MAXSEC_CRESTICT;
						
									goto end_func;
								}
							}
					
							if(pre->limit <= 0) {
								pre->maxsec = CC_MAXSEC_NO_PCARD;
								goto end_func;
							}
							
							LOG("cc_maxsec_f()",
								"PCard data,call_uid: %s,sim: %d,limit/calls: %f,k: %f",
								pre->call_uid,sim,test,k);
						}
				
						if(tr) {
							//calc_maxsec(ccserver.conn,pre,tr);
							calc_maxsec(pre,tr);
							
							if(pre->maxsec <= 0) pre->maxsec = CC_MAXSEC_NO_CREDIT;

							mem_free(tr);
							tr = NULL;
						} else {
							LOG("cc_maxsec_f()","Don't have a tariff");
							pre->maxsec = CC_MAXSEC_NO_TARIFF;	
						}
					}
				}
			}
		end_func:
			db_pgsql_res_clear(db_pgsql_exec(ccserver.conn,"COMMIT"));
			
			if(pre != NULL) { 
				cc_ptr->max->maxsec = pre->maxsec;
				LOG("cc_maxsec_f()","call_uid: %s,clg: %s,maxsec: %d",pre->call_uid,pre->clg,pre->maxsec);
				
				if(cc_ptr->max->maxsec > 0) {
					/* save 'pre' and 'cc_ptr' pointers in the 'cc_tbl' */
					cc_f = cc_server_call_init(cc_ptr,pre);
			
					if(cc_f == RE_ERROR) {
						LOG("cc_maxsec_f()","CC Table is full");
						cc_ptr->max->maxsec = CC_MAXSEC_NO_CCTBL;
					}
				} else {
					if(card != NULL) { 
						mem_free(card);
						LOG("cc_maxsec_f()","mem_free(card)");
					}
					
					if(tr != NULL) {
						mem_free(tr);
						LOG("cc_maxsec_f()","mem_free(tr)");
					}
					
					mem_free(pre);
					LOG("cc_maxsec_f()","mem_free(pre)");
				}			
			} else cc_ptr->max->maxsec = CC_MAXSEC_NO_PRE;
		}
	} 
}

void cc_balance_f(cc_t *cc_ptr)
{
	int bacc_id;
	
	if(cc_ptr != NULL) {
		if(cc_ptr->bal != NULL) {
			bacc_id = rt_get_bacc_id(ccserver.conn,"calling_number",cc_ptr->cdr_server_id,cc_ptr->bal->clg);
		
			if(bacc_id > 0) {
				cc_ptr->bal->amount = rt_get_balances(ccserver.conn,bacc_id);
			}
		}
		
		cc_ptr->cc_status = CC_STATUS_DEACTIVE;
	}
}

void cc_rate_f(cc_t *cc_ptr)
{
	if(cc_ptr != NULL) {
		if(cc_ptr->rate != NULL) {
			
		}
	
		cc_ptr->cc_status = CC_STATUS_DEACTIVE;
	}
}

void cc_cprice_f(cc_t *cc_ptr)
{
	if(cc_ptr != NULL) {
		if(cc_ptr->cprice != NULL) {
			
		}
	
		cc_ptr->cc_status = CC_STATUS_DEACTIVE;
	}
}

void cc_call_rating(cc_t *cc_ptr,rating *pre)
{
	pre->rating_mode_id = 1;
    pre->billsec = cc_ptr->term->billsec;
    
	rating_exec(ccserver.conn2,pre,'a');

	LOG("cc_call_rating()","rating_exec(),call_uid: %s",pre->call_uid);
	
	mem_free(pre->card);
    mem_free(pre->tc);
}

void cc_term_f(cc_t *cc_ptr)
{
	int i;

	if(cc_ptr->term != NULL) {
		i = cc_server_call_search_call_uid(cc_ptr);
		
		LOG("cc_term_f()","tbl idx: %d",i);
		
		if(i >= 0 ) cc_server_call_term(&cc_tbl[i],cc_ptr);
		else if(i == RE_ERROR_N) LOG("cc_term_f()","[%s],call_uid is not in cc_server",cc_ptr->term->call_uid);
		else LOG("cc_term_f()","[%s],error",cc_ptr->term->call_uid);
	
		cc_ptr->term->tbl_idx = i;
	}
}

void cc_event_manager(cc_t *cc_ptr)
{	
	switch(cc_ptr->t) {
		case maxsec_event:
					cc_maxsec_f(cc_ptr);
					//cc_ptr->max->maxsec = 3662;
					break;
		case balance_event:
					cc_balance_f(cc_ptr);
					break;
		case rate_event:
					cc_rate_f(cc_ptr);
					break;
		case cprice_event:
					cc_cprice_f(cc_ptr);
					break;
		case term_event:
					cc_term_f(cc_ptr);
					break;
		case unkn_event:
		default:
					break;
	};
}
