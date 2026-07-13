#ifndef RT_BIND_API_H
#define RT_BIND_API_H

#include "rt_data.h"

typedef void* (*rt_eng_f) (void *dt);
/* online maxsec: builds a racc_t (pcard + rate + time-conditions), applies the
 * sim-based shared-pcard split, computes pre->maxsec. 'sim' = concurrent calls
 * for this subscriber (from CallControl). Returns the racc_t on success
 * (pre->maxsec > 0); NULL on failure with a negative RT_MAXSEC_* in pre->maxsec.
 * Caller owns the racc_t + pre. */
typedef racc_t* (*rt_maxsec_f) (db_t *dbp,rating_t *pre,int sim);
typedef void (*rt_rating_exec_f)(db_t *dbp,racc_t *rtp,char leg);
/* free a racc_t obtained from maxsec() - lets a separate module (CallControl)
 * release the racc across the .so boundary without linking Rating internals.
 * Does NOT free rtp->pre (the caller owns pre). */
typedef void (*rt_racc_free_f)(racc_t *rtp);

typedef struct rt_funcs {
	rt_eng_f         engine;
	rt_maxsec_f      maxsec;
	rt_rating_exec_f exec;
	rt_racc_free_f   racc_free;
} rt_funcs_t;

#endif
