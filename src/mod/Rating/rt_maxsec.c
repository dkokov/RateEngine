#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "rating.h"
#include "pcard.h"
#include "calc_functions.h"

#include "rt_maxsec.h"

int rt_maxsec(db_t *dbp,rating_t *pre)
{
/*	tariff *tr;
    pcard_t *card;
	
	tr = NULL;
	pre = NULL;
	card = NULL;
	
	if(dbp == NULL) {
		LOG("rt_maxsec()","DB no init - pointer is NULL !");
		return RE_ERROR_N;
	}
	
	if(pre == NULL) {
		LOG("rt_maxsec()","Memory alloc error - pointer 'pre' is NULL !");
		return RT_MAXSEC_NO_PRE;
	}
	
	f_bacc_query(dbp,pre);
	chk_bplan_periods(pre);

	if(pre->bacc == 0) {
		LOG("rt_maxsec()","The bacc is not found!");
		pre->maxsec = RT_MAXSEC_NO_BACC;
	} else {
		if(pre->bplan == 0) {
			LOG("rt_maxsec_f()","The bplan is not found!");
			pre->maxsec = RT_MAXSEC_NO_BPLAN;
		} else {					
			pcard_manager_v2(dbp,pre);

			if(pre->card == 0) {
				// Nerazre6eno nabirane poradi neplatena smetka ili dostignat krediten limit! 
				LOG("rt_maxsec_f()","The pcard is not found or actived");
				pre->maxsec = RT_MAXSEC_NO_PCARD;
			} else {
				card = pre->card;
				tr = rate_searching(dbp,pre);						
				pre->limit = ((card->amount)-(pre->balanse));
				
				if(pre->limit <= 0) {
					// Nerazre6eno nabirane poradi dostignat krediten limit 
					pre->maxsec = RT_MAXSEC_NO_CLIMIT;
					
					return RE_ERROR;
				}
				
				if(card->call_number <= 0) return RE_ERROR; */
						
//						sim = cc_server_call_search_racc(pre->clg);
//						LOG("cc_maxsec_f()","clg: %s,call_uid: %s,sim: %d",pre->clg,pre->call_uid,sim);
						
						/* compare sim calls with define call number in the PCard */
/*						if(card->call_number <= sim) {
							LOG("cc_maxsec_f()","call number restriction %s,call number %d,sim %d",
								pre->call_uid,card->call_number,sim);
					
							pre->maxsec = RT_MAXSEC_CRESTICT;
							return RE_ERROR;
						} */
						
						/* one user,a lot of calls */
/*						if(card->call_number > 1) {
							double test,k;
					
							k = (pre->limit/card->amount);
					
							if((k > k_limit_min)) {
								test = ((pre->limit)/((card->call_number)));
								pre->limit = test;
							} else {						
								if((sim) > 0) {
									pre->limit = 0;
									pre->maxsec = MAXSEC_CRESTICT;
						
									return RE_ERROR;
								}
							}
					
							if(pre->limit <= 0) {
								pre->maxsec = MAXSEC_NO_PCARD;
								return RE_ERROR;
							}
							
							LOG("cc_maxsec_f()",
								"PCard data,call_uid: %s,sim: %d,limit/calls: %f,k: %f",
								pre->call_uid,sim,test,k);
						} */
				
	/*			if(tr) {
					calc_maxsec(pre,tr);
				
					if(pre->maxsec <= 0) pre->maxsec = RT_MAXSEC_NO_CREDIT;

					mem_free(tr);
					tr = NULL;
				} else {
					LOG("rt_maxsec_f()","Don't have a tariff");
					pre->maxsec = RT_MAXSEC_NO_TARIFF;	
				}


					}
				}
			}
			
	LOG("rt_maxsec_f()","call_uid: %s,clg: %s,maxsec: %d",pre->call_uid,pre->clg,pre->maxsec);
		
	if(pre->maxsec < 0) {
		if(card != NULL) { 
			mem_free(card);
			LOG("rt_maxsec_f()","mem_free(card)");
		}
					
		if(tr != NULL) {
			mem_free(tr);
			LOG("rt_maxsec_f()","mem_free(tr)");
		}
	}			
	*/
	return RE_SUCCESS;
}
