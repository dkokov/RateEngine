#include <stdio.h>
#include <pthread.h>

#include "../../db/db.h"

#include "../Rating/rating.h"
#include "../Rating/rt_bind_api.h"

#include "cc.h"
#include "cc_server.h"
#include "cc_bind_api.h"

int cc_bind_api(cc_funcs_t *api)
{
	if(api == NULL) return -1;
	
	api->cc_main    = cc_server_main;
	api->cc_alloc   = cc_alloc;
	api->cc_free    = cc_free;
	api->cc_event_m = cc_event_manager;

	return 0;
}
