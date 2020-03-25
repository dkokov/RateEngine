#ifndef SHM_MEM_H
#define SHM_MEM_H

void *shm_mem_init(key_t key,size_t shm_size);
void shm_mem_remove(key_t key,size_t shm_size);
void *shm_mem_read(key_t key,size_t shm_size);
int shm_mem_free(char *addr);

#endif
