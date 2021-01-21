#ifndef RT_BIND_API_H
#define RT_BIND_API_H

#include "rt_data.h"

typedef void* (*rt_eng_f) (void *dt);
typedef int (*rt_maxsec_f) (db_t *dbp,rating_t *pre);
typedef void (*rt_rating_exec_f)(db_t *dbp,racc_t *rtp,char leg);

typedef struct rt_funcs {
	rt_eng_f         engine;
	rt_maxsec_f      maxsec;
	rt_rating_exec_f exec;
} rt_funcs_t;

#endif
