#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "mem.h"

void mem_info_clear(void)
{
#if DEBUG_MEM
	memset(&memstat,0,sizeof(meminfo_t));
#endif
}

void *mem_alloc(size_t mem)
{
	void *ptr;

	if(mem <= 0) return NULL;
	else {
		ptr = malloc(mem);
		
		if(ptr != NULL) {
			memset(ptr,0,mem);
			
#if DEBUG_MEM
			MEM_FSIZE_INC(mem);
#endif
		}
	
		return ptr;
	}
} 

void *mem_alloc_arr(int n,size_t mem)
{
	void *ptr;
	
	if(n <= 0) return NULL;
	
	if(mem <= 0) return NULL;
	else {
		ptr = calloc(n,mem);
		
#if DEBUG_MEM
		MEM_FSIZE_INC((mem*n));
#endif

	}
	
	return ptr;
}

void mem_free(void *ptr)
{
	size_t mem;
	
	if(ptr != NULL) {
		mem = malloc_usable_size(ptr);

#if DEBUG_MEM		
		MEM_FSIZE_DEC(mem);
#endif
		free(ptr);
		
		malloc_trim(0);
	}
}
