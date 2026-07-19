#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "rating.h"
#include "pcard.h"
#include "calc_functions.h"

#include "rt_maxsec.h"

/*
 * Online-charging max-seconds computation for CallControl.
 *
 * This is the full computation 0.6.14's monolithic cc_maxsec_f() performed,
 * relocated into the Rating module so CallControl reaches it via the single
 * rt_maxsec bind entry. It reuses the same functions as the batch path, so
 * pcards and time-conditions behave identically:
 *   - rt_racc_voip_av_a() : billing account + bill plan
 *   - pcard_manager()     : active payment card  -> bacc_ptr->pcard_ptr
 *   - rt_rate_searching() : rate/tariff + time-conditions + consumed balance
 *   - calc_maxsec()       : seconds, from the (possibly split) credit limit
 *
 * 'sim' is the number of simultaneous calls already up for this subscriber,
 * supplied by CallControl (it lives in the cc_tbl / CallControl plane).
 *
 * Returns the racc_t on success (pre->maxsec > 0); the caller owns it, holds
 * it until 'term', rates the call with rt_exec(), then frees it with
 * rt_data_racc_free(). pre is owned by the caller and never freed here.
 * On failure returns NULL and sets pre->maxsec to a negative RT_MAXSEC_* code.
 */
racc_t *rt_maxsec(db_t *dbp,rating_t *pre,int sim)
{
	racc_t *rtp;
	pcard_t *card;

	if(dbp == NULL) return NULL;
	if(pre == NULL) return NULL;

	/* ensure the shared reference cache exists (online charging otherwise runs
	 * with rt_eng.cache == NULL and re-queries all static data every call) */
	rt_cache_ensure();

	/* billing account + bill plan (voip leg-a, online) */
	rtp = rt_racc_voip_av_a(dbp,pre);

	if(rtp == NULL) {
		pre->maxsec = RT_MAXSEC_NO_BACC;
		return NULL;
	}

	rtp->pre = pre;

	if((rtp->bacc_ptr == NULL)||(rtp->bacc_ptr->id == 0)) {
		pre->maxsec = RT_MAXSEC_NO_BACC;
		rt_data_racc_free(rtp);
		return NULL;
	}

	if((rtp->bplan_ptr == NULL)||(rtp->bplan_ptr->id == 0)) {
		pre->maxsec = RT_MAXSEC_NO_BPLAN;
		rt_data_racc_free(rtp);
		return NULL;
	}

	rt_chk_bplan_periods(rtp->bplan_ptr,pre->ts);

	/* active payment card */
	pcard_manager(dbp,rtp);

	card = rtp->bacc_ptr->pcard_ptr;

	if(card == NULL) {
		pre->maxsec = RT_MAXSEC_NO_PCARD;
		rt_data_racc_free(rtp);
		return NULL;
	}

	/* rate/tariff + time-conditions; also reads consumed balance */
	rt_rate_searching(dbp,rtp);

	if((rtp->bplan_ptr->rates_ptr == NULL)||
	   (rtp->bplan_ptr->rates_ptr->calc_funcs == NULL)) {
		pre->maxsec = RT_MAXSEC_NO_TARIFF;
		rt_data_racc_free(rtp);
		return NULL;
	}

	/* remaining credit = card credit limit - already consumed balance */
	pre->limit = (card->amount) - (rtp->bal_ptr->amount);

	if(pre->limit <= 0) {
		pre->maxsec = RT_MAXSEC_NO_CLIMIT;
		rt_data_racc_free(rtp);
		return NULL;
	}

	/* no simultaneous calls permitted for this pcard */
	if(card->call_number <= 0) {
		pre->maxsec = RT_MAXSEC_CRESTICT;
		rt_data_racc_free(rtp);
		return NULL;
	}

	/* already at/over the allowed number of concurrent calls */
	if(card->call_number <= sim) {
		LOG("rt_maxsec()","call number restriction %s,call number %d,sim %d",
			pre->call_uid,card->call_number,sim);
		pre->maxsec = RT_MAXSEC_CRESTICT;
		rt_data_racc_free(rtp);
		return NULL;
	}

	/* one user, several simultaneous calls: split the shared credit limit
	 * across the allowed number of calls, gated by k_limit_min */
	if(card->call_number > 1) {
		double test,k;

		test = 0;
		k = (pre->limit / card->amount);

		if(k > k_limit_min) {
			test = (pre->limit) / (card->call_number);
			pre->limit = test;
		} else {
			if(sim > 0) {
				pre->limit = 0;
				pre->maxsec = RT_MAXSEC_CRESTICT;
				rt_data_racc_free(rtp);
				return NULL;
			}
		}

		if(pre->limit <= 0) {
			pre->maxsec = RT_MAXSEC_NO_PCARD;
			rt_data_racc_free(rtp);
			return NULL;
		}

		LOG("rt_maxsec()","PCard data,call_uid: %s,sim: %d,limit/calls: %f,k: %f",
			pre->call_uid,sim,test,k);
	}

	/* seconds from the (possibly split) credit limit; AFTER the split so
	 * maxsec reflects the divided credit (0.6.14 ordering) */
	calc_maxsec(rtp);

	if(pre->maxsec <= 0) {
		pre->maxsec = RT_MAXSEC_NO_CREDIT;
		rt_data_racc_free(rtp);
		return NULL;
	}

	LOG("rt_maxsec()","call_uid: %s,clg: %s,limit: %f,maxsec: %d",
		pre->call_uid,pre->clg,pre->limit,pre->maxsec);

	return rtp;
}
