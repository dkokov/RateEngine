#ifndef MOD_H
#define MOD_H

#define MOD_NAME_LEN 64
#define MOD_FUNC_NAME_LEN 128

/* RTLD_NOW or RTLD_LAZY */
#define DL_OPT RTLD_NOW

#include "../config/xml_cfg.h"

typedef int (*mod_func_init_f)(void);
typedef int (*mod_func_destroy_f)(void);

typedef struct mod_dep {
	/* depend module name */
	char dep_mod_name[MOD_NAME_LEN];
	
	/* depend module version */
	unsigned short ver;
	
	/* load dep module : 1(yes),0(no) */
	unsigned short flag;
} mod_dep_t;

typedef struct mod {
	/* module name */
	char mod_name[MOD_NAME_LEN];
	
	/* module version */
	unsigned short ver;	
	
	/* init function in the module */
	mod_func_init_f init;
	
	/* destroy function in the module */
	mod_func_destroy_f destroy;
	
	/* module depends list */
	mod_dep_t *depends;
	
	/* dl(.so) file pointer after file opening */
	void *handle;
		
	/* a pointer to next 'mod struct' from the 'mod list' */
	struct mod *next;
} mod_t;

typedef struct mod_cfg {	
	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
} mod_cfg_t;

int mod_load_modules(char *dirname);
void mod_destroy_modules(void);
mod_t *mod_find_module(char *mod_name);
//void *mod_find_func(void *handle,char *funcname);
void *mod_find_sim(void *handle,char *sim);
void mod_free(void);

#endif
