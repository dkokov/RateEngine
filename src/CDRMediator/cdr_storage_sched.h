#ifndef _CDR_STORAGE_SCHED_H_
#define _CDR_STORAGE_SCHED_H_

#define CDR_STORAGE_SCHED_M 60
#define CDR_STORAGE_SCHED_H 3600
#define CDR_STORAGE_SCHED_D 86400

typedef struct cdr_storage_sched
{
    time_t ts;
    time_t last;
    time_t start;
    int replies;
    time_t curr_ts;
    time_t last_ts;
    
}cdr_storage_sched_t;

int cdr_storage_sched_set_ts(cdr_storage_profile_t *cfg);
void cdr_storage_sched_insert(cdr_storage_profile_t *cfg);
void cdr_storage_sched_update(cdr_storage_profile_t *cfg);

#endif
