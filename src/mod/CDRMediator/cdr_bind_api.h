#ifndef CDR_BIND_API_H
#define CDR_BIND_API_H

#include "../../db/db.h"

#include "prefix_filter.h"
#include "cdr.h"
#include "cdr_mediator.h"

typedef void* (*cdr_mediator_eng_f) (void *dt);
typedef cdr_t* (*cdr_get_cdrs_f) (db_t *dbp,char leg,int dig);
typedef void (*cdr_update_cdr_f) (db_t *dbp,int rating_id,int cdr_id,char leg,char *call_uid);
typedef int (*cdr_add_in_db_f) (db_t *dbp,cdr_t *cdr_pt,filter *filters);
typedef int (*cdr_get_cdr_id_f)(db_t *dbp,cdr_t *the_cdr);

typedef struct cdr_funcs {
	cdr_mediator_eng_f engine;
	cdr_get_cdrs_f     get_cdrs;
	cdr_update_cdr_f   update_cdr;
	cdr_add_in_db_f    add_cdr;
	cdr_get_cdr_id_f   get_cdr_id;
} cdr_funcs_t;

#endif
