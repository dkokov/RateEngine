//#include <stdio.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "../../misc/exten/time_funcs.h"
#include "../../misc/exten/json_ext.h"

#include "rt_data.h"
#include "rt_json.h"

static json_ext_obj_t rt_json_bacc_temp[] = {
	{json_str,RT_JSON_BACC_USR_STR},
	{json_str,RT_JSON_CURR_STR},
	{json_ushrt,RT_JSON_BDAY_STR},
	{json_ushrt,RT_JSON_DOP_STR},
	{json_str,RT_JSON_ROUND_STR},
	{json_arr,RT_JSON_PCARD_STR},
	{0,""}
};

static json_ext_obj_t rt_json_pcard_temp[] = {
	{json_int_arr,RT_JSON_PCARD_TYPE_STR},
	{json_int_arr,RT_JSON_PCARD_STATUS_STR},
	{json_str_arr,RT_JSON_PCARD_START_STR},
	{json_str_arr,RT_JSON_PCARD_END_STR},
	{json_double_arr,RT_JSON_PCARD_AMOUNT_STR},
	{json_int_arr,RT_JSON_PCARD_SIM_CALLS_STR},
	{0,""}
};

static json_ext_obj_t rt_json_racc_temp[] = {
	{json_int,RT_JSON_RACC_MODE_STR},
	{json_str,RT_JSON_RACC_USR_STR},
	{json_str,RT_JSON_RACC_BACC_STR},
	{json_str,RT_JSON_RACC_BPLAN_STR},
	{0,""}
};

static json_ext_obj_t rt_json_calc_temp[] = {
	{json_int_arr,RT_JSON_CALC_POS_STR},
	{json_int_arr,RT_JSON_CALC_DELTA_STR},
	{json_double_arr,RT_JSON_CALC_TEE_STR},
	{json_int_arr,RT_JSON_CALC_ITER_STR},
	{0,""}
};

static json_ext_obj_t rt_json_tariff_temp[] = {
	{json_str,RT_JSON_TARIFF_NAME_STR},
	{json_int,RT_JSON_TARIFF_START_STR},
	{json_int,RT_JSON_TARIFF_END_STR},
	{json_int,RT_JSON_TARIFF_FSEC_STR},
	{json_arr,RT_JSON_TARIFF_CALC_STR},
	{0,""}
};

static json_ext_obj_t rt_json_bplan_temp[] = {
	{json_int,RT_JSON_BPLAN_START_STR},
	{json_int,RT_JSON_BPLAN_END_STR},
	{0,""}
};

static json_ext_obj_t rt_json_rate_temp[] = {
	{json_str,RT_JSON_RATE_PREFIX_STR},
	{json_str,RT_JSON_RATE_TARIFF_STR},
	{0,""}
};

static json_ext_obj_t rt_json_rated_temp[] = {
	{json_str,RT_JSON_RT_CALL_UID_STR},
	{json_str,RT_JSON_RACC_BACC_STR},
	{json_str,RT_JSON_RACC_BPLAN_STR},
	{json_str,RT_JSON_RATE_PREFIX_STR},
	{json_str,RT_JSON_RATE_TARIFF_STR},
	{json_int,RT_JSON_RT_BILLSEC_STR},
	{json_double,RT_JSON_RT_CPRICE_STR},
	{json_datetime,RT_JSON_RT_TS_STR},
	{0,""}
};

static json_ext_obj_t rt_json_bal_temp[] = {
	{json_str,RT_JSON_BACC_USR_STR},
	{json_datetime,RT_JSON_BAL_START_STR},
	{json_datetime,RT_JSON_BAL_END_STR},
	{json_double,RT_JSON_BAL_AMOUNT_STR},
	{0,""}
};

int rt_json_bacc_put(bacc_t *bpt,rt_json_t *rt_ptr)
{
	json_ext_obj_t *cpy_bacc_temp,*cpy_pcard_temp;
	
	if(bpt == NULL) return -1;
	if(rt_ptr == NULL) return -2;
	
	cpy_pcard_temp = json_ext_new_obj(rt_json_pcard_temp);
	cpy_bacc_temp = json_ext_new_obj(rt_json_bacc_temp);
	json_ext_init_value(cpy_bacc_temp,RT_JSON_PCARD_STR,(void *)cpy_pcard_temp,0);

	json_ext_init_value(cpy_bacc_temp,RT_JSON_BACC_USR_STR,(void *)bpt->username,0);	
	json_ext_init_value(cpy_bacc_temp,RT_JSON_CURR_STR,(void *)bpt->currency,0);
	json_ext_init_value(cpy_bacc_temp,RT_JSON_BDAY_STR,(void *)&bpt->billing_day,0);
	json_ext_init_value(cpy_bacc_temp,RT_JSON_DOP_STR,(void *)&bpt->day_of_payment,0);
	json_ext_init_value(cpy_bacc_temp,RT_JSON_ROUND_STR,(void *)bpt->round_mode,0);
	
	memset(rt_ptr,0,sizeof(rt_json_t));
	
	sprintf(rt_ptr->header,"%s%s",RT_JSON_HDR_PATTERN_BACC,bpt->username);
	
	rt_ptr->msg = json_ext_obj_create(cpy_bacc_temp);
	
	json_ext_delete_obj(cpy_bacc_temp,0);

	return 0;	
}

int rt_json_balance_put(racc_t *rpt,rt_json_t *rt_ptr)
{
	json_ext_obj_t *cpy_bal_temp;
	
	if(rpt == NULL) return -1;
	if(rt_ptr == NULL) return -2;
	
	cpy_bal_temp = json_ext_new_obj(rt_json_bal_temp);

	json_ext_init_value(cpy_bal_temp,RT_JSON_BACC_USR_STR,(void *)rpt->bacc_ptr->username,0);	
	json_ext_init_value(cpy_bal_temp,RT_JSON_BAL_AMOUNT_STR,(void *)&rpt->bal_ptr->amount,0);
	json_ext_init_value(cpy_bal_temp,RT_JSON_BAL_START_STR,(void *)&rpt->bacc_ptr->pcard_ptr->start,0);
	json_ext_init_value(cpy_bal_temp,RT_JSON_BAL_END_STR,(void *)&rpt->bacc_ptr->pcard_ptr->end,0);	
	
	memset(rt_ptr,0,sizeof(rt_json_t));
	
//	sprintf(rt_ptr->header,"%s%s_%s",RT_JSON_HDR_PATTERN_BAL,rpt->bacc_ptr->username,rpt->bacc_ptr->pcard_ptr->start);
	sprintf(rt_ptr->header,"%s%s",RT_JSON_HDR_PATTERN_BAL,rpt->bacc_ptr->username);	
	
	rt_ptr->msg = json_ext_obj_create(cpy_bal_temp);

//	json_ext_delete_obj(cpy_bacc_temp,0);

	return 0;	
}

int rt_json_rated_put(rating_t *pre,rt_json_t *rt_ptr)
{	
	racc_t *rtp;
	json_ext_obj_t *cpy_rated_temp;
	
	if(pre == NULL) return -1;
	if(rt_ptr == NULL) return -2;
//	if(pre->racc_ptr == NULL) return -3;
	
//	rtp = (racc_t *)pre->racc_ptr;
	
	cpy_rated_temp = json_ext_new_obj(rt_json_rated_temp);

	json_ext_init_value(cpy_rated_temp,RT_JSON_RT_CALL_UID_STR,(void *)pre->call_uid,0);
	json_ext_init_value(cpy_rated_temp,RT_JSON_RACC_BACC_STR,(void *)rtp->bacc_ptr->username,0);	
	json_ext_init_value(cpy_rated_temp,RT_JSON_RACC_BPLAN_STR,(void *)rtp->bplan_ptr->bplan_name,0);
	json_ext_init_value(cpy_rated_temp,RT_JSON_RATE_PREFIX_STR,(void *)rtp->bplan_ptr->rates_ptr->prefix,0);
	json_ext_init_value(cpy_rated_temp,RT_JSON_RATE_TARIFF_STR,(void *)rtp->bplan_ptr->rates_ptr->tariff_name,0);	
	json_ext_init_value(cpy_rated_temp,RT_JSON_RT_BILLSEC_STR,(void *)&pre->billsec,0);
	json_ext_init_value(cpy_rated_temp,RT_JSON_RT_CPRICE_STR,(void *)&pre->cprice,0);
	
	memset(pre->timestamp,0,32);
	current_datetime(pre->timestamp);
	json_ext_init_value(cpy_rated_temp,RT_JSON_RT_TS_STR,(void *)pre->timestamp,0);
	
	memset(rt_ptr,0,sizeof(rt_json_t));
	
	sprintf(rt_ptr->header,"%s%s_%s_%s",RT_JSON_HDR_PATTERN_RT,rtp->bacc_ptr->username,rtp->bacc_ptr->pcard_ptr->start,pre->call_uid);
	
	rt_ptr->msg = json_ext_obj_create(cpy_rated_temp);
	
//	json_ext_delete_obj(cpy_rated_temp,0);

	return 0;	
}

int rt_json_bacc_get(rt_json_t *rt_ptr,bacc_t *bpt)
{
	int i,c,p;
	json_ext_obj_t *cpy_bacc_temp,*cpy_pcard_temp,*pcard_json;
	
	cpy_pcard_temp = json_ext_new_obj(rt_json_pcard_temp);
	cpy_bacc_temp = json_ext_new_obj(rt_json_bacc_temp);
	json_ext_init_value(cpy_bacc_temp,RT_JSON_PCARD_STR,(void *)cpy_pcard_temp,0);

	rt_ptr->jobj = json_tokener_parse(rt_ptr->msg);
	if(rt_ptr->jobj == NULL) return -3;

	json_ext_get_obj(rt_ptr->jobj,cpy_bacc_temp);

	i=0;
	while(cpy_bacc_temp[i].t) {
		if(strcmp(cpy_bacc_temp[i].name,RT_JSON_BACC_USR_STR) == 0) strcpy(bpt->username,cpy_bacc_temp[i].u.str);
		else if(strcmp(cpy_bacc_temp[i].name,RT_JSON_CURR_STR) == 0) strcpy(bpt->currency,cpy_bacc_temp[i].u.str);		
		else if(strcmp(cpy_bacc_temp[i].name,RT_JSON_BDAY_STR) == 0) bpt->billing_day = *cpy_bacc_temp[i].u.us;
		else if(strcmp(cpy_bacc_temp[i].name,RT_JSON_DOP_STR) == 0) bpt->day_of_payment = *cpy_bacc_temp[i].u.us;
		else if(strcmp(cpy_bacc_temp[i].name,RT_JSON_PCARD_STR) == 0) {			
			pcard_json = cpy_bacc_temp[i].u.arr;

			bpt->pcard_ptr = (pcard_t *)mem_alloc_arr(cpy_bacc_temp[i].num_arr,sizeof(pcard_t));

			for(p=0;p<cpy_bacc_temp[i].num_arr;p++)	{
				c=0;
				while(pcard_json[c].t) {
					if(strcmp(pcard_json[c].name,RT_JSON_PCARD_STATUS_STR) == 0) {
						bpt->pcard_ptr[p].status = *pcard_json[c].u.num2[p];
					} else if(strcmp(pcard_json[c].name,RT_JSON_PCARD_TYPE_STR) == 0) {
						bpt->pcard_ptr[p].type = *pcard_json[c].u.num2[p];
					} else if(strcmp(pcard_json[c].name,RT_JSON_PCARD_AMOUNT_STR) == 0) {
						bpt->pcard_ptr[p].amount = *pcard_json[c].u.dnum2[p];
					} else if(strcmp(pcard_json[c].name,RT_JSON_PCARD_SIM_CALLS_STR) == 0) {
						bpt->pcard_ptr[p].call_number = *pcard_json[c].u.num2[p];
					} else if(strcmp(pcard_json[c].name,RT_JSON_PCARD_START_STR) == 0) {
						strcpy(bpt->pcard_ptr[p].start,pcard_json[c].u.str2[p]);
					} else if(strcmp(pcard_json[c].name,RT_JSON_PCARD_END_STR) == 0) {
						strcpy(bpt->pcard_ptr[p].end,pcard_json[c].u.str2[p]);
					}
				
					c++;
				}
			}
		}
		
		i++;
	}

	json_ext_delete_obj(cpy_bacc_temp,0);

	return 0;
}

int rt_json_bplan_get(rt_json_t *rt_ptr,bplan_t *bpt)
{
	int i;
	json_ext_obj_t *cpy_bplan_temp;
	
	cpy_bplan_temp = json_ext_new_obj(rt_json_bplan_temp);

	rt_ptr->jobj = json_tokener_parse(rt_ptr->msg);
	if(rt_ptr->jobj == NULL) return -3;

	json_ext_get_obj(rt_ptr->jobj,cpy_bplan_temp);

	i=0;
	while(cpy_bplan_temp[i].t) {
		if(strcmp(cpy_bplan_temp[i].name,RT_JSON_BPLAN_START_STR) == 0) bpt->bplan_start_period = *cpy_bplan_temp[i].u.num;
		else if(strcmp(cpy_bplan_temp[i].name,RT_JSON_BPLAN_END_STR) == 0) bpt->bplan_end_period = *cpy_bplan_temp[i].u.num;		
		
		i++;
	}

	json_ext_delete_obj(cpy_bplan_temp,0);
	
	return 0;
}

int rt_json_racc_get(rt_json_t *rt_ptr,racc_t *rpt)
{
	int i;
	json_ext_obj_t *cpy_racc_temp;
	
	if(rpt == NULL) return -1;
	if(rt_ptr == NULL) return -2;	
	
	cpy_racc_temp = json_ext_new_obj(rt_json_racc_temp);
	
	rt_ptr->jobj = json_tokener_parse(rt_ptr->msg);
	if(rt_ptr->jobj == NULL) return -3;

	json_ext_get_obj(rt_ptr->jobj,cpy_racc_temp);

	i=0;
	while(cpy_racc_temp[i].t) {
		if(strcmp(cpy_racc_temp[i].name,RT_JSON_RACC_MODE_STR) == 0) rpt->rtm = *cpy_racc_temp[i].u.num;
		else if(strcmp(cpy_racc_temp[i].name,RT_JSON_RACC_BACC_STR) == 0) strcpy(rpt->bacc_ptr->username,cpy_racc_temp[i].u.str);
		else if(strcmp(cpy_racc_temp[i].name,RT_JSON_RACC_BPLAN_STR) == 0) strcpy(rpt->bplan_ptr->bplan_name,cpy_racc_temp[i].u.str);
		else if(strcmp(cpy_racc_temp[i].name,RT_JSON_RACC_USR_STR) == 0) strcpy(rpt->rating_account,cpy_racc_temp[i].u.str);
				
		i++;
	}	
	
	json_ext_delete_obj(cpy_racc_temp,0);

	return 0;
}

int rt_json_rate_get(rt_json_t *rt_ptr,rate_t *rpt)
{
	int i;
	
	json_ext_obj_t *cpy_rate_temp;
	
	if(rpt == NULL) return -1;
	if(rt_ptr == NULL) return -2;	
	
	cpy_rate_temp = json_ext_new_obj(rt_json_rate_temp);

	rt_ptr->jobj = json_tokener_parse(rt_ptr->msg);
	if(rt_ptr->jobj == NULL) return -3;

	json_ext_get_obj(rt_ptr->jobj,cpy_rate_temp);

	i=0;
	while(cpy_rate_temp[i].t) {
		if(strcmp(cpy_rate_temp[i].name,RT_JSON_RATE_PREFIX_STR) == 0) strcpy(rpt->prefix,cpy_rate_temp[i].u.str);
		else if(strcmp(cpy_rate_temp[i].name,RT_JSON_RATE_TARIFF_STR) == 0) strcpy(rpt->tariff_name,cpy_rate_temp[i].u.str);
				
		i++;
	}	
	
	json_ext_delete_obj(cpy_rate_temp,0);
		
	return 0;
}

int rt_json_tariff_get(rt_json_t *rt_ptr,rate_t *rpt)
{
	int i,c,p;
	
	json_ext_obj_t *cpy_tariff_temp,*cpy_calc_temp,*calc_json;
	
	if(rpt == NULL) return -1;
	if(rt_ptr == NULL) return -2;	
	
	cpy_calc_temp = json_ext_new_obj(rt_json_calc_temp);
	cpy_tariff_temp = json_ext_new_obj(rt_json_tariff_temp);
	json_ext_init_value(cpy_tariff_temp,RT_JSON_TARIFF_CALC_STR,(void *)cpy_calc_temp,0);

	rt_ptr->jobj = json_tokener_parse(rt_ptr->msg);
	if(rt_ptr->jobj == NULL) return -3;

	json_ext_get_obj(rt_ptr->jobj,cpy_tariff_temp);

	i=0;
	while(cpy_tariff_temp[i].t) {
		if(strcmp(cpy_tariff_temp[i].name,RT_JSON_TARIFF_CALC_STR) == 0) {
			calc_json = cpy_tariff_temp[i].u.arr;
									
			rpt->calc_funcs = (calc_function_t *)mem_alloc_arr(cpy_tariff_temp[i].num_arr,sizeof(calc_function_t));

			for(p=0;p<cpy_tariff_temp[i].num_arr;p++) {
				c=0;
				while(calc_json[c].t) {
					if(strcmp(calc_json[c].name,RT_JSON_CALC_POS_STR) == 0) {
						rpt->calc_funcs[p].pos = *calc_json[c].u.num2[p];
					} else if(strcmp(calc_json[c].name,RT_JSON_CALC_DELTA_STR) == 0) {
						rpt->calc_funcs[p].delta = *calc_json[c].u.num2[p];
					} else if(strcmp(calc_json[c].name,RT_JSON_CALC_TEE_STR) == 0) {
						rpt->calc_funcs[p].fee = *calc_json[c].u.dnum2[p];
					} else if(strcmp(calc_json[c].name,RT_JSON_CALC_ITER_STR) == 0) {
						rpt->calc_funcs[p].iterations = *calc_json[c].u.num2[p];
					}
				
					c++;
				}
			}			
		}
				
		i++;
	}
	
	json_ext_delete_obj(cpy_tariff_temp,0);	
	
	return 0;
}

int rt_json_balance_get(rt_json_t *rt_ptr,racc_t *rtp)
{
	int i;
	
	json_ext_obj_t *cpy_bal_temp;
	
	if(rtp == NULL) return -1;
	if(rt_ptr == NULL) return -2;	
	
	cpy_bal_temp = json_ext_new_obj(rt_json_bal_temp);

	rt_ptr->jobj = json_tokener_parse(rt_ptr->msg);
	if(rt_ptr->jobj == NULL) return -3;

	json_ext_get_obj(rt_ptr->jobj,cpy_bal_temp);

	i=0;
	while(cpy_bal_temp[i].t) {
		if((strcmp(cpy_bal_temp[i].name,RT_JSON_BAL_AMOUNT_STR) == 0)&&(cpy_bal_temp[i].u.dnum != NULL)) rtp->bal_ptr->amount = *cpy_bal_temp[i].u.dnum;
		else if(strcmp(cpy_bal_temp[i].name,RT_JSON_BAL_START_STR) == 0) strcpy(rtp->bal_ptr->start_period,cpy_bal_temp[i].u.str);
		else if(strcmp(cpy_bal_temp[i].name,RT_JSON_BAL_END_STR) == 0) strcpy(rtp->bal_ptr->end_period,cpy_bal_temp[i].u.str);
		
		i++;
	}	
	
	json_ext_delete_obj(cpy_bal_temp,0);
		
	return 0;
}

int rt_json_rated_get(rt_json_t *rt_ptr,rating_t *pre)
{
	int i;
	
	json_ext_obj_t *cpy_rated_temp;
//	racc_t *rtp;
	
	if(pre == NULL) return -1;
	if(rt_ptr == NULL) return -2;	
	
//	rtp = (racc_t *)pre->racc_ptr;
//	if(rtp == NULL) return -3;
	
	cpy_rated_temp = json_ext_new_obj(rt_json_rated_temp);

	rt_ptr->jobj = json_tokener_parse(rt_ptr->msg);
	if(rt_ptr->jobj == NULL) return -4;

	json_ext_get_obj(rt_ptr->jobj,cpy_rated_temp);

	i=0;
	while(cpy_rated_temp[i].t) {
		if((strcmp(cpy_rated_temp[i].name,RT_JSON_RT_CPRICE_STR) == 0)&&(cpy_rated_temp[i].u.dnum != NULL)) {pre->cprice = *cpy_rated_temp[i].u.dnum;printf("\nopa\n");}
//		else if(strcmp(cpy_rate_temp[i].name,RT_JSON_RATE_TARIFF_STR) == 0) strcpy(rpt->tariff_name,cpy_rate_temp[i].u.str);
				
		i++;
	}	
	
	json_ext_delete_obj(cpy_rated_temp,0);
	
	return 0;
}

int rt_json_racc_get_file(rt_json_t *rt_ptr,racc_t *rpt)
{
	int ret;
	
	if(rt_ptr == NULL) return -1;
	if(rt_ptr->filename == NULL) return -2;
	if(rpt == NULL) return -3;
	
	ret = json_ext_read_file(rt_ptr->filename,&rt_ptr->msg);
	if(ret < 0) return -4;
	
	rt_json_racc_get(rt_ptr,rpt);
	
	free(rt_ptr->msg);
	
	return 0;
}
