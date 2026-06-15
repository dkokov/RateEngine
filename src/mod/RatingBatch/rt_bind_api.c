#include <stdio.h>
#include <pthread.h>

#include "../../db/db.h"

#include "rating.h"
#include "rt_bind_api.h"

#include "rt_maxsec.h"

int rt_bind_api(rt_funcs_t *api)
{
	if(api == NULL) return -1;
	
	api->engine     = RateEngine;
	api->maxsec     = rt_maxsec;
	api->exec       = rt_exec;

	return 0;
}
