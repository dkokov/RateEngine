#include "../mod.h"
#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

int reapi_init(void);
int reapi_free(void);

mod_dep_t reapi_mod_dep[] = {
	{"jsonrpc_rt.so",0,1},
	{"",0,0}
};

mod_t reapi_mod_t = {
	.mod_name = "RateEngineAPI",
	.ver      = 1,
	.init     = NULL,
	.destroy  = NULL,
	.depends  = reapi_mod_dep,
	.handle   = NULL,
	.next     = NULL
};
