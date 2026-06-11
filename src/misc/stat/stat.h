#ifndef STAT_H
#define STAT_H

#define STAT_SHMEM_KEY 5767

typedef struct stat_data
{
	/* Full using size of the memory */
	unsigned long use_mem;

	/* Current sim calls in the 'cc_server_thread()' */
	unsigned short sim;

	/* Current timestamp */
	time_t ts;

	/* Rating statistics */
	unsigned int rt_total_calls;
	unsigned int rt_success_calls;
	unsigned int rt_error_calls;
	unsigned int rt_rating_minutes;
	time_t rt_ts;

	/* CDRMediator statistics */
	unsigned int cdrm_total_cdrs;
	time_t cdrm_ts;

	/* Process flags */
	char rt_flag;
	char cdrm_flag;
	char cc_flag;
	char mgr_flag;

	pid_t mgr_pid;

}stat_data_t;

stat_data_t *stat_init(void);
void stat_remove(void);
void stat_write(stat_data_t *ptr,stat_data_t *tmp);
stat_data_t *stat_read(void);

#endif
