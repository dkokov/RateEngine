#include "proc_thread.h"

#include "../mem/mem.h"

proc_thread_t *proc_thread_tbl(int n)
{
	proc_thread_t *tbl;
	
	if(n <= 0) return NULL;
	
	tbl = (proc_thread_t *)mem_alloc_arr(n,sizeof(proc_thread_t));
	
	return tbl;
}

proc_thread_t *proc_thread_func(void *(*thread_func)(void*),void *args)
{
	proc_thread_t *proc = NULL;
	
	proc = proc_thread_tbl(1);
	
	if(proc != NULL) {
		proc->thread_func = thread_func;
		proc->args = args;
		
		proc_thread_run(proc);
	}
	
	return proc;
}

void proc_thread_run(proc_thread_t *dt)
{	
	pthread_attr_init(&dt->attr);
	
	if(dt->stack_size > 0) {
		pthread_attr_setstacksize(&dt->attr,dt->stack_size);
	}
	
	if(dt->mode == PROC_THREAD_JOINABLE) pthread_attr_setdetachstate(&dt->attr,PTHREAD_CREATE_JOINABLE);
	if(dt->mode == PROC_THREAD_DETACHED) pthread_attr_setdetachstate(&dt->attr,PTHREAD_CREATE_DETACHED);
				
	dt->rc = pthread_create(&dt->proc,&dt->attr,dt->thread_func,dt->args);

	if(dt->rc == 0) {
		dt->error = PROC_THREAD_STATUS_OK;
		
		pthread_attr_destroy(&dt->attr);
		if(dt->mode == PROC_THREAD_JOINABLE) pthread_join(dt->proc,dt->status);
	} else {
		dt->error = PROC_THREAD_ERROR;
	}
} 
