/*
 * JSON CDR string:
 * cdr_1111 {"call-uid": "1111", "clg": "359112","cld": "359111", "billsec": 20,"duration": 30}
 * 
 */
 
#include <stdio.h>
#include <time.h>

#include "../../misc/globals.h"
#include "../../misc/exten/str_ext.h"
#include "../../misc/exten/time_funcs.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "cdrm_json.h"

static json_ext_obj_t cdr_temp[] = {
	{json_str  ,CDR_JSON_CDR_SERVER_STR},
	{json_ushrt,CDR_JSON_CDR_REC_TYPE_STR},
	{json_str  ,CDR_JSON_CALL_UID_STR},
	{json_str  ,CDR_JSON_CLG_STR},
	{json_str  ,CDR_JSON_CLD_STR},
	{json_str  ,CDR_JSON_START_TS_STR},
	{json_uint ,CDR_JSON_START_EPOCH_STR},
	{json_uint ,CDR_JSON_ANSWER_EPOCH_STR},
	{json_uint ,CDR_JSON_END_EPOCH_STR},
	{json_uint ,CDR_JSON_BILLSEC_STR},
	{json_uint ,CDR_JSON_DURATION_STR},
	{json_str  ,CDR_JSON_SRC_CONTEXT_STR},
	{json_str  ,CDR_JSON_DST_CONTEXT_STR},
	{json_str  ,CDR_JSON_SRC_TGROUP_STR},
	{json_str  ,CDR_JSON_DST_TGROUP_STR},
	{0,""}
};

static json_ext_obj_t cdr_profile_temp[] = {
	{json_str,CDR_JSON_CDR_SERVER_STR},
	{json_int,CDR_JSON_START_EPOCH_STR},
	{json_str,CDR_JSON_CDR_PROFILE_CFG_FILE_STR},
	{json_arr,CDR_JSON_PFILTER_ARR_STR},
	{0,""}
};

static json_ext_obj_t cdr_pfilters_temp[] = {
	{json_str,CDR_JSON_PFILTER_PREFIX_STR},
	{json_int,CDR_JSON_PFILTER_NUMBER_STR},
	{json_str,CDR_JSON_PFILTER_REPLACE_STR},
	{json_int,CDR_JSON_PFILTER_LEN_STR},
	{0,""}
};

static json_ext_obj_t cdr_sched_temp[] = {
	{json_str,CDR_JSON_CDR_SERVER_STR},
	{json_int,CDR_JSON_CDR_SCHED_TS_STR},
	{json_int,CDR_JSON_CDR_SCHED_LAST_STR},
	{json_int,CDR_JSON_CDR_SCHED_START_STR},
	{json_int,CDR_JSON_CDR_SCHED_REPLIES_STR},
	{0,""}
};

/* converting from the 'CDR struct' in the JSON string */
int cdrm_struct_json(cdr_t *cdr_ptr,cdrm_json_t *cdr_json_ptr)
{	
	json_ext_obj_t *cpy_cdr_temp;
	
	if(cdr_ptr == NULL) return -1;
	if(cdr_json_ptr == NULL) return -2;
	
	cpy_cdr_temp = json_ext_new_obj(cdr_temp);
	
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_CDR_SERVER_STR,(void *)cdr_ptr->profile_name,0);	
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_CDR_REC_TYPE_STR,(void *)&cdr_ptr->cdr_rec_type_id,0);
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_CALL_UID_STR,(void *)cdr_ptr->call_uid,0);
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_CLG_STR,(void *)cdr_ptr->calling_number,0);
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_CLD_STR,(void *)cdr_ptr->called_number,0);

	if(strlen(cdr_ptr->start_ts) > 0) {
		char *buf;
		
		buf = str_ext_replace_symbol(cdr_ptr->start_ts,' ','T');
		
		memset(cdr_ptr->start_ts,0,strlen(cdr_ptr->start_ts));
		strcpy(cdr_ptr->start_ts,buf);
		
		mem_free(buf);
		
		json_ext_init_value(cpy_cdr_temp,CDR_JSON_START_TS_STR,(void *)cdr_ptr->start_ts,0);
	} 
	
	if(cdr_ptr->start_epoch > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_START_EPOCH_STR,(void *)&cdr_ptr->start_epoch,0);
	if(cdr_ptr->answer_epoch > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_ANSWER_EPOCH_STR,(void *)&cdr_ptr->answer_epoch,0);
	if(cdr_ptr->end_epoch > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_END_EPOCH_STR,(void *)&cdr_ptr->end_epoch,0);
	
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_BILLSEC_STR,(void *)&cdr_ptr->billsec,0);
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_DURATION_STR,(void *)&cdr_ptr->duration,0);

	if(strlen(cdr_ptr->src_context) > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_SRC_CONTEXT_STR,(void *)cdr_ptr->src_context,0);
	if(strlen(cdr_ptr->dst_context) > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_DST_CONTEXT_STR,(void *)cdr_ptr->dst_context,0);	

	if(strlen(cdr_ptr->src_tgroup) > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_SRC_TGROUP_STR,(void *)cdr_ptr->src_tgroup,0);	
	if(strlen(cdr_ptr->dst_tgroup) > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_DST_TGROUP_STR,(void *)cdr_ptr->dst_tgroup,0);
	
	memset(cdr_json_ptr,0,sizeof(cdrm_json_t));
	
	sprintf(cdr_json_ptr->header,"%s%s",CDR_JSON_HDR_PATTERN,cdr_ptr->call_uid);
	
	cdr_json_ptr->msg = json_ext_obj_create(cpy_cdr_temp);

	json_ext_delete_obj(cpy_cdr_temp,-1);
	
	return CDR_JSON_OK;
}

/* converting from JSON CDR string in the 'CDR struct' */
int cdrm_json_struct(cdrm_json_t *cdr_json_ptr,cdr_t *cdr_ptr)
{
	int i;
	
	json_ext_obj_t *cpy_cdr_temp;
		
	if(cdr_ptr == NULL) return -1;
	if(cdr_json_ptr == NULL) return -2;

	cpy_cdr_temp = json_ext_new_obj(cdr_temp);
	
	cdr_json_ptr->jobj = json_tokener_parse(cdr_json_ptr->msg);
	if(cdr_json_ptr->jobj == NULL) return -3;

	json_ext_get_obj(cdr_json_ptr->jobj,cpy_cdr_temp);
	
	memset(cdr_ptr,0,sizeof(cdr_t));

	i=0;
	while(cpy_cdr_temp[i].t) {
		if(strcmp(cpy_cdr_temp[i].name,CDR_JSON_CDR_SERVER_STR) == 0) {
			strcpy(cdr_ptr->profile_name,cpy_cdr_temp[i].u.str);
			cdr_ptr->id = 1;
		} else if(strcmp(cpy_cdr_temp[i].name,CDR_JSON_CDR_REC_TYPE_STR) == 0) cdr_ptr->cdr_rec_type_id = *cpy_cdr_temp[i].u.us;
		else if(strcmp(cpy_cdr_temp[i].name,CDR_JSON_CALL_UID_STR) == 0) strcpy(cdr_ptr->call_uid,cpy_cdr_temp[i].u.str);
		else if(strcmp(cpy_cdr_temp[i].name,CDR_JSON_CLG_STR) == 0) strcpy(cdr_ptr->calling_number,cpy_cdr_temp[i].u.str);
		else if((strcmp(cpy_cdr_temp[i].name,CDR_JSON_CLD_STR) == 0)&&(cpy_cdr_temp[i].u.str != NULL)) strcpy(cdr_ptr->called_number,cpy_cdr_temp[i].u.str);
		else if((strcmp(cpy_cdr_temp[i].name,CDR_JSON_START_EPOCH_STR) == 0)&&(cpy_cdr_temp[i].u.ui != NULL)) cdr_ptr->start_epoch = *cpy_cdr_temp[i].u.ui;
		else if((strcmp(cpy_cdr_temp[i].name,CDR_JSON_ANSWER_EPOCH_STR) == 0)&&(cpy_cdr_temp[i].u.ui != NULL)) cdr_ptr->answer_epoch = *cpy_cdr_temp[i].u.ui;
		else if((strcmp(cpy_cdr_temp[i].name,CDR_JSON_END_EPOCH_STR) == 0)&&(cpy_cdr_temp[i].u.ui != NULL)) cdr_ptr->end_epoch = *cpy_cdr_temp[i].u.ui;
		else if(strcmp(cpy_cdr_temp[i].name,CDR_JSON_BILLSEC_STR) == 0) cdr_ptr->billsec = *cpy_cdr_temp[i].u.ui;
		else if(strcmp(cpy_cdr_temp[i].name,CDR_JSON_DURATION_STR) == 0) cdr_ptr->duration = *cpy_cdr_temp[i].u.ui;
		
		i++;
	}
	
	json_ext_delete_obj(cpy_cdr_temp,0);
	
	json_object_put(cdr_json_ptr->jobj);
	
	return CDR_JSON_OK;
}

int cdrm_profile_struct_json(cdr_profile_cfg_t *profile,cdrm_json_t *cdr_json_ptr)
{
	int tt;
	
	json_ext_obj_t *cpy_cdr_profile_temp;
	json_ext_obj_t *cpy_cdr_pfilters_temp;
	
	if(profile == NULL) return -1;
	if(cdr_json_ptr == NULL) return -2;
	
	tt = time(NULL);
	
	cpy_cdr_profile_temp = json_ext_new_obj(cdr_profile_temp);
	cpy_cdr_pfilters_temp = json_ext_new_obj(cdr_pfilters_temp);
	
	json_ext_init_value(cpy_cdr_profile_temp,CDR_JSON_CDR_SERVER_STR,(void *)profile->profile_name,0);
	json_ext_init_value(cpy_cdr_profile_temp,CDR_JSON_CDR_PROFILE_CFG_FILE_STR,(void *)profile->filename,0);
	json_ext_init_value(cpy_cdr_profile_temp,CDR_JSON_START_EPOCH_STR,(void *)&tt,0);
	json_ext_init_value(cpy_cdr_profile_temp,CDR_JSON_PFILTER_ARR_STR,(void *)cpy_cdr_pfilters_temp,0);

	memset(cdr_json_ptr,0,sizeof(cdrm_json_t));
	
	sprintf(cdr_json_ptr->header,"%s%s",CDR_JSON_CDR_PROFILE_HDR_PATTERN,profile->profile_name);
	
	cdr_json_ptr->msg = json_ext_obj_create(cpy_cdr_profile_temp);
	
	json_ext_delete_obj(cpy_cdr_profile_temp,-1);
	json_ext_delete_obj(cpy_cdr_pfilters_temp,-1);
		
	return CDR_JSON_OK;
}

int cdrm_sched_struct_json(cdr_storage_profile_t *cfg,cdrm_json_t *cdr_json_ptr)
{	
	int start;
	json_ext_obj_t *cpy_cdr_sched_temp;
	
	if(cfg == NULL) return -1;
	if(cdr_json_ptr == NULL) return -2;
	
	start = (int)convert_ts_to_epoch(cfg->cdr_sched_ts);
	
	cpy_cdr_sched_temp = json_ext_new_obj(cdr_sched_temp);
	
	json_ext_init_value(cpy_cdr_sched_temp,CDR_JSON_CDR_SERVER_STR,(void *)cfg->profile_name,0);
	json_ext_init_value(cpy_cdr_sched_temp,CDR_JSON_CDR_SCHED_TS_STR,(void *)&cfg->ts,0);
	json_ext_init_value(cpy_cdr_sched_temp,CDR_JSON_CDR_SCHED_LAST_STR,(void *)&cfg->chk_ts,0);
	json_ext_init_value(cpy_cdr_sched_temp,CDR_JSON_CDR_SCHED_START_STR,(void *)&start,0);
//	json_ext_init_value(cpy_cdr_sched_temp,CDR_JSON_CDR_SCHED_REPLIES_STR,(void *)&cfg->replies,0);	
	
	memset(cdr_json_ptr,0,sizeof(cdrm_json_t));
	
	sprintf(cdr_json_ptr->header,"%s%s",CDR_JSON_CDR_SCHED_HDR_PATTERN,cfg->profile_name);
	
	cdr_json_ptr->msg = json_ext_obj_create(cpy_cdr_sched_temp);
	
	json_ext_delete_obj(cpy_cdr_sched_temp,-1);
		
	return CDR_JSON_OK;
}

int cdrm_json_sched_struct(cdrm_json_t *cdr_json_ptr,cdr_storage_sched_t *sched)
{
	int i;
	
	json_ext_obj_t *cpy_cdr_sched_temp;
	
	if(sched == NULL) return -1;
	if(cdr_json_ptr == NULL) return -2;	
	if(cdr_json_ptr->msg == NULL) return -3;
	
	cpy_cdr_sched_temp = json_ext_new_obj(cdr_sched_temp);

	cdr_json_ptr->jobj = json_tokener_parse(cdr_json_ptr->msg);

	json_ext_get_obj(cdr_json_ptr->jobj,cpy_cdr_sched_temp);

	memset(sched,0,sizeof(cdr_storage_sched_t));

	i=0;
	while(cpy_cdr_sched_temp[i].t) {
		if(strcmp(cpy_cdr_sched_temp[i].name,CDR_JSON_CDR_SCHED_TS_STR) == 0) sched->ts = *cpy_cdr_sched_temp[i].u.num;
		else if(strcmp(cpy_cdr_sched_temp[i].name,CDR_JSON_CDR_SCHED_LAST_STR) == 0) sched->last = *cpy_cdr_sched_temp[i].u.num;
		else if(strcmp(cpy_cdr_sched_temp[i].name,CDR_JSON_CDR_SCHED_START_STR) == 0) sched->start = *cpy_cdr_sched_temp[i].u.num;

		i++;
	}
	
	json_ext_delete_obj(cpy_cdr_sched_temp,0);
	
	json_object_put(cdr_json_ptr->jobj);
	
	return CDR_JSON_OK;
}
