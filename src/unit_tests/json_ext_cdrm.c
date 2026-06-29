/*
 * gcc -Wall -g -o json_ext_cdrm json_ext_cdrm.c -ljson-c -L../ -lre7core
 * 
 * valgrind --leak-check=full --show-leak-kinds=all ./json_ext_cdrm
 * 
 * This example tests 'json_ext' + 'cdrm_json'
 * 
 * */
 
#include <stdio.h>
#include <string.h>

#include "../misc/exten/json_ext.h"

#include "../config/xml_cfg.h"

#include "../db/db.h"
#include "../mod/CDRMediator/prefix_filter.h"
#include "../mod/CDRMediator/cdr.h"
#include "../mod/CDRMediator/cdrm_json.h"

static json_ext_obj_t cdr_temp[] = {
	{json_ushrt,CDR_JSON_CDR_SERVER_STR},
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

int main(void)
{	
	char *buf;

	cdr_t *cdr_ptr;
	json_ext_obj_t *cpy_cdr_temp;
	
	//memset(&cdr_ptr,0,sizeof(cdr_t));
	cdr_ptr = (cdr_t *)calloc(1,sizeof(cdr_t));
	
	cdr_ptr->cdr_server_id = 1;
	cdr_ptr->cdr_rec_type_id = 3;
	strcpy(cdr_ptr->call_uid,"test-1111");
	strcpy(cdr_ptr->calling_number,"35924119998");
	strcpy(cdr_ptr->called_number,"359882079235");
	
	cpy_cdr_temp = json_ext_new_obj(cdr_temp);

	json_ext_init_value(cpy_cdr_temp,CDR_JSON_CDR_SERVER_STR,(void *)&cdr_ptr->cdr_server_id,0);
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_CDR_REC_TYPE_STR,(void *)&cdr_ptr->cdr_rec_type_id,0);
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_CALL_UID_STR,(void *)cdr_ptr->call_uid,0);
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_CLG_STR,(void *)cdr_ptr->calling_number,0);
	json_ext_init_value(cpy_cdr_temp,CDR_JSON_CLD_STR,(void *)cdr_ptr->called_number,0);
	
//	if(cdr_ptr->start_epoch > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_START_EPOCH_STR,(void *)&cdr_ptr->start_epoch,0);
//	if(cdr_ptr->answer_epoch > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_ANSWER_EPOCH_STR,(void *)&cdr_ptr->answer_epoch,0);
//	if(cdr_ptr->end_epoch > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_END_EPOCH_STR,(void *)&cdr_ptr->end_epoch,0);
	
//	json_ext_init_value(cpy_cdr_temp,CDR_JSON_BILLSEC_STR,(void *)&cdr_ptr->billsec,0);
//	json_ext_init_value(cpy_cdr_temp,CDR_JSON_DURATION_STR,(void *)&cdr_ptr->duration,0);

//	if(strlen(cdr_ptr->src_context) > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_SRC_CONTEXT_STR,(void *)cdr_ptr->src_context,0);
//	if(strlen(cdr_ptr->dst_context) > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_DST_CONTEXT_STR,(void *)cdr_ptr->dst_context,0);
//	if(strlen(cdr_ptr->src_tgroup) > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_SRC_TGROUP_STR,(void *)cdr_ptr->src_tgroup,0);
//	if(strlen(cdr_ptr->dst_tgroup) > 0) json_ext_init_value(cpy_cdr_temp,CDR_JSON_DST_TGROUP_STR,(void *)cdr_ptr->dst_tgroup,0);


	/* put json,write */
	buf = json_ext_obj_create(cpy_cdr_temp);
	
	json_ext_delete_obj(cpy_cdr_temp,-1);

	free(cdr_ptr);

	printf("\nbuf: %s\n\n",buf);

	json_ext_write_file("cdr_test.json",buf);
	
	free(buf);
	
	return 0;
}

