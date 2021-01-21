#include <unistd.h>
#include <pthread.h>

//#include "../mod.h"
#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "prefix_filter.h"
#include "cdr.h"
#include "cdr_file.h"
#include "cdr_storage.h"
#include "cdr_cfg.h"

/*
mod_t cdrm_mod_t = {
	.mod_name = "",
	.ver      = 1,
	.init     = NULL,
	.destroy  = NULL,
	.depends  = NULL,
	.handle   = NULL,
	.next     = NULL
};
*/
// shte se izpolzali ... sega se 4ete ot profil faila !!!!
void mcdr_filters_init(cdr_profile_cfg_t *profile)
{	
	if((profile->called_number_filtering == 't')) {
		if(profile->dbp->t == sql) profile->filters = prefix_filter_get(profile->dbp,profile->cdr_profile_id);
	}
}

void mcdr_file_reader(cdr_profile_cfg_t *profile)
{
	profile->profile_file_type->filters = profile->filters;
	profile->profile_file_type->dbp = profile->dbp;
	strcpy(profile->profile_file_type->profile_name,profile->profile_name);
	
	profile->profile_file_type->cdr_server_id = profile->cdr_server_id;
	profile->profile_file_type->cdr_rec_type_id = profile->t2;
	
	cdr_file_reader(profile->profile_file_type);
}

void mcdr_storage_reader(cdr_profile_cfg_t *profile)
{
	profile->profile_db_type->filters = profile->filters;
	profile->profile_db_type->dbp = profile->dbp;
	strcpy(profile->profile_db_type->profile_name,profile->profile_name);
	
	profile->profile_db_type->cdr_server_id = profile->cdr_server_id;
	profile->profile_db_type->cdr_rec_type_id = profile->t2;
	
	cdr_storage_reader(profile->profile_db_type);
}

void mcdr_cfg_free(cdr_cfg_t *cfg)
{
	int i;
	
	if(cfg != NULL) {
		for(i=0;i<cfg->profiles_number;i++) {
			if(cfg->profiles[i].profile_db_type != NULL) {
				mem_free(cfg->profiles[i].profile_db_type->cols); 
				mem_free(cfg->profiles[i].profile_db_type); 
			}

			if(cfg->profiles[i].profile_file_type != NULL) {
				mem_free(cfg->profiles[i].profile_file_type->hdr);
				mem_free(cfg->profiles[i].profile_file_type);
			}
			
			mem_free(cfg->profiles[i].filters);
		} 
	
		mem_free(cfg->list);
		mem_free(cfg->profiles);
		mem_free(cfg);
	}
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
	int i = 0;
	cdr_profile_cfg_t *profile = (cdr_profile_cfg_t *)dt;
	
	if(profile != NULL) {
		mcdr_filters_init(profile);

		loop:
		i++;

		LOG("CDRMediatorThread()","start[%d] profile '%s',cdr_server_id: %d (%d) ",i,profile->profile_name,profile->cdr_server_id,profile->cdr_profile_id);

		if(profile->called_number_filtering == 't') {
			LOG("CDRMediatorThread()","profile: %s,cdr_server_id: %d",profile->profile_name,profile->cdr_server_id);
							  
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
	
//		if(profile->filters != NULL) mem_free(profile->filters);
		
//		mcdr_cfg_profile_free(profile);
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
	int i,ret;
	cdr_cfg_t *cfg;
    cdr_profile_cfg_t *profiles;

	if(cdr_tbl_cpy_ptr == NULL) cdr_tbl_cpy_ptr = cdr_tbl_cpy();
        
    cfg = cdr_cfg_main(mcfg->cfg_filename);

	if(cfg != NULL) {
		profiles = cfg->profiles;

		for(i=0;i<cfg->profiles_number;i++) {
			/* Threads init */
			int rc[cfg->profiles_number];
			pthread_t th[cfg->profiles_number];
			
			/* Starting profile ... */
			profiles[i].dbp = db_init();
				
			profiles[i].dbp->conn = db_conn_init(mcfg->dbtype,mcfg->dbhost,mcfg->dbname,mcfg->dbport,mcfg->dbuser,mcfg->dbpass,0);

			ret = db_engine_bind(profiles[i].dbp);
			if(ret < 0) {
				db_error(ret);	
				LOG("CDRMediatorEngine","db_engine_bind() ERROR - local '%s' db server",profiles[i].dbp->conn->enginename);

				break;
			}
				
			ret = db_connect(profiles[i].dbp);
			if(ret < 0) {
				db_error(ret);
				LOG("CDRMediatorEngine","db_connect() ERROR - local '%s' db server",profiles[i].dbp->conn->enginename);
				
				break;
			}			

			cdr_cfg_profile_insert(&profiles[i]);

			if(profiles[i].t == db) {
				/* Remote DB server */
				profiles[i].profile_db_type->rem_dbp = db_init();
				
				profiles[i].profile_db_type->rem_dbp->conn = db_conn_init(profiles[i].profile_db_type->dbtype,
																		profiles[i].profile_db_type->dbhost,
																		profiles[i].profile_db_type->dbname,
																		profiles[i].profile_db_type->dbport,
																		profiles[i].profile_db_type->dbuser,
																		profiles[i].profile_db_type->dbpass,0);
			
				ret = db_engine_bind(profiles[i].profile_db_type->rem_dbp);
				if(ret < 0) {
					db_error(ret);
					LOG("CDRMediatorEngine","db_engine_bind() ERROR - remote db server ");
					
					break;
				}
			}
				
			rc[i] = pthread_create(&th[i],NULL,CDRMediatorThread,(void *)&profiles[i]);			
			if(rc[i]) {
				LOG("CDRMediatorEngine","pthread_create(cdr_mediator) error!A profile '%s' is not starting!",profiles[i].profile_name);
				continue;
			} else {
				LOG("CDRMediatorEngine","Starting profile[%s (%d),v.%d]",
					 profiles[i].profile_name,profiles[i].cdr_server_id,profiles[i].profile_version);
			}
				
			if(profiles[i].cdr_active_flag == 'f') {
				pthread_join(th[i],NULL);
				
				db_close(profiles[i].dbp);
				db_free(profiles[i].dbp);
				
				if(profiles[i].t == db) db_free(profiles[i].profile_db_type->rem_dbp);
				
				LOG("CDRMediatorEngine","pthread_join() for profile : %s",profiles[i].profile_name);
			} else {
				loop_flag = 't';
				opt_cli_mem.daemon_flag = 1;
			}
		}		
	} else {
		LOG("CDRMediatorEngine","A 'cfg' pointer is null!");
	}
	
	if(opt_cli_mem.daemon_flag == 0) {
		loop_flag = 'f';

		mcdr_cfg_free(cfg);		
		mem_free(cdr_tbl_cpy_ptr);
	}

    pthread_exit(NULL);
}
