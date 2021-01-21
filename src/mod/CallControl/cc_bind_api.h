#ifndef CC_BIND_API_H
#define CC_BIND_API_H

//#include "../../db/db.h"

typedef void *(*cc_eng_f) (void*);
typedef cc_t* (*cc_alloc_f)(void);
typedef void (*cc_free_f)(cc_t *cc_ptr);
typedef void (*cc_event_manager_f)(cc_t *cc_ptr);

typedef struct cc_funcs {
	cc_eng_f           cc_main;
	cc_alloc_f         cc_alloc;
	cc_free_f          cc_free;
	cc_event_manager_f cc_event_m;
} cc_funcs_t;

#endif
