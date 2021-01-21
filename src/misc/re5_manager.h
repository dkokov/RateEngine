#ifndef RE5_MANAGER_H
#define RE5_MANAGER_H

#define RE5_MGR_RET_NUM 5
#define RE5_MGR_SHMEM_NAME "/RateEngine.re5_mgr"

shm_mem_t re5_mgr_shm;

typedef struct re5_mgr_mem {
	time_t re5_mgr_ts;
	int re5_mgr_pid;
	char re5_mgr_flag;
	
	time_t cc_ts;
	int cc_pid;
	int cc_sv[2];
	unsigned int cc_interval;
	char cc_flag;
	
	time_t rt_ts;
	int rt_pid;
	int rt_sv[2];
	unsigned int rt_interval;
	char rt_flag;
	
	time_t cdrm_ts;
	int cdrm_pid;
	int cdrm_sv[2];
	unsigned int cdrm_interval;
	char cdrm_flag;
} re5_mgr_mem_t;

int re5_starter(void);
void re5_manager(void);

re5_mgr_mem_t *re5_mgr_mem_init(shm_mem_t *shm,int mode);
void re5_mgr_mem_free(shm_mem_t *shm,int mode);

#endif
