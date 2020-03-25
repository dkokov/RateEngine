/*
 * http://man7.org/linux/man-pages/man3/pthread_create.3.html
 * 
 * */
 
#ifndef PROC_THREAD_H
#define PROC_THREAD_H

#include <pthread.h>

/* value x 1000 = Mbytes */
#define PROC_THREAD_STACKSIZE_1M 1024
#define PROC_THREAD_STACKSIZE_2M 2048
#define PROC_THREAD_STACKSIZE_4M 4096
#define PROC_THREAD_STACKSIZE_8M 8192

#define PROC_THREAD_STATUS_OK 0
#define PROC_THREAD_ERROR    -1

#define PROC_THREAD_JOINABLE 1
#define PROC_THREAD_DETACHED 2 

typedef struct proc_thread {
	
	int rc;
	
	unsigned short mode;
	
	void *(*thread_func)(void*);
	
	pthread_t proc;
	
	pthread_attr_t attr;
	
	int stack_size;
	
	void *status;
	
	void *args;
	
	int error;
		
}proc_thread_t;

proc_thread_t *proc_thread_tbl(int n);
proc_thread_t *proc_thread_func(void *(*thread_func)(void*),void *args);
void proc_thread_run(proc_thread_t *dt);

#endif 
