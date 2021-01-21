#ifndef MEM_H
#define MEM_H

#define MEM_PAGE 4156

typedef struct meminfo {

	/* memory allocator size - malloc,mmap,.. input size */
	unsigned long m_size;
	
	/* memory size in pages */
	unsigned long u_size;
	
	/* memory pages */
	unsigned long u_pages;
} meminfo_t;

#ifdef DEBUG_MEM
	meminfo_t memstat;
	
	#define MEM_MSIZE_DEC(mem) memstat.m_size = memstat.m_size - mem;
	
	#define MEM_MSIZE_INC(mem) memstat.m_size = memstat.m_size + mem;
	
	#define MEM_PAGES memstat.u_pages = (memstat.m_size / MEM_PAGE) + 1;
	
	#define MEM_USIZE memstat.u_size = memstat.u_pages * MEM_PAGE;
#endif

void  mem_info_clear(void);
void  mem_stat(void);
void *mem_alloc(size_t mem);
void *mem_alloc_arr(int n,size_t mem);
void  mem_free(void *ptr);

#endif
