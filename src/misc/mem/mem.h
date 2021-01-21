#ifndef MEM_H
#define MEM_H

typedef struct meminfo {
	
	unsigned long f_size;
	unsigned long u_size;
	
}meminfo_t;

#ifdef DEBUG_MEM
	meminfo_t memstat;
	
	#define MEM_FSIZE_DEC(mem) memstat.f_size = memstat.f_size - mem;
	
	#define MEM_FSIZE_INC(mem) memstat.f_size = memstat.f_size + mem;
#endif

void mem_info_clear(void);
void *mem_alloc(size_t mem);
void *mem_alloc_arr(int n,size_t mem);
void mem_free(void *ptr);

#endif
