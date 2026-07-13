#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "../Rating/rating.h"
#include "../Rating/pcard.h"
#include "../Rating/rt_bind_api.h"

#include "cc.h"
#include "cc_server.h"

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

rating_t *cc_maxsec_pre(cc_t *cc_ptr)
{
	rating_t *pre;

	/* core alloc (zeroed) - CallControl must not call Rating-internal
	 * rt_data_rating_init() across the .so boundary */
	pre = mem_alloc(sizeof(rating_t));

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

	rating_t *pre;
	racc_t *rtp;

	pre = NULL;
	rtp = NULL;
	sim = 0;

	if(cc_ptr->max != NULL) {
		if(strlen(cc_ptr->max->clg) > 0) {
			pre = cc_maxsec_pre(cc_ptr);

			if(pre == NULL) {
				LOG("cc_maxsec_f()","Memory alloc error!");
				cc_ptr->max->maxsec = CC_MAXSEC_NO_PRE;
				return;
			}

			/* concurrent calls already up for this subscriber (this call is
			 * not in cc_tbl yet, so it is excluded from the count) */
			sim = cc_server_call_search_racc(pre->clg);
			LOG("cc_maxsec_f()","clg: %s,call_uid: %s,sim: %d",pre->clg,pre->call_uid,sim);

			/* full online maxsec (pcard + rate + time-conditions + credit
			 * limit + sim/call-number restriction + shared-pcard split) */
			rtp = rt_api.maxsec(ccserver.dbp,pre,sim);
		}
	}

	if(pre != NULL) {
		cc_ptr->max->maxsec = pre->maxsec;
		LOG("cc_maxsec_f()","call_uid: %s,clg: %s,maxsec: %d",pre->call_uid,pre->clg,pre->maxsec);

		if(rtp != NULL) {
			/* success (pre->maxsec > 0): keep the racc_t (owning pre) alive
			 * in 'cc_tbl' until 'term' */
			cc_f = cc_server_call_init(cc_ptr,rtp);

			if(cc_f == RE_ERROR) {
				LOG("cc_maxsec_f()","CC Table is full");
				cc_ptr->max->maxsec = CC_MAXSEC_NO_CCTBL;

				rt_api.racc_free(rtp);
				mem_free(pre);
			}
		} else {
			/* failure: rt_api.maxsec() already freed the racc_t, free pre */
			mem_free(pre);
			LOG("cc_maxsec_f()","mem_free(pre)");
		}
	} else cc_ptr->max->maxsec = CC_MAXSEC_NO_PRE;
}

void cc_balance_f(cc_t *cc_ptr)
{
	int bacc_id;
	
	if(cc_ptr != NULL) {
		if(cc_ptr->bal != NULL) {
//			bacc_id = rt_get_bacc_id(ccserver.conn,"calling_number",cc_ptr->cdr_server_id,cc_ptr->bal->clg);
		
			if(bacc_id > 0) {
//				cc_ptr->bal->amount = rt_get_balances(ccserver.conn,bacc_id);
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

void cc_call_rating(cc_t *cc_ptr,racc_t *rtp)
{
	struct timeval times;
	rating_t *pre;

	pre = rtp->pre;

	gettimeofday(&times, NULL);
	pre->start_timer = (((times.tv_sec)*1000000)+(times.tv_usec));

	pre->rating_mode_id = rt_mode_clg;
    pre->billsec = cc_ptr->term->billsec;

	rt_api.exec(ccserver.dbp,rtp,'a');

	LOG("cc_call_rating()","rating_exec(),call_uid: %s",pre->call_uid);

	/* the racc_t (incl. pcard) is freed later by cc_server_call_clear() */

	gettimeofday(&times, NULL);
	pre->current_timer = (((times.tv_sec)*1000000)+(times.tv_usec));
			
	LOG("cc_call_rating()","rating_exec(),call_uid: %s,rating call times: %f",
		pre->call_uid,
		((double)((pre->current_timer)-(pre->start_timer))/1000000));    
}

void cc_term_f(cc_t *cc_ptr)
{
	int i;

	if(cc_ptr->term != NULL) {
		i = cc_server_call_search_call_uid(cc_ptr);
		
		LOG("cc_term_f()","tbl idx: %d",i);
		
		if(i >= 0 ) { 
			cc_server_call_term(&cc_tbl[i],cc_ptr);
		} else if(i == RE_ERROR_N) { 
			LOG("cc_term_f()","[%s],call_uid is not in cc_server",cc_ptr->term->call_uid);
		} else LOG("cc_term_f()","[%s],error",cc_ptr->term->call_uid);
	
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
