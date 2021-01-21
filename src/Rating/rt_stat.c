#include <time.h>
#include <string.h>
#include <pthread.h>

#include "../misc/mem/shm_mem.h"

#include "rt_stat.h"

rt_stat_t *rt_stat_init(shm_mem_t *shm,int mode)
{
	rt_stat_t *stat;
	
	strcpy(shm->shm_name,RT_STAT_SHM_NAME);
	shm->shm_fd = -1;
	shm->ptr = NULL;
	shm->shm_size = sizeof(rt_stat_t);
	
	if(mode == 1) shm->shm_mode = SHMEM_MODE_CREATE;
	else shm->shm_mode = SHMEM_MODE_RW;
	
	shmem_open(shm);
	stat = (rt_stat_t *)shm->ptr;
	
	return stat;
}

void rt_stat_free(shm_mem_t *shm,int mode)
{
	if(mode == 1) shm->shm_mode = SHMEM_MODE_REMOVE;
	
	shmem_close(shm);
}

void rt_stat_put(rt_stat_t *stat,unsigned int total,unsigned int success,unsigned int error,unsigned int minutes)
{
	if(stat != NULL) {
		stat->rt_stat_ts = time(NULL);
		
		stat->rt_stat_total_calls    += total;
		stat->rt_stat_success_calls  += success;
		stat->rt_stat_error_calls    += error;
		stat->rt_stat_rating_minutes += minutes;
	}
}
