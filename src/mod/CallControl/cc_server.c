#include <unistd.h>
#include <pthread.h>

#include "../../misc/globals.h"
#include "../../db/db.h"
#include "../../net/net.h"
#include "../../mem/mem.h"
#include "../../proc/proc_thread.h"
#include "../../misc/exten/time_funcs.h"
#include "../../misc/stat/stat.h"

#include "../Rating/rating.h"
#include "../Rating/rt_bind_api.h"

#include "cc.h"
#include "cc_cfg.h"
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
				if(strcmp(cc_tbl[i].pre->clg,racc) == 0) {
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
		if(cdrm_api.add_cdr(ccserver.dbp,&cdr_pt,NULL)) 
			tbl->pre->cdr_id = cdrm_api.get_cdr_id(ccserver.dbp,&cdr_pt);	
		else tbl->pre->cdr_id = 0;
	}	
}

void cc_server_call_term(cc_server_tbl_t *tbl,cc_t *cc_ptr)
{
	if((tbl != NULL)&&(cc_ptr->term != NULL)) {
		tbl->cc_ptr->term = cc_ptr->term;
	}
}

void cc_server_call_clear(cc_server_tbl_t *tbl)
{
	pthread_mutex_lock(&cc_tbl_lock);
	
	if(tbl->pre != NULL) {
		if(tbl->pre->tr != NULL) {
			mem_free(tbl->pre->tr);
			tbl->pre->tr = NULL;
		}
		
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

void *cc_server_proto_bind(char *proto)
{
	void *func;
	mod_t *mod_ptr;
	
	char libname[255];
	char funcname[255];

	if(mod_lst == NULL) return NULL;
		
	if(strlen(proto) == 0) return NULL;
	
	memset(libname,0,sizeof(libname));
	sprintf(libname,"%s.so",proto);
	
	memset(funcname,0,sizeof(funcname));
	sprintf(funcname,"%s_proto_bind",proto);
	
	mod_ptr = mod_find_module(libname);
	if(mod_ptr == NULL) return NULL;
	
	func = NULL;
	
	if(mod_ptr->handle != NULL) {
		func = mod_find_func(mod_ptr->handle,funcname);
	}
	
	return func;	
}

void *cc_server_thread_int(void *dt)
{
	int ret;
	void *func;
	
	net_t *np;
	int (*external_func)(char *);
	cc_cfg_int_t *cc_int = (cc_cfg_int_t *)dt;
	
	/* CallControl protocol init */
	func = cc_server_proto_bind(cc_int->cc_proto);

	if(func != NULL) {
		external_func = func;
		
		LOG("cc_server_thread_int()","cc_server_proto_bind() for '%s'",cc_int->cc_proto);
	} else {
		LOG("cc_server_thread_int()","cc_server_proto_bind() ERROR for '%s'",cc_int->cc_proto);

		goto _end;
	}
	
	/* Network interface init */
	np = net_init();
	
	if(np == NULL) {
		LOG("cc_server_thread_int()","net_init() ERROR");
		goto _end;
	}
	
	np->conn->buf_size = 2048;
	np->conn->ipv = cc_int->ipv;
	strcpy(np->conn->ip_str,cc_int->ip);
	np->conn->port = cc_int->port;
	strcpy(np->conn->proto,cc_int->proto);
	
	/* Network Protocol Bind */
	if(net_proto_bind(np) < 0) {
		LOG("cc_server_thread_int()","net_proto_bind() ERROR for '%s'",cc_int->proto);
		goto _end;
	}
	
	LOG("cc_server_thread_int()","net_proto_bind() for '%s'",cc_int->proto);
	
	if(net_open(np) < 0) goto _end;
	
	LOG("cc_server_thread_int()","net_open() success");	
	
	if(net_listen(np) < 0) goto _end;

	LOG("cc_server_thread_int()","net_listen() success");
	
	ret = net_serial_server(np,external_func);
	
_end:
	LOG("cc_server_thread_int()","_end , ret: %d",ret);
	
	net_close(np);

	net_free(np);
	pthread_exit(NULL);	
}

void cc_server_thread_int_run(cc_cfg_int_t *cc_int)
{
	proc_thread_t proc;
	
	memset(&proc,0,sizeof(proc_thread_t));
	
	proc.args = (void *)cc_int;
	proc.thread_func = cc_server_thread_int;
	proc.mode = PROC_THREAD_DETACHED;
	proc_thread_run(&proc);
}

void *cc_server_thread(void *dt)
{
	int i;
	stat_data_t *ptr;
	
	ptr = stat_read();
	
	if(ptr == 0) LOG("cc_server_thread()","The CC_SERVER_THREAD is not attached successfull into STAT shmem!");
	
	while(loop_flag == 't') {
		cc_server_sim = 0;

		for(i=0;i<sim_calls;i++) {	
			if((cc_tbl[i].cc_ptr != NULL)) {				
				if(cc_tbl[i].cc_ptr->term != NULL) {
					LOG("cc_server_thread","term is NOT NULL(%d)",i);
										
					if(cc_tbl[i].cc_ptr->term->status == normal_clear) {	
						convert_epoch_to_ts(cc_tbl[i].pre->ts,cc_tbl[i].pre->timestamp);
							
						if(cc_tbl[i].cc_ptr->term->billsec > 0) cc_server_add_cdr(&cc_tbl[i]);
							
						if(cc_tbl[i].pre->cdr_id > 0) cc_call_rating(cc_tbl[i].cc_ptr,cc_tbl[i].pre);
						else {
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

		if(ptr) {
			ptr->sim = cc_server_sim;
			ptr->ts = time(NULL);
		}
		
		usleep(ccserver.ccserver_usleep);
	}
	
//	PQfinish(ccserver.conn);
//	PQfinish(ccserver.conn2);	
	
	pthread_mutex_destroy(&cc_tbl_lock);
	
	mem_free(cc_tbl);
	
	LOG("cc_server_thread()","A CallControl server thread is stoped");
	
//	pthread_join(ccserver_proc,NULL);
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

int cc_server_cdrm_init(void)
{
	int ret;
	void *func;
	mod_t *mod_ptr;
	int (*fptr)(cdr_funcs_t *);

	memset(&cdrm_api,0,sizeof(cdr_funcs_t));

	mod_ptr = mod_find_module("CDRMediator.so");

	if(mod_ptr == NULL) return RE_ERROR_N;
	if(mod_ptr->handle == NULL) return RE_ERROR_N;
	
	func = mod_find_func(mod_ptr->handle,"cdr_bind_api");
	if(func != NULL) {
		fptr = func;
			
		ret = fptr(&cdrm_api);
		if(ret < 0) {
			LOG("cc_server_cdr_init()","cdr_bind_api(),ret: %d",ret);
			return RE_ERROR_N;
		}
	}

	return RE_SUCCESS;	
}

int cc_server_rt_init(void)
{
	int ret;
	void *func;
	mod_t *mod_ptr;
	int (*fptr)(rt_funcs_t *);

	memset(&rt_api,0,sizeof(rt_funcs_t));

	mod_ptr = mod_find_module("Rating.so");

	if(mod_ptr == NULL) return RE_ERROR_N;
	if(mod_ptr->handle == NULL) return RE_ERROR_N;
	
	func = mod_find_func(mod_ptr->handle,"rt_bind_api");
	if(func != NULL) {
		fptr = func;
			
		ret = fptr(&rt_api);
		if(ret < 0) {
			LOG("cc_server_rt_init()","rt_bind_api(),ret: %d",ret);
			return RE_ERROR_N;
		}
	}

	return RE_SUCCESS;	
}

int CallControl_mod_init(void)
{
	cc_server_cdrm_init();
	cc_server_rt_init();
	
	return RE_SUCCESS;
}

void *cc_server_main(void *dt)
{
	int i;
	proc_thread_t proc;
	cc_cfg_t *cfg;
	
	cc_tbl = NULL;
	
	cfg = cc_cfg_main(mcfg->cfg_filename);
	
	if(cfg != NULL) {		
		if(cfg->cc_server_usleep > 0) ccserver.ccserver_usleep = cfg->cc_server_usleep;
		else ccserver.ccserver_usleep = CC_SERVER_USLEEP;
		
		LOG("cc_server_main()","int_number: %d",cfg->int_number);
			
		if(cfg->call_maxsec_limit > 0) call_maxsec_limit = cfg->call_maxsec_limit;
		else call_maxsec_limit = CALL_MAXSEC_LIMIT;

		if(cfg->sim_calls > 0) sim_calls = cfg->sim_calls;
		else sim_calls = SIM_CALLS;

		ccserver.dbp = db_init();
		ccserver.dbp->conn = db_conn_init(mcfg->dbtype,mcfg->dbhost,mcfg->dbname,mcfg->dbport,mcfg->dbuser,mcfg->dbpass,0);
		
		if(db_engine_bind(ccserver.dbp) < 0) {
			LOG("cc_server_main()","db_engine_bind() ERROR - local '%s' db server",ccserver.dbp->conn->enginename);
			goto _end_func;
		}

		if(db_connect(ccserver.dbp) < 0) {
			LOG("cc_server_main()","db_connect() ERROR - local '%s' db server",ccserver.dbp->conn->enginename);
			goto _end_func;
		}
		
		cc_tbl = cc_server_tbl_init(sim_calls);
				
		if(cc_tbl != NULL) {
			pthread_mutex_init(&cc_tbl_lock,NULL);
									
			cc_server_thread_run(&proc);
			
			if(proc.error == PROC_THREAD_ERROR) { 
				LOG("cc_server_main()","A 'cc_server_thread' is not started!");
			} else {
				LOG("cc_server_main()","A 'cc_server_thread' is started successful(usleep: %d)!",
					ccserver.ccserver_usleep);					
			}
			
			for(i=0;i<cfg->int_number;i++) {
				if(strlen(cfg->interfaces[i].cc_proto) > 0) {
					cc_server_thread_int_run(&cfg->interfaces[i]);
				}
			}
		} else LOG("cc_server_main()","A 'cc_tbl' pointer is null!");
	} else LOG("cc_server_main()","A 'cfg' pointer is null!");

_end_func:
	mem_free(cfg);
	db_free(ccserver.dbp);
	
	pthread_exit(NULL);
} 
