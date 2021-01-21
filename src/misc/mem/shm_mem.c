#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#include "shm_mem.h"

int shmem_open(shm_mem_t *shm)
{
	if(shm == NULL) return 1;
	
	if(strlen(shm->shm_name) > 0) {
		int flags;
		if(shm->shm_mode == SHMEM_MODE_CREATE) flags = O_CREAT | O_RDWR;
		else flags = O_RDWR;
		
		shm->shm_fd = shm_open(shm->shm_name, flags, 0600);
		if(shm->shm_fd < 0) return 1;
		
		if(shm->shm_mode == SHMEM_MODE_CREATE) { 
			if(ftruncate(shm->shm_fd, shm->shm_size) < 0)  return 1;
		}
		
		shm->ptr = mmap(0, shm->shm_size, PROT_WRITE, MAP_SHARED, shm->shm_fd, 0);
		if (shm->ptr == MAP_FAILED) return 1;
	}
	
	return 0;
}

int shmem_close(shm_mem_t *shm)
{
	if(shm == NULL) return 1;
	if(shm->ptr == NULL) return 1;
	if(shm->shm_fd < 0) return 1;
	
	if(munmap(shm->ptr, shm->shm_size) < 0) return 1;
	if(close(shm->shm_fd) < 0) return 1;
	
	if(shm->shm_mode == SHMEM_MODE_REMOVE) { 
		if(shm_unlink(shm->shm_name) < 0) return 1; 
	}
	
	return 0;
}
