#include <unistd.h>

#include "../misc/globals.h"
#include "../misc/mem/mem.h"
#include "../misc/exten/time_funcs.h"
#include "../misc/mem/shm_mem.h"
#include "../misc/re5_manager.h"

#include "cc_cfg.h"
#include "cc_stat.h"

#include "my_cc/my_cc.h"
#include "cc_server.h"

cc_server_tbl_t *cc_server_tbl_init(int sim)
{
	cc_server_tbl_t *tbl;
	
	if(sim == 0) return NULL;
	
	tbl = mem_alloc_arr(sim,sizeof(cc_server_tbl_t));
	
	return tbl;
}

int cc_server_call_init(cc_t *cc_ptr,rating *pre)
{
	int i,ret;
	
	ret = RE_ERROR;
	
	pthread_mutex_lock(&cc_tbl_lock);

	for(i=0;i<sim_calls;i++) {
		if((cc_tbl[i].cc_ptr == NULL)&&(cc_tbl[i].pre == NULL)) {
			cc_tbl[i].cc_ptr = cc_ptr;
			cc_tbl[i].pre = pre;
			
			ret = RE_SUCCESS;
			
			//LOG("cc_server_call_init()","new,clg: %s,call_uid: %s,i: %d",
			//	 cc_tbl[i].cc_ptr->max->clg,cc_tbl[i].cc_ptr->max->call_uid,i);
			
			break;
		} //else LOG("cc_server_call_init()","old,clg: %s,call_uid: %s,i: %d",
			//		cc_tbl[i].cc_ptr->max->clg,cc_tbl[i].cc_ptr->max->call_uid,i);
	}
	
	pthread_mutex_unlock(&cc_tbl_lock);
	
	return ret;
}

int cc_server_call_search_call_uid(cc_t *cc_ptr)
{
	int i,ret;
	
	ret = RE_ERROR_N;
	
	pthread_mutex_lock(&cc_tbl_lock);

	for(i=0;i<sim_calls;i++) {
		if(cc_tbl[i].cc_ptr != NULL) {
			if(cc_tbl[i].cc_ptr->max != NULL) {	
				if(strcmp(cc_tbl[i].cc_ptr->max->call_uid,cc_ptr->term->call_uid) == 0) {
					LOG("cc_server_call_search_call_uid()","tbl ind: %d",i);

					cc_tbl[i].cc_ptr->term = cc_ptr->term;

					ret = i;
					break;
				}
			}
		}
	}
	
	pthread_mutex_unlock(&cc_tbl_lock);

	LOG("cc_server_call_search_call_uid()","ret: %d",ret);
	return ret;
}

int cc_server_call_search_racc(char *racc)
{
	int i,ret;
	
	ret = 0;
	
	pthread_mutex_lock(&cc_tbl_lock);
	
	for(i=0;i<sim_calls;i++) {
		if(cc_tbl[i].cc_ptr != NULL) {
			if(cc_tbl[i].cc_ptr->max != NULL) {
				//if(strcmp(cc_tbl[i].pre->clg,racc) == 0) { BUG#174
				if(strcmp(cc_tbl[i].cc_ptr->max->clg,racc) == 0) {
					ret++;
					
					LOG("cc_server_call_search_racc()","racc: %s,sim: %d",racc,ret);
				}
			}
		}
	}
	
	pthread_mutex_unlock(&cc_tbl_lock);
	
	return ret;
}

void cc_server_add_cdr(cc_server_tbl_t *tbl)
{	
	cdr_t cdr_pt;
	
	if((tbl != NULL)&&(tbl->cc_ptr != NULL)&&(tbl->cc_ptr->term != NULL)) {
		memset(&cdr_pt,0,sizeof(cdr_t));

		/* copy 'cdr_server_id' from 'struct cc' to 'struct cdr' 
		 * - from 'struct cc' is null ????
		 * - from 'pre' is not null ! temp to use this !!!
		 * */
		cdr_pt.cdr_server_id = tbl->pre->cdr_server_id;
		
		/* set 'cdr_rec_type_id' to be 'voip_a' ??? */
		cdr_pt.cdr_rec_type_id = voip_a;
		
		/* copy 'call_uid' from 'struct term' to 'struct cdr' */
		strcpy(cdr_pt.call_uid,tbl->cc_ptr->term->call_uid);
		
		/* copy 'pre->timestamp' to 'cdr.start_ts' */
		strcpy(cdr_pt.start_ts,tbl->pre->timestamp);
		
		/* copy 'pre->ts' to 'cdr->start_epoch' */
		cdr_pt.start_epoch = tbl->pre->ts;
		
		/* copy 'clg' from 'struct max' to 'struct cdr' as 'calling_number' */
		strcpy(cdr_pt.calling_number,tbl->cc_ptr->max->clg);
		
		/* copy 'cld' from 'struct max' to 'struct cdr' as 'called_number' */
		strcpy(cdr_pt.called_number,tbl->cc_ptr->max->cld);
		
		/* copy 'billsec' from 'struct term' to 'struct cdr' */
		cdr_pt.billsec = tbl->cc_ptr->term->billsec;
		
		/* copy 'duration' from 'struct term' to 'struct cdr' */
		cdr_pt.duration = tbl->cc_ptr->term->duration;
				
		/* insert 'no complete' cdr in DB */
		if(cdr_add_in_db(ccserver.conn2,&cdr_pt,NULL)) 
			tbl->pre->cdr_id = cdr_get_cdr_id(ccserver.conn2,&cdr_pt);	
		else tbl->pre->cdr_id = 0;
	}	
}

void cc_server_call_term(cc_server_tbl_t *tbl,cc_t *cc_ptr)
{
	pthread_mutex_lock(&cc_tbl_lock); // bug#173

	// bug#173
	//if((tbl != NULL)&&(cc_ptr->term != NULL)) {	
	if((tbl->cc_ptr != NULL)&&(cc_ptr->term != NULL)) {
		tbl->cc_ptr->term = cc_ptr->term;
	}
	
	pthread_mutex_unlock(&cc_tbl_lock); // bug#173
}

void cc_server_call_clear(cc_server_tbl_t *tbl)
{
	pthread_mutex_lock(&cc_tbl_lock);
	
	if(tbl->pre != NULL) {
		LOG("cc_server_call_clear()","mem_free(tbl->pre),call_uid: %s",tbl->pre->call_uid);
		mem_free(tbl->pre);
	}

	tbl->pre = NULL;

	if(tbl->cc_ptr != NULL) {
		tbl->cc_ptr->cc_status = CC_STATUS_DEACTIVE;
		LOG("cc_server_call_clear()","cc_free(tbl->cc_ptr)");
		cc_free(tbl->cc_ptr);
	} else {
		LOG("cc_server_call_clear()","'cc_ptr' pointer is NULL");
	}
	
	tbl->cc_ptr = NULL;
	
	pthread_mutex_unlock(&cc_tbl_lock);
}

void *cc_server_thread(void *dt)
{
	int i;
	
	proc_thread_t proc;
	shm_mem_t cc_st_shm;
	
	cc_status_t *status = NULL;
	re5_mgr_mem_t *mgr = NULL;
	
	unsigned int requests = 0;
	unsigned int responses = 0;
	unsigned int return_errors = 0;
	unsigned int rating_minutes = 0;
	
	mgr = re5_mgr_mem_init(&re5_mgr_shm,2);
	
	if(demonize) {
		cc_status_init(&cc_st_shm,1);
		status = (cc_status_t *)cc_st_shm.ptr;
	}
	
	if(status == NULL) LOG("cc_server_thread()","The CC_SERVER_THREAD is not attached successfull into STATUS shmem!");
	
	while(loop_flag == 't') {
		cc_server_sim = 0;
		
		for(i=0;i<sim_calls;i++) {
			if((cc_tbl[i].cc_ptr != NULL)) {
				requests ++;
				
				if(cc_tbl[i].cc_ptr->term != NULL) {
					LOG("cc_server_thread","term is NOT NULL(%d)",i);
					
					if(cc_tbl[i].cc_ptr->term->status == normal_clear) {
						convert_epoch_to_ts(cc_tbl[i].pre->ts,cc_tbl[i].pre->timestamp);
							
						if(cc_tbl[i].cc_ptr->term->billsec > 0) cc_server_add_cdr(&cc_tbl[i]);
							
						if(cc_tbl[i].pre->cdr_id > 0) { 
							cc_call_rating(cc_tbl[i].cc_ptr,cc_tbl[i].pre);
							
							rating_minutes += (cc_tbl[i].pre->billsec)/60;
						} else {
							LOG("cc_server_thread()","call_uid: %s,no rating",cc_tbl[i].cc_ptr->term->call_uid);
						
							if(cc_tbl[i].pre) {
								if(cc_tbl[i].pre->card) mem_free(cc_tbl[i].pre->card);
								if(cc_tbl[i].pre->card) mem_free(cc_tbl[i].pre->tc);
								
								mem_free(cc_tbl[i].pre);
								cc_tbl[i].pre = NULL; 
							}
						}
					}
					
					LOG("cc_server_thread()","call_uid: %s,clear from the cc_server table (receive 'term')",cc_tbl[i].cc_ptr->term->call_uid);
					
					cc_server_call_clear(&cc_tbl[i]);
					cc_server_sim--;

#if CC_NOLOOP
					loop_flag = 'f';
					sleep(2);
					break;
#endif
				} else if(cc_tbl[i].cc_ptr->max != NULL) {
					if((time(NULL) - cc_tbl[i].cc_ptr->max->ts) > (call_maxsec_limit + CALL_MAXSEC_LIMIT_WAIT)) {
						LOG("cc_server_thread()","call_uid: %s,clear from the cc_server table (ts > call_maxsec_limit)",cc_tbl[i].cc_ptr->max->call_uid);
						
						cc_server_call_clear(&cc_tbl[i]);
						cc_server_sim--;
					}
				}
				
				cc_server_sim++;
			}
		}
		
		if(status) {
			status->cc_status_sim = cc_server_sim;
			status->cc_status_ts = time(NULL);
		}
		
		if(mgr) {
			mgr->cc_ts = time(NULL);
			loop_flag = mgr->cc_flag;
		}
		
		usleep(ccserver.ccserver_usleep);
	}
	
	PQfinish(ccserver.conn);
	PQfinish(ccserver.conn2);
	
	if(status) cc_status_free(&cc_st_shm,1);
	
	pthread_mutex_destroy(&cc_tbl_lock);
	
	mem_free(cc_tbl);
	
	LOG("cc_server_thread()","A CallControl server thread is stoped");
	
	if(mgr) {
		mgr->cc_pid = 0;
		re5_mgr_mem_free(&re5_mgr_shm,0);
	}
	
	pthread_exit(NULL);
}

void cc_server_thread_run(proc_thread_t *proc)
{
	if(proc != NULL) {
		proc->args = NULL;
		proc->thread_func = cc_server_thread;
		proc->mode = PROC_THREAD_DETACHED;
		
		proc_thread_run(proc);
	}
}

void cc_server_main(void)
{
	int i,ret;
	proc_thread_t proc;
	cc_cfg_t *cfg;
	re5_mgr_mem_t *mgr;
	
	cc_tbl = NULL;
	
	mgr = re5_mgr_mem_init(&re5_mgr_shm,0);
	
	cfg = cc_cfg_main(mcfg->cfg_filename);
	if(cfg != NULL) {
		if(mgr) {
			mgr->cc_interval = cfg->cc_interval;
			re5_mgr_mem_free(&re5_mgr_shm,0);
		}
		
		ccserver.conn = db_pgsql_conn(mcfg->db);
		ccserver.conn2 = db_pgsql_conn(mcfg->db);
		
		if(cfg->cc_server_usleep > 0) ccserver.ccserver_usleep = cfg->cc_server_usleep;
		else ccserver.ccserver_usleep = CC_SERVER_USLEEP;
		
		LOG("cc_server_main()","int_number: %d",cfg->int_number);
		
		if(cfg->call_maxsec_limit > 0) call_maxsec_limit = cfg->call_maxsec_limit;
		else call_maxsec_limit = CALL_MAXSEC_LIMIT;
		
		if(cfg->sim_calls > 0) sim_calls = cfg->sim_calls;
		else sim_calls = SIM_CALLS;
		
		cc_tbl = cc_server_tbl_init(sim_calls);
		if(cc_tbl != NULL) {
			pthread_mutex_init(&cc_tbl_lock,NULL);
			
			cc_server_thread_run(&proc);
			
			if(proc.error == PROC_THREAD_ERROR) LOG("cc_server_main()","A 'cc_server_thread' is not started!");
			else {
				LOG("cc_server_main()","A 'cc_server_thread' is started successful(usleep: %d)!",ccserver.ccserver_usleep);
			}
			
			for(i=0;i<cfg->int_number;i++) {
				switch(cfg->interfaces[i].proto) {
					case my_cc:
						ret = my_cc_main(&cfg->interfaces[i]);
						LOG("cc_server_main()","my_cc_main() is return %d",ret);
						break;
					case diam:
						break;
					case my_cc_v2:
						//ret = my_cc_v2_main(&cfg->interfaces[i]);
						LOG("cc_server_main()","my_cc_v2_main() is return %d",ret);
						break;
					case unkn_proto:
						break;
				}
			}
		} else LOG("cc_server_main()","A 'cc_tbl' pointer is null!"); 
	} else LOG("cc_server_main()","A 'cfg' pointer is null!");
	
	mem_free(cfg);
} 
