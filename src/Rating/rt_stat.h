#ifndef RT_STAT_H
#define RT_STAT_H

#define RT_STAT_SHM_NAME "/RateEngine.rt_stat"

typedef struct rt_stat {
	unsigned int rt_stat_total_calls;
	unsigned int rt_stat_success_calls;
	unsigned int rt_stat_error_calls;
	unsigned int rt_stat_rating_minutes;
	
	time_t rt_stat_ts;
} rt_stat_t;

rt_stat_t *rt_stat_init(shm_mem_t *shm,int mode);
void rt_stat_free(shm_mem_t *shm,int mode);
void rt_stat_put(rt_stat_t *stat,unsigned int total,unsigned int success,unsigned int error,unsigned int minutes);

#endif 
