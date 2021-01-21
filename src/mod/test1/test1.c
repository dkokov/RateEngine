#include <stdio.h>

#include "../../misc/globals.h"
#include "../../db/db.h"
#include "../../mem/mem.h"
#include "../mod.h"


int test1_test2_init(char *tt)
{
	int ret;
	void *func;
	mod_t *mod_ptr;
	int (*fptr)(char *);

	mod_ptr = mod_find_module("test2.so");

	if(mod_ptr == NULL) return RE_ERROR_N;
	if(mod_ptr->handle == NULL) return RE_ERROR_N;
	
	func = mod_find_func(mod_ptr->handle,"test2_func");
	if(func != NULL) {
		fptr = func;
			
		ret = fptr(tt);
		if(ret < 0) {
			LOG("test1_test2_init()","test2_func(),ret: %d",ret);
			return RE_ERROR_N;
		}
	}

	return RE_SUCCESS;	
}

int test1_mod_init(void)
{
	char *buf = strdup("Dimitar Kokov");
	
	test1_test2_init(buf);
	
	free(buf);
	
	return RE_SUCCESS;
}



