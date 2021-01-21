/*
 * JSONRPC-CC Protocol , version 1, 2019-08-02
 * 
 * --> {"jsonrpc": "2.0", "method": "maxsec", "params": {"cdr_server_id": 3,"call-uid": "1111", "clg": "359112","cld": "359111"}, "id": 3}
 * <-- {"jsonrpc": "2.0", "result": {"maxsec": 3600}, "id": 3}
 * 
 * --> {"jsonrpc": "2.0", "method": "balance", "params": {"clg": "359112"}, "id": 31}
 * <-- {"jsonrpc": "2.0", "result": {"amount": 2.41}, "id": 31}
 * 
 * --> {"jsonrpc": "2.0", "method": "cprice", "params": {"clg": "359112","cld":"359880001","billsec":120}, "id": 61}
 * <-- {"jsonrpc": "2.0", "result": {"amount": 0.01}, "id": 61}
 * 
 * --> {"jsonrpc": "2.0", "method": "rate", "params": {"clg": "359112","cld":"359880001"}, "id": 611}
 * <-- {"jsonrpc": "2.0", "result": {"calc_funcs": [{"pos":1,"delta":30,"fee":0.023,"iterations":1},{"pos":2,"delta":1,"fee":0.00163,"iterations":0}]}, "id": 611}
 * 
 * --> {"jsonrpc": "2.0", "method": "state", "params": {"cdr_server_id": 3}, "id": 62}
 * <-- {"jsonrpc": "2.0", "result": {"status": "idle"}, "id": 62}
 * 
 * <-- {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request"}, "id": 0}
 * 
 * */

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../mod.h"

//#include "../Rating/rating.h"
//#include "../CallControl/cc.h"
//#include "../CallControl/cc_bind_api.h"

#include "jsonrpc_cc.h"

static json_ext_obj_t maxsec_params[] = {
	{json_ushrt,JSONRPC_CC_CDR_SERV_ID},
	{json_str,JSONRPC_CC_CALL_UID_STR},
	{json_str,JSONRPC_CC_CLG_STR},
	{json_str,JSONRPC_CC_CLD_STR},
	{0,""}
};

static json_ext_obj_t maxsec_result[] = {
	{json_int,JSONRPC_CC_MAXSEC_STR},
	{0,""}
};

static json_ext_obj_t balance_params[] = {
	{json_str,JSONRPC_CC_CLG_STR},
	{0,""}
};

static json_ext_obj_t balance_result[] = {
	{json_double,JSONRPC_CC_AMOUNT_STR},
	{0,""}
};

static json_ext_obj_t rate_params[] = {
	{json_str,JSONRPC_CC_CLG_STR},
	{json_str,JSONRPC_CC_CLD_STR},
	{0,""}
};

static json_ext_obj_t rate_result[] = {
	{json_double,JSONRPC_CC_AMOUNT_STR},
	{0,""}
};

static json_ext_obj_t state_params[] = {
	{json_ushrt,JSONRPC_CC_CDR_SERV_ID},
	{0,""}
};

static json_ext_obj_t state_result[] = {
	{json_str,JSONRPC_CC_SRV_STATUS},
	{0,""}
};

static jsonrpc_proto_type_t cc_proto_type[] = {
	{JSONRPC_CC_MAXSEC_STR,maxsec_params,maxsec_result},
	{JSONRPC_CC_BALANCE_STR,balance_params,balance_result},
	{JSONRPC_CC_RATE_STR,rate_params,rate_result},
	{JSONRPC_CC_SRV_STATE,state_params,state_result},
	{"",NULL,NULL}
};

void jsonrpc_cc_mod_init(void)
{
	
}

int jsonrpc_cc_request_recognize(jsonrpc_t *jrpc,char *msg)
{
	if(jrpc == NULL) return JSONRPC_OBJ_ERR;
	if(msg == NULL) return JSONRPC_OBJ_ERR;
	
	jrpc = json_rpc_init();
	
	jrpc->msg = msg;
	
	t = json_rpc_proto_get(jrpc);
	
	if(t == JSONRPC_OK) {
		if(jrpc->t == JSONRPC_TYPE_REQUEST) {
			json_rpc_proto_init(jrpc,proto_params);
			
		}
		//else if(jrpc->t == JSONRPC_TYPE_ERROR) json_rpc_proto_init(jrpc,json_rpc_err_msg);
		//else if(jrpc->t == JSONRPC_TYPE_RESPONSE) json_rpc_proto_init(jrpc,result);
	
		if((jrpc->jobj != NULL)&&(jrpc->obj != NULL)) {
			json_ext_get_obj(jrpc->jobj,jrpc->obj);
			//json_ext_print_obj(jrpc->obj,0);
		}
	}
	
	json_rpc_free(jrpc);
	
	return JSONRPC_ERR_INTER;
}
