#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "../mod.h"
#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"
#include "../../misc/exten/time_funcs.h"

#include "rating.h"
#include "rt_cfg.h"
#include "time_cond.h"
#include "free_billsec.h"
#include "pcard.h"
#include "calc_functions.h"
#include "round_billsec.h"
#include "rt_data_q.h"

rate_engine_t rt_eng;
cdr_funcs_t *cdrm_api;
 
int rt_init(void);
int rt_free(void);

mod_dep_t rt_mod_dep[] = {
	{"cdrm.so",0,1},
	{"",0,0}
};

mod_t rt_mod_t = {
	.mod_name = "Rating",
	.ver      = 1,
	.init     = NULL,
	.destroy  = NULL,
	.depends  = rt_mod_dep,
	.handle   = NULL,
	.next     = NULL
};

void rt_rating_init(rating_t *pre)
{
	memset(pre,0,sizeof(rating_t));
}

void rt_chk_bplan_periods(bplan_t *bpt,int ts)
{
    if(bpt->bplan_start_period) {
		if(bpt->bplan_start_period <= ts) {
            if(bpt->bplan_end_period) {
                if(bpt->bplan_end_period >= ts) return;
                else bpt->id = 0;
            }
        } else bpt->id = 0;
    }
}

void rt_chk_tr_opt(db_t *dbp,rating_t *pre)
{
/*	if(f_time_cond_query_id(dbp,pre)) {
		f_time_cond_query_v2(dbp,pre);
		if(!tc_ts_cmp(pre)) pre->tariff = 0;
		free(pre->tc);
	}
	
    f_free_billsec_query(dbp,pre); */
}

void rt_balance_exec(db_t *dbp,racc_t *rtp,char *start,char *end)
{
	int ret;

	if(dbp == NULL) return;
	if(rtp == NULL) return;
	if(rtp->pre == NULL) return;

	ret = rt_data_q_bal(dbp,rtp,start,end);
	if(ret == 0) {
		rtp->bal_ptr->amount = rtp->bal_ptr->amount + rtp->pre->cprice;
		rt_data_q_bal_add(dbp,rtp,start,end);
	}
}

int rt_prerating_process(db_t *dbp,racc_t *rtp)
{
	time_t tt;
	int bal_num;
	int online_rating;
	int day,bday,pp;
	struct tm  *tm;
	
	bacc_t *btp;
    pcard_t *card;	
	rating_t *pre;

	if(dbp == NULL) return DB_ERR_DBP_NUL;
	if(rtp == NULL) return -999;
	if(rtp->pre == NULL) return -998;
	if(rtp->bacc_ptr == NULL) return -997;

	pre = rtp->pre;
	btp = rtp->bacc_ptr;
    card = btp->pcard_ptr;
    
    if(pre->epoch) {
		/* offline rating process */
		tt = pre->epoch;
		online_rating = 0;
	} else {
		/* online rating process */
		tt = pre->ts;
		online_rating = 1;
	}
			    
	bal_num = 0;
			
	/* blocking card,when have more balance with flag 't' than 2 */
    if((card->id)&&(online_rating)) {	
		/* day of payment */
		bal_num = rt_data_q_bal_count(dbp,rtp,'t');
		
		if(bal_num > 1) {
			tm   = localtime(&tt);
			day  = tm->tm_mday;
					
			bday = btp->billing_day;
			
			if(bday == 0) bday = atoi(billing_day);

			if(bal_num > rt_eng.bal_num) {
				rt_data_q_pcard_set(dbp,rtp->bacc_ptr,pcard_block);
				LOG("rt_prerating_process()","Blocking PCard,card_id %d",card->id);
				return RE_ERROR;				
			}
			
			if(btp->day_of_payment == 0) pp = 0;
			else pp = btp->day_of_payment;
								
			/* DayOfPayment checking,if 'dday > 0' */
			if(pp > 0) {
				if(bday > pp) {
					if(day < bday) {
						if(day >= pp) {
							LOG("rt_prerating_process()","Blocking PCard,card_id %d",card->id);
							rt_data_q_pcard_set(dbp,rtp->bacc_ptr,pcard_block);

							return RE_ERROR;
						}					
					}
				} else {
					if(day >= pp) {
						rt_data_q_pcard_set(dbp,rtp->bacc_ptr,pcard_block);

						LOG("rt_prerating_process()","Blocking PCard,card_id %d",card->id);
						return RE_ERROR;
					}
				}
			}
		}
	}

	if((strlen(card->start))&&(strlen(card->end))) {
		/* Check date validation */
		if((check_date_valid(card->start) == 0)||(check_date_valid(card->end) == 0)) {
			rt_data_q_pcard_set(dbp,rtp->bacc_ptr,pcard_block);
			
			LOG("rt_prerating_process()","start_date(%s) or end date(%s) is not valid!The pcard(%d) is blocked!",
				card->start,card->end,card->id);

			return RE_ERROR;
		}
		
		/* generate new balance in the same time period 
		if((f_bal_query_2(dbp,pre,card->start,card->end,'f'))&&(card->type == credit_card)) {	
			if((strcmp(pre->bal_lupdate,card->end) < 0)) {
				pre->bal_id = 0;
				pre->balanse = 0;
				strcpy(card->start,pre->bal_lupdate);

				DBG("prerating_process()","changing of the start_date - start: %s,last_update: %s,end: %s",card->start,pre->bal_lupdate,card->end);
			}
		}*/
/*		
		balance_exec(conn,pre,card->start,card->end);
		if(pre->bal_id = 0) {
			LOG("prerating_process()","create new balance ERROR(balance_id is NULL),bacc %d,start %s,end %s",pre->bacc,card->start,card->end);
			return 1;
		}
	
		if((pre->free_billsec_limit)) {
			free_billsec_exec(conn,pre,card->start,card->end);
			if(pre->free_id == 0) {
				LOG("prerating process()","ERROR,no free_billsec balance,bacc: %d,start: %s,end: %s",pre->bacc,card->start,card->end);
				return 1;
			}
		} */
		
		rt_data_q_bal(dbp,rtp,card->start,card->end);
		
		//if(pre->free_billsec_limit) {
		//	free_billsec_balance_v2(dbp,rtp,card->start,card->end);
		//	f_free_billsec_bal_query_2(dbp,pre);
		//}
    } else {
		LOG("rt_prerating process()","ERROR,no start or end date,bacc: %d,start: %s,end: %s",btp->id,card->start,card->end);
		
		return RE_ERROR;
    }
    
    return RE_SUCCESS;
}

void rt_rate_searching(db_t *dbp,racc_t *rtp)
{
	if(dbp == NULL) return;
	if(rtp == NULL) return;
	if(rtp->pre == NULL) return;
	
	rt_data_q_rate(dbp,rtp);

    if(rtp->bplan_ptr->rates_ptr == NULL) { 
		LOG("rt_rate_searching()","rates_ptr is null,call_uid %s",rtp->pre->call_uid);
		return;
    }
    
    if(rtp->bacc_ptr->pcard_ptr != NULL) { 
		if(rt_prerating_process(dbp,rtp)) {
			LOG("rt_rate_searching()","A error from prerating_process,call_uid %s",rtp->pre->call_uid);
			return;
		}
    }
}

void rt_cdr_to_rating(cdr_t *the_cdr,rating_t *pre)
{
	int tmp_ts = convert_ts_to_epoch(the_cdr->start_ts);
	
    pre->cdr_server_id = the_cdr->cdr_server_id;
    //pre->cdr_rec_type_id = the_cdr->cdr_rec_type_id;
    strcpy(pre->call_uid,the_cdr->call_uid);
    strcpy(pre->clg,the_cdr->calling_number);
    strcpy(pre->cld,the_cdr->called_number);
    pre->billsec = the_cdr->billsec;
    pre->cdr_id = the_cdr->id;
    strcpy(pre->timestamp,the_cdr->start_ts);
    pre->dow = get_weekday_from_epoch(tmp_ts);
    pre->ts = the_cdr->start_epoch;
    strcpy(pre->account_code,the_cdr->account_code);
    strcpy(pre->src_context,the_cdr->src_context);
    strcpy(pre->dst_context,the_cdr->dst_context);
    strcpy(pre->src_tgroup,the_cdr->src_tgroup);
    strcpy(pre->dst_tgroup,the_cdr->dst_tgroup);
    pre->epoch = tmp_ts;
    pre->clg_nadi = the_cdr->clg_nadi;
    pre->cld_nadi = the_cdr->cld_nadi;
    pre->billusec = the_cdr->billusec;
}

void rt_rating_save(db_t *dbp,racc_t *rtp)
{   
	int ret;
	
	if(rtp->pre->cdr_id > 0) {
		pthread_mutex_lock(&config.sync_bt_thread);
		
		ret = rt_data_q_rating_add(dbp,rtp);
		
		if(ret < 0) {
			
		}
		
		pthread_mutex_unlock(&config.sync_bt_thread);
	}	
}

void rt_exec(db_t *dbp,racc_t *rtp,char leg)
{
	int ret;
	int billsec_temp = 0;

	pcard_t *card;	
	rating_t *pre;
	bacc_t *bacc_ptr;
	bplan_t *bplan_ptr;
	
	if(rtp->pre == NULL) return;
	pre = rtp->pre;
	
	if(rtp->bacc_ptr == NULL) return;
	bacc_ptr = rtp->bacc_ptr;
	
	if(rtp->bplan_ptr == NULL) return;
	bplan_ptr = rtp->bplan_ptr;
	
    rt_chk_bplan_periods(bplan_ptr,pre->ts);

    if(rtp->rtm) {
		if(bacc_ptr->pcard_ptr != NULL) card = bacc_ptr->pcard_ptr;
		else card = NULL;
		
		rt_rate_searching(dbp,rtp);

//		if((bplan_ptr->rates_ptr != NULL)&&(bplan_ptr->rates_i > 0)) {
		if((bplan_ptr->rates_ptr != NULL)) {
			if(bacc_ptr->round_mode_id) 
				billsec_temp = round_billsec(bacc_ptr->round_mode_id,pre->billusec);
			
			if(billsec_temp) pre->billsec = billsec_temp;
			
			if(rtp->rtm == rt_mode_sms) {
				//if((card > 0)&&(pre->free_billsec_id > 0)) free_billsec_sms_balance(dbp,rtp,card->start,card->end);
				//ret = calc_cprice_sms(rtp);
			} else ret = calc_cprice_group(rtp);
			
			switch(ret) {
				case 0:
						rt_rating_save(dbp,rtp);
						break;
				case (-1):
						pre->rating_id = -1;
						break;
				case 1:
						if((rt_double_rating(dbp,rtp,leg)) == -1)
							pre->rating_id = -1;
			}
			
			//tr_free(tr);
		} else {
			pre->rating_id = -1;
			LOG("DEBUG","don't have any rates");
		}
    } else pre->rating_id = -1;

	if(pre->rating_id == 0) {
		LOG("rating_exec()","rating_id is '0' after 'rating_save()' / this call_uid %s,call_id %d is already rated",pre->call_uid,pre->cdr_id);
	} else {
		cdrm_api->update_cdr(dbp,pre->rating_id,pre->cdr_id,leg,pre->call_uid);

		if((card)&&((pre->rating_id) > 0)) {			
			rt_balance_exec(dbp,rtp,card->start,card->end);
		
			//if((pre->free_billsec_limit)&&(pre->rating_mode_id < 7)) free_billsec_exec(dbp,pre,card->start,card->end);	
		}
	}
}

int rt_double_rating(db_t *dbp,racc_t *rtp,char leg)
{
	int ret_1,ret_2;
	int no_free_sec;
	int free_sec;
	int old_billsec;
	
	pcard_t *card;
	rating_t *pre;
	
	pre = rtp->pre;
	card = rtp->bacc_ptr->pcard_ptr;
	
	/* save current billsec value */
	old_billsec = pre->billsec;
	
	free_sec = ((pre->free_billsec_limit) - (pre->free_billsec_sum));
	
	if(free_sec > 0) {
		no_free_sec = ((pre->billsec) - (free_sec));
	} else goto no_free_billsec;
	
	if(log_debug_level > LOG_LEVEL_INFO) {
		char msg[512];
		
		sprintf(msg,"CallingNumber(%s),Call-UID(%s),free_sec(%d),no_free_sec(%d)",
				pre->clg,pre->call_uid,free_sec,no_free_sec);
		
		LOG("double_rating()",msg);
	}
	
	/* Phase 1 - free billsec rating */
	pre->billsec = free_sec;
	
	ret_1 = calc_cprice_group(rtp);

	if(ret_1 == 0) rt_rating_save(dbp,rtp);
	else if(ret_1 == 1) {
		LOG("double_rating()","1/ret_2 = 1");
		
		pre->billsec = old_billsec;
		
		goto no_free_billsec;
	} else return -1;

	/* Phase 2 - no free billsec rating */
	pre->billsec = no_free_sec;
	
	if(card) {
		if((strlen(card->start))&&(strlen(card->end))) {
			rt_balance_exec(dbp,rtp,card->start,card->end);
			
			if((pre->free_billsec_limit)) free_billsec_exec(dbp,rtp,card->start,card->end);
		}
	}	
		
	no_free_billsec:
	ret_2 = calc_cprice_group(rtp);

	if(ret_2 == 0) {
		rt_rating_save(dbp,rtp);
		
		LOG("double_rating()","2/rating save");
		
		return 0;
	} else if(ret_2 == 1) {
		LOG("double_rating()","2/ret_2 = 1");
		
		rt_rating_save(dbp,rtp);
		
		return 0;
	} else {
		LOG("double_rating()","2/ret_2 = -1");
		
		return -1;
	}
}

racc_t *rt_racc_voip_av_a(db_t *dbp,rating_t *pre)
{
	racc_t *racc_pt;
	
	pre->rating_mode_id = rt_mode_clg;
	racc_pt = rt_data_q_racc(dbp,pre);
	
	if(racc_pt == NULL) {
		pre->rating_mode_id = rt_mode_acode;
		racc_pt = rt_data_q_racc(dbp,pre);
	}

	return racc_pt;
}

racc_t *rt_racc_voip_t_a(db_t *dbp,rating_t *pre)
{
	racc_t *racc_pt;
	
	pre->rating_mode_id = rt_mode_acode;
	racc_pt = rt_data_q_racc(dbp,pre);
	
	if(racc_pt == NULL) {
		pre->rating_mode_id = rt_mode_srcc;
		racc_pt = rt_data_q_racc(dbp,pre);
	}

	return racc_pt;
}

void rt_racc_voip_t_b(db_t *dbp,rating_t *pre)
{
/*	f_dst_context_query(dbp,pre,pre->dst_context);	
	
	if((pre->bacc) > 0) pre->rating_mode_id = RT_MODE_DST_CONTEXT; */
}

racc_t *rt_racc_isup_a(db_t *dbp,rating_t *pre)
{
	racc_t *racc_pt;
	
	pre->rating_mode_id = rt_mode_srctg;
	racc_pt = rt_data_q_racc(dbp,pre);

	return racc_pt;
}

void rt_racc_isup_b(db_t *dbp,rating_t *pre)
{
/*	f_dst_tgroup_query(dbp,pre,pre->dst_tgroup,pre->clg_nadi,pre->cld_nadi);
	
	if((pre->bacc) > 0) pre->rating_mode_id = RT_MODE_DST_TGROUP; */
}

racc_t *rt_racc_sms_a(db_t *dbp,rating_t *pre)
{
	racc_t *racc_pt;

	pre->rating_mode_id = rt_mode_sms;
	racc_pt = rt_data_q_racc(dbp,pre);	
	
	return racc_pt;
}

void rt_racc_sms_b(db_t *dbp,rating_t *pre)
{
	
}

racc_t *rt_racc_unkn_a(db_t *dbp,rating_t *pre)
{
	racc_t *racc_pt;
	
	pre->rating_mode_id = rt_mode_clg;
	racc_pt = rt_data_q_racc(dbp,pre);

	if(racc_pt == NULL) {
		pre->rating_mode_id = rt_mode_acode;
		racc_pt = rt_data_q_racc(dbp,pre);

		if(racc_pt == NULL) {
			pre->rating_mode_id = rt_mode_srcc;
			racc_pt = rt_data_q_racc(dbp,pre);
				
			if(racc_pt == NULL) {
				pre->rating_mode_id = rt_mode_srctg;
				racc_pt = rt_data_q_racc(dbp,pre);
				
				if(racc_pt == NULL) {
					if(log_debug_level > LOG_LEVEL_INFO) {
						LOG("rating_unkn_a()","Rating5-LegA error,call_uid %s",pre->call_uid);
						return NULL;
					}
				}
			} 
		}
	}
	
	return racc_pt;
}

void rt_racc_unkn_b(db_t *dbp,rating_t *pre)
{
/*	if(strcmp(pre->dst_context,"")) f_dst_context_query(dbp,pre,pre->dst_context);
	
	if((pre->bacc == 0)) {
	    if(strcmp(pre->dst_tgroup,"")) f_dst_tgroup_query(dbp,pre,pre->dst_tgroup,pre->clg_nadi,pre->cld_nadi);
	
	    if((pre->bacc == 0)) {
			if(log_debug_level > LOG_LEVEL_INFO) {
				LOG("rating_unkn_b()","Rating5-LegB error,call_uid %s",pre->call_uid);
			}
	    } else pre->rating_mode_id = RT_MODE_DST_TGROUP;
	} else pre->rating_mode_id = RT_MODE_DST_CONTEXT; */
}

/* Main rating function */
void rt_main(db_t *dbp,rating_t *pre,char leg,int t)
{
	racc_t *rtp;
	
    pre->rating_mode_id = 0;

    if(leg == 'a') {
		switch(t) {
			case unkn:
					rtp = rt_racc_unkn_a(dbp,pre);
					break;
			case isup:
					rtp = rt_racc_isup_a(dbp,pre);
					break;
			case voip_a:
					rtp = rt_racc_voip_av_a(dbp,pre);
					break;
			case voip_v:
					break;
			case voip_t:
					rtp = rt_racc_voip_t_a(dbp,pre);
					break;			
			case sms:
					rtp = rt_racc_sms_a(dbp,pre);
					break;
		};
    }

    if(leg == 'b') {
/*		switch(t) {
			case unkn:
					rating_unkn_b(dbp,pre);
					break;
			case isup:
					rating_isup_b(dbp,pre);
					break;
			case voip_a:
					break;
			case voip_v:
					break;
			case voip_t:
					rating_voip_t_b(dbp,pre);
					break;
			case sms:
					rating_sms_b(dbp,pre);
					break;
		}; */
    }

	if(rtp == NULL) return;
	if(rtp->pre == NULL) return;
	if(rtp->bacc_ptr == NULL) return;

	rtp->pre = pre;

	if((rtp->bacc_ptr->id)||(strlen(rtp->bacc_ptr->username)>0)) {			
		pcard_manager(dbp,rtp);
		LOG("rating_main()","pcard_manager,bacc_id %d,bacc %s",rtp->bacc_ptr->id,rtp->bacc_ptr->username);	
	}

    rt_exec(dbp,rtp,leg);
 
	rt_data_racc_free(rtp);
}


void rt_loop(rate_engine_t *rt_eng)
{
	rating_t rt;
	rating_t *pre;
	
	struct timeval tim;
    double t1,t2;
	char msg[512];
	struct timeval times;
	
    cdr_t *cdrs = 0;
    
    int pp = 0;
    int cc = 0;
    
	pre = &rt;
    
    if(log_debug_level >= LOG_LEVEL_INFO) {
		gettimeofday(&tim, NULL);
		t1 = tim.tv_sec+(tim.tv_usec/1000000.0);
	}
        
	cdrs = cdrm_api->get_cdrs(rt_eng->dbp,rt_eng->leg,0);

    if(cdrs) {
		pp=0;
		
		while(cdrs[pp].id > 0) {
			rt_rating_init(pre);			
			rt_cdr_to_rating(&cdrs[pp],pre);
			
			gettimeofday(&times, NULL);
			pre->start_timer = (((times.tv_sec)*1000000)+(times.tv_usec));
			
			rt_main(rt_eng->dbp,pre,rt_eng->leg,cdrs[pp].cdr_rec_type_id);
			
			gettimeofday(&times, NULL);
			pre->current_timer = (((times.tv_sec)*1000000)+(times.tv_usec));
			
			char msg4[1024];
			sprintf(msg4,"rating call times: %f,cdr_id: %d,call_uid: %s",
			(double)((pre->current_timer)-(pre->start_timer))/1000000,cdrs[pp].id,cdrs[pp].call_uid);
			
			LOG("rating_loop()",msg4);
			
			if((pre->rating_id) > 0) cc++;
			
			usleep(rt_eng->wait_rating);
			
			pp++;
		}

		sprintf(msg,"Number of rating cdrs (leg %c) = [%d/%d]",rt_eng->leg,pp,cc);
		
		LOG("rating_loop",msg);
		
		mem_free(cdrs);
    }
    
    if(log_debug_level >= LOG_LEVEL_INFO) {
		gettimeofday(&tim, NULL);
		t2 = tim.tv_sec+(tim.tv_usec/1000000.0);
				
		char msg3[1024];
		sprintf(msg3,"times: %f %f",(t2-t1),((t2-t1)/cc));
//		sprintf(msg3,"times: %f %f",(t2-t1),((t2-t1)/pp));
	
		LOG("rating_loop()",msg3);
	}
}

int rt_init(void)
{
	mod_t *mod_ptr;

	mod_ptr = mod_find_module(rt_mod_dep[0].dep_mod_name);

	if(mod_ptr == NULL) return -1;
	if(mod_ptr->handle == NULL) return -2;
	
	cdrm_api = (cdr_funcs_t *)mod_find_sim(mod_ptr->handle,"cdrm_api");
	if(cdrm_api == NULL) {
		LOG("rt_init()","ERROR! struct 'cdrm_api' is not ready!");
		
		return -3;
	}

	memset(&rt_eng,0,sizeof(rate_engine_t));
	
	rt_eng.dbp = db_init();

	rt_eng.dbp->conn = db_conn_init(mcfg->dbtype,mcfg->dbhost,mcfg->dbname,mcfg->dbport,mcfg->dbuser,mcfg->dbpass,0);
		
	if(db_engine_bind(rt_eng.dbp) < 0) {
		LOG("rt_init()","db_engine_bind() ERROR - local '%s' db server",rt_eng.dbp->conn->enginename);
		goto error;
	}
				
	if(db_connect(rt_eng.dbp) < 0) {
		LOG("rt_init()","db_connect() ERROR - local '%s' db server",rt_eng.dbp->conn->enginename);
		goto error;
	}
						
	goto success;
	
error:
	if(rt_eng.dbp != NULL) db_free(rt_eng.dbp);		 
	
	return RE_ERROR_N;
	
success:
	return RE_SUCCESS;
}

int rt_free(void)
{	
	int ret;
	
	if(rt_eng.dbp != NULL) {
		if(rt_eng.dbp->conn != NULL) {
			ret = db_close(rt_eng.dbp);
			if(ret < 0) db_error(ret);
		}
		
		db_free(rt_eng.dbp);
	} else return RE_ERROR;

	return RE_SUCCESS;
}

void *RateEngine(void *dt)
{
	rt_cfg_t *cfg;
	
	cfg = rt_cfg_main(mcfg->cfg_filename);
	
	if(cfg == NULL) {
		LOG("RateEngine","rt_cfg_main() error");
		goto _end;
	}
	
	if((mcfg->daemon_flag)&&((cfg->rating_active_flag) == 'f')) goto _end;
		
	if(rt_init() < 0) {
		LOG("RateEngine","rt_init() returned error!");
		goto _end;
	}
	
	rt_eng.active = cfg->rating_active_flag;
	rt_eng.rating_interval = cfg->rating_interval;
	rt_eng.leg = opt_cli_mem.leg[0];
	rt_eng.no_prefix_rating = cfg->no_prefix_rating;
	rt_eng.rating_flag = rating_flag;
	
	strcpy(rt_eng.rating_mode,opt_cli_mem.rating_mode);
	strcpy(rt_eng.rating_account,opt_cli_mem.rating_account);
	
	if(strcmp(cfg->pcard_sort_key,"") == 0) strcpy(rt_eng.pcard_sort_key,PCARD_SORT_KEY);
	else strcpy(rt_eng.pcard_sort_key,cfg->pcard_sort_key);
	
	if(strcmp(cfg->pcard_sort_mode,"") == 0) strcpy(rt_eng.pcard_sort_key,PCARD_SORT_MODE);
	else strcpy(rt_eng.pcard_sort_mode,cfg->pcard_sort_mode);
			
	if((rt_eng.leg == '\0')&&(cfg->leg != '\0')) {
		rt_eng.leg = cfg->leg;
	}
	
	if(cfg->bal_num > 0) rt_eng.bal_num = cfg->bal_num; 
	else rt_eng.bal_num = RT_BAL_NUM;
		
	if(cfg->wait_rating > 0) rt_eng.wait_rating = cfg->wait_rating;
	else rt_eng.wait_rating = WAIT_RATING;

	if(strlen(cfg->rt_cfg_json_dir) > 0) strcpy(rt_eng.rt_cfg_json_dir,cfg->rt_cfg_json_dir);

	mem_free(cfg);
	
    loop:
    {
		LOG("RateEngine","RateEngine is started ...");
						
		/* daemon,without leg value */
		if(rt_eng.leg == '\0') {
			rt_eng.leg = 'a';
			LOG("RateEngine","leg_a ...");
			rt_loop(&rt_eng);
			
			LOG("RateEngine","leg_b ...");
			rt_eng.leg = 'b';
			rt_loop(&rt_eng);
			
			rt_eng.leg = '\0';
		} else {
			LOG("RateEngine","leg_%c ...",rt_eng.leg);
			rt_loop(&rt_eng);
		}

		LOG("RateEngine","RateEngine is finished!");
				
		if(rt_eng.active == 't') {
			sleep(rt_eng.rating_interval);
			goto loop;
		}
		else loop_flag = 'f';
	}
	
	rt_free();

_end:
    pthread_exit(NULL);
}
