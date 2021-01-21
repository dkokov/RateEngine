#ifndef SHM_MEM_H
#define SHM_MEM_H

#define SHMEM_MODE_RW     0
#define SHMEM_MODE_CREATE 1
#define SHMEM_MODE_REMOVE 2

typedef struct shm_mem {
	int shm_fd;
	int shm_mode;
	size_t shm_size;
	char shm_name[64];
	
	void *ptr;
} shm_mem_t;

int shmem_open(shm_mem_t *shm);
int shmem_close(shm_mem_t *shm);

#endif
