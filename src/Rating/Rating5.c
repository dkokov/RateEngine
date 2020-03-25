/* 
 * Main rating functionalities , version 5 (RE5 0.5.x)
 * 
 */

#include <string.h>
#include <pthread.h>

#include "../misc/globals.h"
#include "../misc/mem/mem.h"
#include "../DB/db_pgsql.h"

#include "free_billsec.h"
#include "round_billsec.h"
#include "calc_functions.h"
#include "pcard.h"
#include "Rating5.h"

void rating_save(PGconn *conn,rating *pre)
{    
	if(pre->cdr_id > 0) {
		pthread_mutex_lock(&sync_bt_thread);
		
		f_rating_id_query(conn,pre);
		
		if( pre->rating_id > 0) pre->rating_id = 0; 
		else { 
			insert_rating_pgsql(conn,pre);
			f_rating_id_query(conn,pre);
		}
		
		pthread_mutex_unlock(&sync_bt_thread);
	}	
}

void rating_exec(PGconn *conn,rating *pre,char leg)
{
	int ret;
    tariff *tr = 0;
	int billsec_temp = 0;
	pcard *card = 0;
		
    if(pre->bplan) chk_bplan_periods(pre);

    if((pre->bplan)&&(pre->rating_mode_id)) {		
		if(pre->card) card = pre->card;
				
		tr = rate_searching(conn,pre);
	
		if(tr) {	
			if(pre->round_mode_id) 
				billsec_temp = round_billsec(pre->round_mode_id,pre->billusec);
			
			if(billsec_temp) pre->billsec = billsec_temp;
			
			if(pre->rating_mode_id == RT_MODE_CLG_NUM_SMS) {
				if((card > 0)&&(pre->free_billsec_id > 0)) free_billsec_sms_balance(conn,pre,card->start,card->end);
				ret = calc_cprice_sms(tr,pre);
			} else ret = calc_cprice_group(tr,pre);
			
			switch(ret) {
				case 0:
						rating_save(conn,pre);
						break;
				case (-1):
						pre->rating_id = -1;
						break;
				case 1:
						if((rating_double_rating(conn,tr,pre,leg)) == -1)
							pre->rating_id = -1;
			}
			
			tr_free(tr);
		} else pre->rating_id = -1;
    } else pre->rating_id = -1;

	if(pre->rating_id == 0) {
		LOG("rating_exec()","rating_id is '0' after 'rating_save()' / this call_uid %s,call_id %d is already rated",pre->call_uid,pre->cdr_id);
	} else {
		/* mark cdr when a cdr is rated */
		cdr_update_cdr(conn,pre->rating_id,pre->cdr_id,leg);
    
		/* make balance after rating */
		if((card)&&((pre->rating_id) > 0)) {			
			balance_exec(conn,pre,card->start,card->end);
		
			if((pre->free_billsec_limit)&&(pre->rating_mode_id < 7)) free_billsec_exec(conn,pre,card->start,card->end);	
		}
	}
}

int rating_double_rating(PGconn *conn,tariff *tr,rating *pre,char leg)
{
	int ret_1,ret_2;
	int no_free_sec;
	int free_sec;
	int old_billsec;
	pcard *card;
	
	card = pre->card;
	
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
	
	ret_1 = calc_cprice_group(tr,pre);

	if(ret_1 == 0) rating_save(conn,pre);
	else if(ret_1 == 1) {
		LOG("double_rating()","1/ret_2 = 1");
		
		pre->billsec = old_billsec;
		
		goto no_free_billsec;
	} else return -1;

	/* Phase 2 - no free billsec rating */
	pre->billsec = no_free_sec;
	
	if(card) {
		if((strlen(card->start))&&(strlen(card->end))) {
			balance_exec(conn,pre,card->start,card->end);
			
			if((pre->free_billsec_limit)) free_billsec_exec(conn,pre,card->start,card->end);
		}
	}	
		
	no_free_billsec:
	ret_2 = calc_cprice_group(tr,pre);

	if(ret_2 == 0) {
		rating_save(conn,pre);
		
		LOG("double_rating()","2/rating save");
		
		return 0;
	} else if(ret_2 == 1) {
		LOG("double_rating()","2/ret_2 = 1");
		
		rating_save(conn,pre);
		
		return 0;
	} else {
		LOG("double_rating()","2/ret_2 = -1");
		
		return -1;
	}
}

void rating_voip_av_a(PGconn *conn,rating *pre)
{
	f_calling_number_query(conn,pre);
	
	if((pre->bacc) > 0) pre->rating_mode_id = RT_MODE_CLG_NUM;
	else {
		f_account_code_query(conn,pre,pre->account_code);
	
		if(pre->bacc > 0) pre->rating_mode_id = RT_MODE_ACC_CODE;
	}
}

void rating_voip_t_a(PGconn *conn,rating *pre)
{
	f_account_code_query(conn,pre,pre->account_code);
	
	if((pre->bacc == 0)) {
		f_src_context_query(conn,pre,pre->src_context);
		
		if((pre->bacc) > 0) pre->rating_mode_id = RT_MODE_SRC_CONTEXT;
	} else pre->rating_mode_id = RT_MODE_ACC_CODE;
}

void rating_voip_t_b(PGconn *conn,rating *pre)
{
	f_dst_context_query(conn,pre,pre->dst_context);	
	
	if((pre->bacc) > 0) pre->rating_mode_id = RT_MODE_DST_CONTEXT;
}

void rating_isup_a(PGconn *conn,rating *pre)
{
	f_src_tgroup_query(conn,pre,pre->src_tgroup,pre->clg_nadi,pre->cld_nadi);					
	
	if((pre->bacc) > 0) pre->rating_mode_id = RT_MODE_SRC_TGROUP;
}

void rating_isup_b(PGconn *conn,rating *pre)
{
	f_dst_tgroup_query(conn,pre,pre->dst_tgroup,pre->clg_nadi,pre->cld_nadi);
	
	if((pre->bacc) > 0) pre->rating_mode_id = RT_MODE_DST_TGROUP;
}

void rating_sms_a(PGconn *conn,rating *pre)
{
	f_calling_number_sms_query(conn,pre);
	
	if((pre->bacc) > 0) pre->rating_mode_id = RT_MODE_CLG_NUM_SMS;
}

void rating_sms_b(PGconn *conn,rating *pre)
{
	
}

void rating_unkn_a(PGconn *conn,rating *pre)
{
	f_calling_number_query(conn,pre);

	if((pre->bacc == 0)) {
		f_account_code_query(conn,pre,pre->account_code);
			
		if((pre->bacc == 0)) {
			f_src_context_query(conn,pre,pre->src_context);
				
			if((pre->bacc == 0)) {
				f_src_tgroup_query(conn,pre,pre->src_tgroup,pre->clg_nadi,pre->cld_nadi);
					
				if((pre->bacc == 0)) {
					if(log_debug_level > LOG_LEVEL_INFO) {
						LOG("rating_unkn_a()","Rating5-LegA error,call_uid %s",pre->call_uid);
					}
				} else pre->rating_mode_id = RT_MODE_SRC_TGROUP;
			} else pre->rating_mode_id = RT_MODE_SRC_CONTEXT;
		} else pre->rating_mode_id = RT_MODE_ACC_CODE;
	} else pre->rating_mode_id = RT_MODE_CLG_NUM;
}

void rating_unkn_b(PGconn *conn,rating *pre)
{
	if(strcmp(pre->dst_context,"")) f_dst_context_query(conn,pre,pre->dst_context);
	
	if((pre->bacc == 0)) {
	    if(strcmp(pre->dst_tgroup,"")) f_dst_tgroup_query(conn,pre,pre->dst_tgroup,pre->clg_nadi,pre->cld_nadi);
	
	    if((pre->bacc == 0)) {
			if(log_debug_level > LOG_LEVEL_INFO) {
				LOG("rating_unkn_b()","Rating5-LegB error,call_uid %s",pre->call_uid);
			}
	    } else pre->rating_mode_id = RT_MODE_DST_TGROUP;
	} else pre->rating_mode_id = RT_MODE_DST_CONTEXT;
}

/* Main rating function */
void rating_main(PGconn *conn,rating *pre,char leg,cdr_rec_type_t t)
{
    pre->rating_mode_id = 0;

    if(leg == 'a') {
		switch(t) {
			case unkn:
					rating_unkn_a(conn,pre);
					break;
			case isup:
					rating_isup_a(conn,pre);
					break;
			case voip_a:
					rating_voip_av_a(conn,pre);
					break;
			case voip_v:
					break;
			case voip_t:
					rating_voip_t_a(conn,pre);
					break;			
			case sms:
					rating_sms_a(conn,pre);
					break;
		};
    }

    if(leg == 'b') {
		switch(t) {
			case unkn:
					rating_unkn_b(conn,pre);
					break;
			case isup:
					rating_isup_b(conn,pre);
					break;
			case voip_a:
					break;
			case voip_v:
					break;
			case voip_t:
					rating_voip_t_b(conn,pre);
					break;
			case sms:
					rating_sms_b(conn,pre);
					break;
		};
    }

	if(pre->bacc) {			
		pcard_manager_v2(conn,pre);
		LOG("rating_main()","pcard_manager_v2,bacc %d",pre->bacc);	
	}
		
    rating_exec(conn,pre,leg);
 
    if(pre->card) pcard_free(pre->card);
}
