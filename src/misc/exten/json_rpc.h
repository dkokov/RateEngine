#ifndef JSON_RPC_H
#define JSON_RPC_H

#include "json_ext.h"

#define JSONRPC_STR     "jsonrpc"
#define JSONRPC_METHOD  "method"
#define JSONRPC_RESULT  "result"
#define JSONRPC_ERROR   "error"
#define JSONRPC_PARAMS  "params"
#define JSONRPC_ID      "id"
#define JSONRPC_VERSION "2.0"

#define JSONRPC_ERR_CODE "code"
#define JSONRPC_ERR_MSG  "message"
#define JSONRPC_ERR_MSG_LEN 1024

#define JSONRPC_OK 0

/* JSON-RPC main errors: -1...-9 */
#define JSONRPC_OBJ_ERR         -1
#define JSONRPC_NOPROTO         -2
#define JSONRPC_ERR_INTER       -3
#define JSONRPC_ERR_SERVER      -4
#define JSONRPC_ERR_SERV_DEF    -5
#define JSONRPC_TRANS_OBJ_NUL   -6
#define JSONRPC_TRANS_NOEXT     -7

/* JSON-RPC validation: -10 ... -19 */
#define JSONRPC_VALID_ERR_VER   -10
#define JSONRPC_VALID_ERR_MET   -11
#define JSONRPC_VALID_INVALID   -12
#define JSONRPC_VALID_ERR_ID    -13

/* JSON-RPC params errors: -20 ... -29 */
#define JSONRPC_PARAMS_OBJ_NUL  -20
#define JSONRPC_PARAMS_INVALID  -21

/* JSON-RPC result errors: -30 ... -39 */
#define JSONRPC_RESULT_OBJ_NUL  -30

#define JSONRPC_TYPE_REQUEST  1
#define JSONRPC_TYPE_RESPONSE 2
#define JSONRPC_TYPE_ERROR    3

#define JSONRPC_MSG_TYPE_RECV 1
#define JSONRPC_MSG_TYPE_SEND 2

#define JSONRPC_MODE_SYNC   1
#define JSONRPC_MODE_ASYNC  2

extern json_ext_obj_t json_rpc_err_msg[3];

typedef struct jsonrpc_errors {
	/* internal error for 'json_rpc' */
	int  error;
	
	/* external error for JSONRPC specification */
	int  code;
	 
	char err_msg[JSONRPC_ERR_MSG_LEN];
} jsonrpc_errors_t;

typedef enum jsonrpc_type {
	request  = JSONRPC_TYPE_REQUEST,
	response = JSONRPC_TYPE_RESPONSE,
	error    = JSONRPC_TYPE_ERROR
} jsonrpc_type_t;

typedef struct jsonrpc_proto_type {
	/* JSONRPC Request METHOD */
	char *method;
	
	/* JSONRPC Request PARAMS link (pointer to prototype) */
	json_ext_obj_t *req_proto_type;
	
	/* JSONRPC Response RESULT link (pointer to prototype) */
	json_ext_obj_t *resp_proto_type;
} jsonrpc_proto_type_t;

typedef enum jsonrpc_msg_type {
	jsonrpc_recv = JSONRPC_MSG_TYPE_RECV,
	jsonrpc_send = JSONRPC_MSG_TYPE_SEND
} jsonrpc_msg_type_t;

typedef struct jsonrpc {
	jsonrpc_type_t t;
	
	json_object *jobj;
	
	json_ext_obj_t *obj;
	
	char *msg;
} jsonrpc_t;

typedef struct jsonrpc_transaction {
	jsonrpc_msg_type_t msg_t;
	
	unsigned int id;
	
	jsonrpc_proto_type_t *proto_type_ptr;
	
	jsonrpc_t *jrpc;
	
	struct jsonrpc_transaction *next;
} jsonrpc_transaction_t;

void json_rpc_init(void);
void json_rpc_free(void);
void json_rpc_mode(unsigned short mode);

#define JSON_RPC_MODE_SYNC  json_rpc_mode(JSONRPC_MODE_SYNC);
#define JSON_RPC_MODE_ASYNC json_rpc_mode(JSONRPC_MODE_ASYNC);

jsonrpc_t *json_rpc_jrpc_init(char *msg);
void json_rpc_jrpc_free(jsonrpc_t *jrpc);
unsigned int json_rpc_jrpc_get_id(jsonrpc_t *jrpc);
char *json_rpc_jrpc_get_method(jsonrpc_t *jrpc);

void json_rpc_proto_init(jsonrpc_t *jrpc,json_ext_obj_t *pobj);
char *json_rpc_proto_req_init(json_ext_obj_t *arr,unsigned int num_arr,char *method,unsigned int id);
char *json_rpc_proto_resp_init(json_ext_obj_t *arr,unsigned int num_arr,unsigned int id);

void json_rpc_put_error(char *msg);
char *json_rpc_proto_err_init(int error,unsigned int id);

jsonrpc_proto_type_t *json_rpc_proto_type_get(char *method,jsonrpc_proto_type_t *proto);
int json_rpc_proto_get(jsonrpc_t *jrpc,jsonrpc_proto_type_t *ptr);

//jsonrpc_transaction_t *json_rpc_transaction_init(jsonrpc_proto_type_t *proto_type,unsigned int id,int msg_t);
jsonrpc_transaction_t *json_rpc_transaction_init(jsonrpc_proto_type_t *proto_type,unsigned int id,jsonrpc_msg_type_t msg_t);
void json_rpc_transaction_free(jsonrpc_transaction_t *trans);
void json_rpc_transaction_new(jsonrpc_transaction_t *trans);
void json_rpc_transaction_delete(unsigned int id);
jsonrpc_transaction_t *json_rpc_transaction_find(unsigned int id,int msg_t);
void json_rpc_transaction_view(void);

#endif
