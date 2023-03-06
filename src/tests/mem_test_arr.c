/*
 * gcc -Wall -g -o mem_test_arr mem_test_arr.c -L../ -lre7core
 * 
 * valgrind ./mem_test_arr
 * pmap -x PID
 * cat /proc/PID/maps
 * 
 * */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef DEBUG_MEM
	#include <sys/resource.h>
#endif

#include "../mem/mem.h"
#include "../misc/exten/time_funcs.h"

typedef struct mem_test {
	unsigned int i;
	char str[1024];
} mem_test_t;

void mem_test_wait(void)
{
	while(1) { 
		getchar();
		break;
	}	
}

mem_test_t *mem_test_alloc(int n)
{
	int i;
	mem_test_t *ptr;
	
	ptr = (mem_test_t *)mem_alloc_arr((n+1),sizeof(mem_test_t));
	
	for(i=0;i<n;i++) {
		ptr[i].i = i;
		sprintf(ptr[i].str,"test alloc %d",i);	
	}
	
    return ptr;
}

void mem_test_view(mem_test_t *ptr)
{	
	int i;
	
	i = 0;
    while(strlen(ptr[i].str)) {
		fprintf(stderr,"[%d],%s\n",ptr[i].i,ptr[i].str);
		
		i++;
    }
}

void mem_test_free(mem_test_t *ptr)
{
	mem_free(ptr);
}

int main(int argc, char *argv[])
{
	int num,num2,show,loop;
	pid_t pid;
	
	rtimer_t rt;
	mem_test_t *ptr;

#ifdef DEBUG_MEM
	struct rusage stat;
#endif
	
	if(argc == 1) num = 8;
	else num = atoi(argv[1]);
	
	show = 0;loop = 0;
	if(argc == 2) {
		show = 0; 
	} else {
		if(strcmp(argv[2],"-v") == 0) show = 1;
		else if(strcmp(argv[2],"-l") == 0) loop = 1;
	}
	
	memset(&rt,0,sizeof(rtimer_t));
	
	pid = getpid();
	
	fprintf(stderr,"\nProccess PID: %d\n",pid);
	
	if(num <= 0) return -1;

again:
	mem_info_clear();
	
	if(loop) num2 = (rand() % num);
	else num2 = num;

#ifdef DEBUG_MEM	
	rt.flag = 0;
	r_timer(&rt);
#endif	

	ptr = mem_test_alloc(num2);
	
#ifdef DEBUG_MEM
	getrusage(RUSAGE_SELF,&stat);

	fprintf(stderr,"Max: %ld\n", stat.ru_maxrss);

	fprintf(stderr,"\nmem: pages: %ld,u:%ld bytes,m:%ld bytes\n",memstat.u_pages,memstat.u_size,memstat.m_size);
#endif

	if(show) mem_test_view(ptr);

#ifdef DEBUG_MEM
	rt.flag = 1;
	r_timer(&rt);

	fprintf(stderr,"\nmem_test_alloc(),r_timer: %f\n",RTIMER_SEC(rt.r_timer));
#endif

	mem_test_wait();

#ifdef DEBUG_MEM
	rt.flag = 0;
	r_timer(&rt);
#endif

	mem_test_free(ptr);

#ifdef DEBUG_MEM	
	rt.flag = 1;
	r_timer(&rt);

	fprintf(stderr,"\nmem_test_free(),r_timer: %f\n",RTIMER_SEC(rt.r_timer));
#endif
	
	mem_test_wait();
	
	if(loop) goto again;
	
	return 0;
}
