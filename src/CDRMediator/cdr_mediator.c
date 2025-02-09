#include "../misc/globals.h"
#include "../misc/mem/mem.h"

#include "cdr_file.h"
#include "cdr_storage.h"
#include "cdr_cfg.h"

#include <unistd.h>
#include <pthread.h>

void mcdr_filters_init(cdr_profile_cfg_t *profile)
{	
	if((profile->called_number_filtering == 't')) {
		profile->filters = prefix_filter_get(profile->conn,profile->cdr_profile_id);
	}	
}

void mcdr_file_reader(cdr_profile_cfg_t *profile)
{
	//profile->profile_file_type->filters = filters;
	profile->profile_file_type->filters = profile->filters;
	
	profile->profile_file_type->conn = profile->conn;
	
	profile->profile_file_type->cdr_server_id = profile->cdr_server_id;
	profile->profile_file_type->cdr_rec_type_id = profile->t2;
	
	cdr_file_reader(profile->profile_file_type);
}

void mcdr_storage_reader(cdr_profile_cfg_t *profile)
{
	profile->profile_db_type->filters = profile->filters;
	profile->profile_db_type->conn = profile->conn;
	
	profile->profile_db_type->cdr_server_id = profile->cdr_server_id;
	profile->profile_db_type->cdr_rec_type_id = profile->t2;
	
	cdr_storage_reader(profile->profile_db_type);
}

void mcdr_cfg_free(cdr_cfg_t *cfg)
{
	int i;
	
	for(i=0;i<cfg->profiles_number;i++) {
		if(cfg->profiles[i].profile_db_type != NULL) {
			//mem_free(cfg->profiles[i].profile_db_type->cols); 
			//mem_free(cfg->profiles[i].profile_db_type); 
		}

		if(cfg->profiles[i].profile_file_type != NULL) {
			mem_free(cfg->profiles[i].profile_file_type->hdr);
			mem_free(cfg->profiles[i].profile_file_type);
		}
	}
	
	mem_free(cfg->list);
	mem_free(cfg->profiles);
	mem_free(cfg);
}

void mcdr_cfg_profile_free(cdr_profile_cfg_t *profile)
{	
	switch(profile->t) {
		case file:
				mem_free(profile->profile_file_type->hdr);
				mem_free(profile->profile_file_type);
				profile = NULL;
				break;
		case db:
				mem_free(profile->profile_db_type->cols);
				mem_free(profile->profile_db_type);
				profile = NULL;		
				break;
	}
}

/* Thread per profile */
void *CDRMediatorThread(void *dt)
{
	int i;
	
	i = 0;
	
	cdr_profile_cfg_t *profile = (cdr_profile_cfg_t *)dt;
	
	if(profile != NULL) {		
		mcdr_filters_init(profile);

		loop:
		i++;

		LOG("CDRMediatorThread()","start[%d] profile '%s',cdr_server_id: %d (%d) ",
			i,profile->profile_name,profile->cdr_server_id,profile->cdr_profile_id);	

		if(profile->called_number_filtering == 't') {
			LOG("CDRMediatorThread()","profile: %s,cdr_server_id: %d",
				profile->profile_name,profile->cdr_server_id);
							  
			if(profile->filters == NULL) {
				LOG("CDRMediatorThread()",
					"profile: %s,cdr_server_id: %d,'filters' pointer is null",
					profile->profile_name,profile->cdr_server_id);
			}
		}

		switch(profile->t) {
			case file:
				mcdr_file_reader(profile);
				break;
			case db:
				mcdr_storage_reader(profile);
				break;
		}
	
		if(profile->cdr_active_flag == 't') {
			LOG("CDRMediatorThread()","sleep[%d] profile '%s',cdr_server_id: %d",
				profile->cdr_interval,profile->profile_name,profile->cdr_server_id);	
			sleep(profile->cdr_interval);
			goto loop;
		}
	
		if(profile->filters != NULL) mem_free(profile->filters);
		
		mcdr_cfg_profile_free(profile);
	} else {
		LOG("CDRMediatorThread()","A 'profile' pointer is null!");
	}

	pthread_exit(NULL);
}

/* 
 * CDR Mediator Engine is main CDR thread.
 * Read and parse CDR config and start CDR profiles as threads.
 * 
 * */
void *CDRMediatorEngine(void *dt)
{
	int i;
	cdr_cfg_t *cfg;
    cdr_profile_cfg_t *profiles;
  
	if(cdr_tbl_cpy_ptr == NULL) cdr_tbl_cpy_ptr = cdr_tbl_cpy();
    
    cfg = cdr_cfg_main(mcfg->cfg_filename);

	if(cfg != NULL) {
		if(cfg->cdr_tcp_conn > 0) cdr_tcp_conn = cfg->cdr_tcp_conn;
		else cdr_tcp_conn = CDR_TCP_CONN;

		profiles = cfg->profiles;

		for(i=0;i<cfg->profiles_number;i++) {
			int rc[cfg->profiles_number];
			pthread_t th[cfg->profiles_number];
			
			/* License restriction  */
			if(i < cdr_tcp_conn) {
				/* Starting profile ... */
				profiles[i].conn = db_pgsql_conn(mcfg->db);
			
				rc[i] = pthread_create(&th[i],NULL,CDRMediatorThread,(void *)&profiles[i]);
			
				if(rc[i]) {
					LOG("CDRMediatorEngine",
						"pthread_create(cdr_mediator) error!A profile '%s' is not starting!",
						profiles[i].profile_name);
					continue;
				} else {
					LOG("CDRMediatorEngine","Starting profile[%s (%d),v.%d]",
						profiles[i].profile_name,profiles[i].cdr_server_id,
						profiles[i].profile_version);
				}
				
				if(profiles[i].cdr_active_flag == 'f') {
					pthread_join(th[i],NULL);
				
					PQfinish(profiles[i].conn);
					
					LOG("CDRMediatorEngine","pthread_join() for profile : %s",profiles[i].profile_name);
				}
			} else {
				LOG("CDRMediatorEngine",
					"'CDRTCPConn' restriction!Profile '%s' is not started!",
					profiles[i].profile_name);
				break;
			}
		}		
	} else {
		LOG("CDRMediatorEngine","A 'cfg' pointer is null!");
	}	
	
	if(opt_cli_mem.daemon_flag == 0) {
		loop_flag = 'f';
//		mcdr_cfg_free(cfg);
	}
	
    pthread_exit(NULL);
}
