/*
 * JSONRPC-RT Protocol , version 1, 2019-08-02
 * 
 * --> {"jsonrpc": "2.0", "method": "handshake", "params": {"instanse": "RE1","timestamp": 1564753233}, "id": 1}
 * <-- {"jsonrpc": "2.0", "result": {"state": "OK","timestamp": 1564753235}, "id": 1}
 * 
 * --> {"jsonrpc": "2.0", "method": "handshake", "params": {"instanse": "RE2","timestamp": 1564753233}, "id": 2}
 * <-- {"jsonrpc": "2.0", "result": {"state": "OK","timestamp": 1564753235}, "id": 2}
 * 
 * --> {"jsonrpc": "2.0", "method": "action", "params": {"instanse": "RE1","timestamp": 1564753233}, "id": 3}
 * <-- {"jsonrpc": "2.0", "result": {"state": "OK","timestamp": 1564753243}, "id": 3}
 * 
 * <-- {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request"}, "id": 0}
 * 
 * */

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../mod.h"

#include "jsonrpc_rt.h"

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

static jsonrpc_proto_type_t rt_proto_type[] = {
	{"get",NULL,NULL},
	{"",NULL,NULL}
};

void jsonrpc_rt_mod_init(void)
{
	
}

int jsonrpc_rt_recv_msg_recognize(jsonrpc_t *jrpc,char *msg)
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
