#include <stdio.h>

#include "../../misc/globals.h"
#include "../../db/db.h"
#include "../../mem/mem.h"
#include "../mod.h"

int test2_func(char *tt)
{
	if(tt == NULL) {
		LOG("test2_func()","tt pointer is NULL");
		return RE_ERROR_N;
	}
	
	LOG("test2_func()","tt: %s",tt);
	
	return RE_SUCCESS;
}



