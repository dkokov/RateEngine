
* 2016-03-22 , варинт 1 *

- опашки :
	1. инициализира се нов възел (node)
	2. инициализират се нови данни (data) и се въвежда информация
	3. новият възел се добавя в опашката - в началото(put first) или в края(put last) 


* 2016-06-14 *
http://www.canonware.com/jemalloc/index.html

* 2016-06-15 *

* 2016-06-16 *

- standart malloc/free (glibc) : buddy memory allocation scheme

- kamailio :
	http://tlsf.baisoku.org , 
	TLSF - Two Level Segregated Fit memory allocator implementation

- freeswitch :
	apr : http://apr.apache.org/ , Apache Portable Runtime

- redis :
	jemalloc

**********************************
move from mem.c :

/*
 * http://www.tldp.org/LDP/tlk/mm/memory.html
 * https://www.win.tue.nl/~aeb/linux/lk/lk-9.html
 * 
 * http://www.roman10.net/2011/07/29/linux-kernel-programmingmemory-allocation/
 * 
 * http://www.gnu.org/software/libc/manual/html_node/Basic-Allocation.html
 * 
 * malloc,calloc,realloc,free ???
 *  
 * 
 * pmap pid -> view process memory map 
 *
 *
 * http://stackoverflow.com/questions/4863707/how-to-see-linux-view-of-the-ram-in-order-to-determinate-the-fragmentation
 * http://serverfault.com/questions/133305/linux-memory-fragmentation
 * https://lwn.net/Articles/211505/
 * 
 * http://learnlinuxconcepts.blogspot.bg/2014/02/linux-memory-management.html
 * 
 * http://www.intel.com/content/www/us/en/processors/architectures-software-developer-manuals.html
 * 
 * mtrace(),muntrace()
 * mcheck(),mprobe()
 * Valgrind / Insure++
 
   valgrind --tool=memcheck ls -l
 
 * 
 * mallopt(),mallinfo()
 * sbrk(),mmap()
 * memalign(),posix_memalign()
 * alloca()
 * 
 * 

Allocation approach 
	Pros 

	Cons
   
malloc() 
	Easy, simple, common. 
	 
	Returned memory not necessarily zeroed.
   
calloc() 
	Makes allocating arrays simple, zeros returned memory.

	Convoluted interface if not allocating arrays.

realloc() 
	Resizes existing allocations. 
	
	Useful only for resizing existing allocations.

brk() and sbrk() 
	Provides intimate control over the heap. Much too low-level for most users.
	Anonymous memory mappings Easy to work with,sharable,allow
	developer to adjust protection level and provide advice; optimal for large mappings.

	Suboptimal for small allocations; malloc() automatically uses anonymous memory mappings when optimal.

posix_memalign() 
	Allocates memory aligned to any reasonable boundary.
	
	Relatively new and thus portability is questionable; 
	overkill unless alignment concerns are pressing.

memalign() and valloc() 
	More common on other Unix systems than posix_memalign().

	Not a POSIX standard, offers less alignment control than posix_memalign().

alloca() 
	Very fast allocation, no need to explicitly free memory; 
	great for small allocations.

	Unable to return error, no good for large
	allocations, broken on some Unix systems.

Variable-length arrays 
	Same as alloca(),but frees memory when array falls out of scope, not when function returns.
	
	Useful only for arrays; alloca() freeing behavior may be preferable in some situations; 
	less common on other Unix systems than alloca().

 */

* 2017-08-03 *
https://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html

*** 2017-11-18 ***
https://techtalk.intersec.com/2013/07/memory-part-1-memory-types/
...
Anonymous Memory

Anonymous memory is purely in RAM. 
However, the kernel will not actually map that memory to a physical address before it gets actually written. 
As a consequence, anonymous memory does not add any pressure on the kernel before it is actually used. 
This allows a process to “reserve” a lot of memory in its virtual memory address space without really using RAM. 
As a consequence, the kernel lets you reserve more memory than actually available. 
This behavior is often referenced as over-commit (or memory overcommitment).
...

Добра статия !!!
****
