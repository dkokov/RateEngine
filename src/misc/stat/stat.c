#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "stat.h"

stat_data_t *stat_init(void)
{	
	char *shm;
	int shmid;
	stat_data_t *ptr;
	
	shmid = shmget(STAT_SHMEM_KEY, sizeof(stat_data_t), IPC_CREAT | 0666);
	if(shmid < 0) return 0;
	
	shm = shmat(shmid, NULL, 0);
    if((shm = shmat(shmid, NULL, 0)) == (char *) -1) return 0;
    	
	ptr = (stat_data_t *)shm;
	memset(ptr,0,sizeof(stat_data_t));

	return ptr;
}

void stat_remove(void)
{	
	int shmid;
	
	shmid = shmget(STAT_SHMEM_KEY, sizeof(stat_data_t), 0666);
	
	if(shmid > 0) shmctl(shmid, IPC_RMID, NULL);
}

void stat_write(stat_data_t *ptr,stat_data_t *tmp)
{
	if((ptr)&&(tmp)) {
		memcpy(ptr,tmp,sizeof(stat_data_t));
	}
}

stat_data_t *stat_read(void)
{
	char *shm;
	int shmid;
	stat_data_t *ptr;
	
	shmid = shmget(STAT_SHMEM_KEY, sizeof(stat_data_t), 0666);
	if(shmid < 0) return 0; 
	
    if((shm = shmat(shmid, NULL, 0)) == (char *) -1) return 0;
    
    ptr = (stat_data_t *)shm;
    
    return ptr;
}

