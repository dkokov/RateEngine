#include <time.h>
#include <string.h>
#include <pthread.h>

#include "../misc/mem/shm_mem.h"

#include "cc_stat.h"

cc_stat_t *cc_stat_init(shm_mem_t *shm,int mode)
{
	cc_stat_t *stat;
	
	strcpy(shm->shm_name,CC_STAT_SHM_NAME);
	shm->shm_fd = -1;
	shm->ptr = NULL;
	shm->shm_size = sizeof(cc_stat_t);
	
	if(mode == 1) shm->shm_mode = SHMEM_MODE_CREATE;
	else shm->shm_mode = SHMEM_MODE_RW;
	
	shmem_open(shm);
	stat = (cc_stat_t *)shm->ptr;
	
	return stat;
}

void cc_stat_free(shm_mem_t *shm,int mode)
{
	if(mode == 1) shm->shm_mode = SHMEM_MODE_REMOVE;
	
	shmem_close(shm);
}

void cc_stat_put(cc_stat_t *stat,unsigned int requests,unsigned int responses,unsigned int error,double minutes)
{
	if(stat != NULL) {
		stat->cc_ts = time(NULL);
		
		stat->cc_requests   += requests;
		stat->cc_responses  += responses;
		stat->cc_return_errors  += error;
		stat->cc_rating_minutes += minutes;
	}
}

cc_status_t *cc_status_init(shm_mem_t *shm,int mode)
{
	cc_status_t *stat;
	
	strcpy(shm->shm_name,CC_STATUS_SHM_NAME);
	shm->shm_fd = -1;
	shm->ptr = NULL;
	shm->shm_size = sizeof(cc_status_t);
	
	if(mode == 1) shm->shm_mode = SHMEM_MODE_CREATE;
	else shm->shm_mode = SHMEM_MODE_RW;
	
	shmem_open(shm);
	stat = (cc_status_t *)shm->ptr;
	
	return stat;
}

void cc_status_free(shm_mem_t *shm,int mode)
{
	if(mode == 1) shm->shm_mode = SHMEM_MODE_REMOVE;
	
	shmem_close(shm);
}
