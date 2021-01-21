#ifndef CC_STAT_H
#define CC_STAT_H

#define CC_STAT_SHM_NAME   "/RateEngine.cc_stat"
#define CC_STATUS_SHM_NAME "/RateEngine.cc_status"

typedef struct cc_stat {
	unsigned int cc_requests;
	unsigned int cc_responses;
	unsigned int cc_return_errors;
	unsigned int cc_rating_minutes;
	
	time_t cc_ts;
} cc_stat_t;

typedef struct cc_status {
	unsigned short cc_status_sim;
	
	time_t cc_status_ts;
} cc_status_t;

cc_stat_t *cc_stat_init(shm_mem_t *shm,int mode);
void cc_stat_free(shm_mem_t *shm,int mode);
void cc_stat_put(cc_stat_t *stat,unsigned int requests,unsigned int responses,unsigned int error,double minutes);
cc_status_t *cc_status_init(shm_mem_t *shm,int mode);
void cc_status_free(shm_mem_t *shm,int mode);

#endif
