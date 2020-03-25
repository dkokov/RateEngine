#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void *shm_mem_init(key_t key,size_t shm_size)
{	
	char *shm;
	int shmid;
	void *ptr;
	
	shmid = shmget(key, shm_size, IPC_CREAT | 0666);
	if(shmid < 0) return NULL;
	
	shm = shmat(shmid, NULL, 0);
    if((shm = shmat(shmid, NULL, 0)) == (char *) -1) return NULL;
    	
	ptr = (void *)shm;
	memset(ptr,0,shm_size);

	return ptr;
}

void shm_mem_remove(key_t key,size_t shm_size)
{	
	int shmid;
	
	shmid = shmget(key, shm_size, 0666);
	
	if(shmid > 0) shmctl(shmid, IPC_RMID, NULL);
}

void *shm_mem_read(key_t key,size_t shm_size)
{
	char *shm;
	int shmid;
	void *ptr;
	
	shmid = shmget(key, shm_size, 0666);
	if(shmid < 0) return NULL; 
	
    if((shm = shmat(shmid, NULL, 0)) == (char *) -1) return NULL;
    
    ptr = (void *)shm;
    
    return ptr;
}

int shm_mem_free(char *addr)
{
	return shmdt(addr);
}
