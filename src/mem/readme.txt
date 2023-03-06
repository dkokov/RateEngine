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
	
**********************************

- standart malloc/free (glibc) : buddy memory allocation scheme

- opensips : mem/f_malloc,hp_malloc,q_malloc,vq_malloc,shm_mem ... read source code files !

- kamailio :
	http://tlsf.baisoku.org , 
	TLSF - Two Level Segregated Fit memory allocator implementation

- freeswitch :
	apr : http://apr.apache.org/ , Apache Portable Runtime

- redis :
	jemalloc / http://www.canonware.com/jemalloc/index.html /

- other 'malloc' variants: ptmalloc(),tcmalloc(),jemalloc()

**********************************

pmap -x PID -> view process memory map 

* Valgrind / Insure++
 
valgrind --tool=memcheck ls -l

perf ????

**********************************
???
mtrace(),muntrace()
mcheck(),mprobe()
mallopt(),

- mallinfo() , http://man7.org/linux/man-pages/man3/mallinfo.3.html

- getrusage() , http://man7.org/linux/man-pages/man2/getrusage.2.html

**********************************

https://www.win.tue.nl/~aeb/linux/lk/lk-9.html
http://www.roman10.net/2011/07/29/linux-kernel-programmingmemory-allocation/
http://stackoverflow.com/questions/4863707/how-to-see-linux-view-of-the-ram-in-order-to-determinate-the-fragmentation
http://serverfault.com/questions/133305/linux-memory-fragmentation
https://lwn.net/Articles/211505/
http://learnlinuxconcepts.blogspot.bg/2014/02/linux-memory-management.html

http://www.intel.com/content/www/us/en/processors/architectures-software-developer-manuals.html

https://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html

*** !!!! Добра статия !!!!
https://akkadia.org/drepper/cpumemory.pdf
https://techtalk.intersec.com/2013/07/memory-part-1-memory-types/   
***

https://sploitfun.wordpress.com/2015/02/10/understanding-glibc-malloc/

https://gitlab.com/procps-ng/procps

**********************************
test 1: 

[root@localhost tests]# pmap -x 25653
25653:   ./mem_test
Address           Kbytes     RSS   Dirty Mode  Mapping
0000000000400000       4       4       0 r---- mem_test
0000000000401000       4       4       0 r-x-- mem_test
0000000000402000       4       4       0 r---- mem_test
0000000000403000       4       4       4 r---- mem_test
0000000000404000       4       4       4 rw--- mem_test

0000000000df0000       4       4       4 rw---   [ anon ] !!! тук,на този адрес беше heap-a,след освобождане на паметта остават '4k dirty' !!!???

test 2:
[root@localhost tests]# pmap -x 25741
25741:   ./mem_test
Address           Kbytes     RSS   Dirty Mode  Mapping
0000000000400000       4       4       4 r---- mem_test
0000000000401000       4       4       4 r-x-- mem_test
0000000000402000       4       4       4 r---- mem_test
0000000000403000       4       4       4 r---- mem_test
0000000000404000       4       4       4 rw--- mem_test

0000000000f4f000     132      12      12 rw---   [ anon ] !!! не е освободена паметта , 8320 bytes са 'заявени',което са 3*4k pages /4156bytes = 1 page/ ?!


! тестовете направени с тази програма винаги заемат само едно адресно пространство,
но при някои приложения се наблюдават по няколко heap-a ... защо ? няколко нишки ???


****
test 3 /отчитане на време за заемане и време за освобождаване/:

[root@localhost tests]# ./mem_test 1000

Proccess PID: 10590
Max: 4156

mem: pages: 251,u:1043156 bytes,m:1040000 bytes

mem_test_alloc(),r_timer: 0.001283

mem_test_free(),r_timer: 0.002626

[root@localhost tests]# ./mem_test 10000

Proccess PID: 10599
Max: 12564

mem: pages: 2503,u:10402468 bytes,m:10400000 bytes

mem_test_alloc(),r_timer: 0.006924

mem_test_free(),r_timer: 0.025406

[root@localhost tests]# 
[root@localhost tests]# ./mem_test 100000

Proccess PID: 10634
Max: 105588

mem: pages: 25025,u:104003900 bytes,m:104000000 bytes

mem_test_alloc(),r_timer: 0.060725

mem_test_free(),r_timer: 1.276450

[root@localhost tests]# ./mem_test 1000000

Proccess PID: 10644
Max: 1033484

mem: pages: 250241,u:1040001596 bytes,m:1040000000 bytes

mem_test_alloc(),r_timer: 0.550875

mem_test_free(),r_timer: 118.067533

[root@localhost tests]# 
[root@localhost tests]# ./mem_test 10000000

Proccess PID: 10782
 
Max: 6925628

mem: pages: 2502407,u:10400003492 bytes,m:10400000000 bytes

mem_test_alloc(),r_timer: 253.464095

mem_test_free(),r_timer: 14796.629380

****
test 4 /отчитане на време за заемане и време за освобождаване/:

[root@localhost tests]# ./mem_test_arr 1000

Proccess PID: 16805
Max: 3316

mem: pages: 248,u:1030688 bytes,m:1 029 028 bytes

mem_test_alloc(),r_timer: 0.001333

mem_test_free(),r_timer: 0.000125

[root@localhost tests]# 
[root@localhost tests]# ./mem_test_arr 10000

Proccess PID: 16834
Max: 12292

mem: pages: 2474,u:10281944 bytes,m:10 281 028 bytes

mem_test_alloc(),r_timer: 0.007027

mem_test_free(),r_timer: 0.001055

[root@localhost tests]# 
[root@localhost tests]# ./mem_test_arr 100000

Proccess PID: 16864
Max: 102536

mem: pages: 24736,u:102802816 bytes,m:102 801 028 bytes

mem_test_alloc(),r_timer: 0.062675

mem_test_free(),r_timer: 0.007477

[root@localhost tests]# 
[root@localhost tests]# ./mem_test_arr 1000000

Proccess PID: 16918
Max: 1006164

mem: pages: 247354,u:1028003224 bytes,m:1 028 001 028 bytes

mem_test_alloc(),r_timer: 0.561959

mem_test_free(),r_timer: 0.061360

[root@localhost tests]# ./mem_test_arr 10000000

Proccess PID: 17074
Max: 7442540

mem: pages: 2473533,u:10280003148 bytes,m:10 280 001 028 bytes

mem_test_alloc(),r_timer: 372.078745

mem_test_free(),r_timer: 0.530337
