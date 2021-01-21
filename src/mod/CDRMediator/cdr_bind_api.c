#include <stdio.h>

#include "cdr_bind_api.h"

cdr_funcs_t cdrm_api = {
	.engine     = CDRMediatorEngine,
	.get_cdrs   = cdr_get_cdrs,
	.update_cdr = cdr_update_cdr,
	.add_cdr    = cdr_add_in_db,
	.get_cdr_id = cdr_get_cdr_id
};
