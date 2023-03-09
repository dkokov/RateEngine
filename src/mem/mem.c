#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "mem.h"

meminfo_t memstat;

void mem_info_clear(void)
{
#if DEBUG_MEM
	memset(&memstat,0,sizeof(meminfo_t));
#endif
}

void mem_stat(void)
{
#if DEBUG_MEM
	MEM_PAGES;
	MEM_USIZE;
#endif
}

void *mem_alloc_arr(int n,size_t mem)
{
	void *ptr;
	
	if(n <= 0) return NULL;
	
	if(mem <= 0) return NULL;
	else {
		ptr = calloc(n,mem);
		
#if DEBUG_MEM
		MEM_MSIZE_INC((mem*n));
		mem_stat();
#endif

	}
	
	return ptr;
}

void *mem_alloc(size_t mem)
{
	return mem_alloc_arr(1,mem);
}

void mem_free(void *ptr)
{
	if(ptr != NULL) {
#if DEBUG_MEM		
		size_t mem;
		mem = malloc_usable_size(ptr);
		MEM_MSIZE_DEC(mem);
		mem_stat();
#endif
		
		free(ptr);
		
		malloc_trim(0);	
	}
}
