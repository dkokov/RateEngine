/* 
 * JSON-RPC 2.0
 * 
 * https://www.jsonrpc.org/specification
 * 
 * Specification example cases:
 * 
 *  1) rpc call with positional parameters
 *  2) rpc call with named parameters
 *  3) a Notification { 3.1) with positional parameters; 3.2) without parameters }
 *  4) rpc call of non-existent method
 *  5) rpc call with invalid JSON
 *  6) rpc call with invalid Request object
 *  7) rpc call Batch, invalid JSON
 *  8) rpc call with an empty Array
 *  9) rpc call with an invalid Batch (but not empty)
 * 10) rpc call with invalid Batch
 * 11) rpc call Batch
 * 12) rpc call Batch (all notifications)
 * 
 * http://xmlrpc-epi.sourceforge.net/specs/rfc.fault_codes.php
 *  
 * ************************************************************************************************
 * 
 * 'json_rpc' realese:
 * 
 *  1) not support / not support 'params' as array
 *  2) support
 *  3) 3.1) not support , 3.2) support
 *  4) decoded without 'params',only 'version','method','id' ?
 *  5) return error message 'Parse error'
 *  6) return error message 'Invalid Request'
 *  7) return error message 'Parse error'
 *  8) return error message 'Parse error'
 *  9) return error message 'Parse error'
 * 10) return error message 'Parse error'
 * 11) not support
 * 12) not support
 * 
 * "params": {}, mandatory, 2)
 * "result": {}, mandatory / not support 'result' as array(positional vars) or only value ("result" :3)
 * "id": 0, instead "id": "null"
 * 
 * */

#include "../../misc/globals.h"
#include "../../proc/proc_thread.h"
#include "../../mem/mem.h"

#include "json_rpc.h"

static unsigned short jsonrpc_mode;

/* JSONRPC global counter for ID locking */
pthread_mutex_t jsonrpc_mutex;

/* JSONRPC global counter for ID */
static unsigned int jsonrpc_transaction_id;

/* JSONRPC transactions list  */
static jsonrpc_transaction_t *jsonrpc_transaction_lst;

static json_ext_obj_t jsonrpc_proto_req[] = {
	{json_str ,JSONRPC_STR},
	{json_str ,JSONRPC_METHOD},
	{json_obj ,JSONRPC_PARAMS},
	{json_uint,JSONRPC_ID},
	{0,""}
};

static json_ext_obj_t jsonrpc_proto_resp[] = {
	{json_str ,JSONRPC_STR},
	{json_obj ,JSONRPC_RESULT},
	{json_uint,JSONRPC_ID},
	{0,""}
};

static json_ext_obj_t jsonrpc_proto_err[] = {
	{json_str ,JSONRPC_STR},
	{json_obj ,JSONRPC_ERROR},
	{json_uint,JSONRPC_ID},
	{0,""}
};

json_ext_obj_t jsonrpc_err_msg[3] = {
	{json_int,JSONRPC_ERR_CODE},
	{json_str,JSONRPC_ERR_MSG},
	{0,""}
};

static jsonrpc_errors_t jsonrpc_errors[] = {
	{JSONRPC_NOPROTO,-32700,"Parse error"},             /* Invalid JSON was received by the server.
                                                           An error occurred on the server while parsing the JSON text.*/
	{JSONRPC_OBJ_ERR,-32700,"Parse error"},
	{JSONRPC_PARAMS_OBJ_NUL,-32600,"Invalid Request"},  /* The JSON sent is not a valid Request object. */
	{JSONRPC_VALID_ERR_ID  ,-32600,"Invalid Request"},
	{JSONRPC_VALID_ERR_VER ,-32600,"Invalid Request"},
	{JSONRPC_VALID_ERR_MET ,-32601,"Method not found"}, /* The method does not exist / is not available. */
	{JSONRPC_VALID_INVALID ,-32602,"Invalid params"},   /* Invalid method parameter(s). */
	{JSONRPC_PARAMS_INVALID,-32602,"Invalid params"},
	{JSONRPC_ERR_INTER ,-32603,"Internal error"},       /* Internal JSON-RPC error. */
	{JSONRPC_ERR_SERVER,-32000,"Server error"},         /* -32000 to -32099 ; Reserved for implementation-defined server-errors. */
	{JSONRPC_TRANS_OBJ_NUL,-32001,"Transaction object is NULL"},
	{JSONRPC_TRANS_NOEXT,-32002,"No exist transaction"},
	{JSONRPC_ERR_SERV_DEF,-32099,""},
	{0,0,""}
};

int json_rpc_get_error(int error)
{
	int i;
	
	if(error < 0) {
		i = 0;
		while(jsonrpc_errors[i].code != 0) {
			if(error == jsonrpc_errors[i].error) return i; 
			
			i++;
		}
	}
	
	return 0;
}

void json_rpc_put_error(char *msg)
{
	int i;
	
	i = json_rpc_get_error(JSONRPC_ERR_SERV_DEF);
	
	strcpy(jsonrpc_errors[i].err_msg,msg);
}

jsonrpc_t *json_rpc_jrpc_init(char *msg)
{
	jsonrpc_t *jrpc;
	
	if(msg == NULL) return NULL;
	
	jrpc = mem_alloc(sizeof(jsonrpc_t));
	if(jrpc != NULL) jrpc->msg = msg;

	return jrpc;
}

void json_rpc_jrpc_free(jsonrpc_t *jrpc)
{
	if(jrpc != NULL) {
		if(jrpc->obj != NULL) json_ext_delete_obj(jrpc->obj,JSON_EXT_GET_MODE_FREE);
		if(jrpc->jobj != NULL) json_object_put(jrpc->jobj);

		mem_free(jrpc);
	}
}

unsigned int json_rpc_jrpc_get_id(jsonrpc_t *jrpc)
{
	int p;
	unsigned int id;
	
	if(jrpc == NULL) return 0;
	
	id = 0;
	if(jrpc->obj != NULL) {
		p = json_ext_find_obj(jrpc->obj,json_uint,JSONRPC_ID);
		if(p >= 0) {
			if(jrpc->obj[p].u.ui != NULL) id = *jrpc->obj[p].u.ui;
		}
	}
	
	return id;
}

char *json_rpc_jrpc_get_method(jsonrpc_t *jrpc)
{
	int p;
	char *method;
	
	if(jrpc == NULL) return NULL;
	
	method = NULL;
	if(jrpc->obj != NULL) {
		p = json_ext_find_obj(jrpc->obj,json_str,JSONRPC_METHOD);
		if(p >= 0) {
			if(jrpc->obj[p].u.str != NULL) method = jrpc->obj[p].u.str;
		}
	}
	
	return method;
}

int json_rpc_jrpc_cpy_proto_type()
{
	
}

void json_rpc_proto_init(jsonrpc_t *jrpc,json_ext_obj_t *pobj)
{
	json_ext_obj_t *new;
	
	switch(jrpc->t) {
		case JSONRPC_TYPE_REQUEST:
			jrpc->obj = json_ext_new_obj(jsonrpc_proto_req);
			
			new = json_ext_new_obj(pobj);
			json_ext_init_value(jrpc->obj,JSONRPC_PARAMS,(void *)new,0);			
			break;
		case JSONRPC_TYPE_RESPONSE:
			jrpc->obj = json_ext_new_obj(jsonrpc_proto_resp);
			
			new = json_ext_new_obj(pobj);
			json_ext_init_value(jrpc->obj,JSONRPC_RESULT,(void *)new,0);
			break;
		case JSONRPC_TYPE_ERROR:
			jrpc->obj = json_ext_new_obj(jsonrpc_proto_err);
			
			new = json_ext_new_obj(pobj);
			json_ext_init_value(jrpc->obj,JSONRPC_ERROR,(void *)new,0);		
			break;
		default:
			break;
	};	
}

char *json_rpc_proto_req_init(json_ext_obj_t *obj,unsigned int num_arr,char *method,unsigned int id)
{
	char *msg;
	
	json_ext_obj_t *new;
	
	new = json_ext_new_obj(jsonrpc_proto_req);
	
	json_ext_init_value(new,JSONRPC_STR,JSONRPC_VERSION,0);
	json_ext_init_value(new,JSONRPC_METHOD,method,0);
	json_ext_init_value(new,JSONRPC_PARAMS,(void *)obj,num_arr);
	json_ext_init_value(new,JSONRPC_ID,(void *)&id,0);

	msg = json_ext_obj_create(new);
	
	json_ext_delete_obj(new,JSON_EXT_PUT_MODE_FREE);
	json_ext_delete_obj(obj,JSON_EXT_PUT_MODE_FREE);
	
	return msg;
}

char *json_rpc_proto_resp_init(json_ext_obj_t *arr,unsigned int num_arr,unsigned int id)
{
	char *msg;
	
	json_ext_obj_t *new;
	
	new = json_ext_new_obj(jsonrpc_proto_resp);
	
	json_ext_init_value(new,JSONRPC_STR,JSONRPC_VERSION,0);
	json_ext_init_value(new,JSONRPC_RESULT,(void *)arr,num_arr);
	json_ext_init_value(new,JSONRPC_ID,(void *)&id,0);
	
	msg = json_ext_obj_create(new);
	
	json_ext_delete_obj(new,JSON_EXT_PUT_MODE_FREE);
	json_ext_delete_obj(arr,JSON_EXT_PUT_MODE_FREE);
	
	return msg;
}

char *json_rpc_proto_err_init(int error,unsigned int id)
{
	int i,code;
	char *msg;
	char *errmsg;
	
	json_ext_obj_t *new,*err_msg;
	
	i = json_rpc_get_error(error);
	code   = jsonrpc_errors[i].code;
	errmsg = jsonrpc_errors[i].err_msg;
	
	new = json_ext_new_obj(jsonrpc_proto_err);
	err_msg = json_ext_new_obj(jsonrpc_err_msg);
	
	json_ext_init_value(err_msg,JSONRPC_ERR_CODE,(void *)&code,0);
	json_ext_init_value(err_msg,JSONRPC_ERR_MSG,(void *)errmsg,0);
	
	json_ext_init_value(new,JSONRPC_STR,JSONRPC_VERSION,0);
	json_ext_init_value(new,JSONRPC_ERROR,(void *)err_msg,1);
	json_ext_init_value(new,JSONRPC_ID,(void *)&id,0);
	
	msg = json_ext_obj_create(new);
	
	json_ext_delete_obj(new,JSON_EXT_PUT_MODE_FREE);
	json_ext_delete_obj(err_msg,JSON_EXT_PUT_MODE_FREE);
	
	return msg;
}

jsonrpc_proto_type_t *json_rpc_proto_type_get(char *method,jsonrpc_proto_type_t *proto)
{
	int i;
	
	i = 0;
	while(proto[i].method != NULL) {
		if(strcmp(method,proto[i].method) == 0) return &proto[i];
		
		i++;
	}
	
	return NULL;
}

int json_rpc_proto_get(jsonrpc_t *jrpc,jsonrpc_proto_type_t *ptr)
{	
	int i,c,p;
	unsigned int id;
	
	json_object *tmp;
	jsonrpc_transaction_t *trans;
	
	jrpc->jobj = json_tokener_parse(jrpc->msg);
	
	tmp = NULL;
	json_object_object_get_ex(jrpc->jobj, JSONRPC_STR, &tmp);
	
	if(tmp == NULL) return JSONRPC_OBJ_ERR;
	
	/* Version validation */
	if(strcmp(json_object_get_string(tmp),JSONRPC_VERSION)) {
		return JSONRPC_VALID_ERR_VER;
	}
	
	/* Validation method or result/error */
	tmp = NULL;
	json_object_object_get_ex(jrpc->jobj,JSONRPC_METHOD, &tmp);

	if(tmp != NULL) {
		/* JSONRPC Request */
		jrpc->t = JSONRPC_TYPE_REQUEST;
		jrpc->obj = json_ext_new_obj(jsonrpc_proto_req);
		
		if((jrpc->jobj != NULL)&&(jrpc->obj != NULL)) {
			json_ext_get_obj(jrpc->jobj,jrpc->obj);
		}
		
		c = json_ext_find_obj(jrpc->obj,json_str,JSONRPC_METHOD);
		if(c >= 0) {
			i = 0;
			while(ptr[i].req_proto_type != NULL) {
				if(strcmp(jrpc->obj[c].u.str,ptr[i].method) == 0) {
					p = json_ext_find_obj(jrpc->obj,json_obj,JSONRPC_PARAMS);
					if(p > 0) {
						jrpc->obj[p].t = json_obj;
						jrpc->obj[p].u.jobj = json_ext_new_obj(ptr[i].req_proto_type);

						json_ext_get_obj(jrpc->jobj,jrpc->obj);
					}
					
					break;
				}
				
				i++;
			}
			
			if(p < 0) return JSONRPC_VALID_ERR_MET; // ima metod no ne e ot poso4enite v 'proto_type' !!!
		} else return JSONRPC_VALID_ERR_MET; // ne e nameren v obj !!!
					
		c = json_ext_find_obj(jrpc->obj,json_uint,JSONRPC_ID);
		if(c < 0) return JSONRPC_VALID_ERR_ID;
		else {
			if(jrpc->obj[c].u.ui != NULL) id = *jrpc->obj[c].u.ui;
			else return JSONRPC_VALID_ERR_ID;
		}
		
		if(jsonrpc_mode == JSONRPC_MODE_ASYNC) {
			trans = json_rpc_transaction_init(&ptr[i],id,jsonrpc_recv);
			if(trans != NULL) {
				trans->jrpc = jrpc;
				json_rpc_transaction_new(trans);
			} else return JSONRPC_TRANS_OBJ_NUL;
		}
		
		return JSONRPC_OK;
	} else {
		/* JSONRPC Response */
		tmp = NULL;
		json_object_object_get_ex(jrpc->jobj,JSONRPC_RESULT, &tmp);
	
		if(tmp != NULL) {
			/* result */		
			jrpc->t = JSONRPC_TYPE_RESPONSE;
			jrpc->obj = json_ext_new_obj(jsonrpc_proto_resp);

			if((jrpc->jobj != NULL)&&(jrpc->obj != NULL)) {
				json_ext_get_obj(jrpc->jobj,jrpc->obj);
			}

			if(trans != NULL) {
				c = json_ext_find_obj(jrpc->obj,json_uint,JSONRPC_ID);
				if(c > 0) {
					while(trans) {
						if(trans->id == *jrpc->obj[c].u.num) {
							p = json_ext_find_obj(jrpc->obj,json_obj,JSONRPC_RESULT);
							if(p > 0) {
								jrpc->obj[p].t = json_obj;
								jrpc->obj[p].u.jobj = json_ext_new_obj(trans->proto_type_ptr->resp_proto_type);

								json_ext_get_obj(jrpc->jobj,jrpc->obj);
							
								break;
							}
						}
						
						trans = trans->next;
					}					
				} else return JSONRPC_RESULT_OBJ_NUL;
			} else return JSONRPC_TRANS_OBJ_NUL;

			return JSONRPC_OK;
		} else {
			json_object_object_get_ex(jrpc->jobj,JSONRPC_ERROR, &tmp);
			
			if(tmp != NULL) {
				/* error */
				jrpc->t = JSONRPC_TYPE_ERROR;
				
				return JSONRPC_OK;
			} else {
				/* Invalid */
				return JSONRPC_VALID_INVALID;
			}
		}
	}
	
	return JSONRPC_NOPROTO;
}
/*
int json_rpc_proto_type_validator(jsonrpc_t *jrpc,jsonrpc_proto_type_t *proto_type)
{	
	int i,c;
	
	if(jrpc == NULL) return JSONRPC_OBJ_ERR;
	if(proto_type == NULL) return JSONRPC_OBJ_ERR;
	
	if(jrpc->t == JSONRPC_TYPE_REQUEST) {
		i = 0;
		while(proto_type[i].req_proto_type != NULL) {
			c = json_ext_find_obj(jrpc->obj,json_str,JSONRPC_METHOD);
			if(c > 0) {
				if(strcmp(jrpc->obj[c].u.str,proto_type[i].method) == 0) return JSONRPC_OK;
			}
		
			i++;
		}
		
		return JSONRPC_VALID_ERR_MET;
	}
				
	return JSONRPC_ERR_INTER;
}*/

unsigned int json_rpc_gen_id(void)
{
	pthread_mutex_lock(&jsonrpc_mutex);
	jsonrpc_transaction_id++;
	pthread_mutex_unlock(&jsonrpc_mutex);

	return jsonrpc_transaction_id;
}

jsonrpc_transaction_t *json_rpc_transaction_init(jsonrpc_proto_type_t *proto_type,unsigned int id,jsonrpc_msg_type_t msg_t)
{
	jsonrpc_transaction_t *trans;
		
	if(proto_type == NULL) return NULL;
	if(msg_t <= 0) return NULL;
	
	trans = (jsonrpc_transaction_t *)mem_alloc(sizeof(jsonrpc_transaction_t));
	
	if(trans != NULL) {
		if(msg_t == jsonrpc_recv) trans->id = id;
		else trans->id = json_rpc_gen_id();
		
		trans->proto_type_ptr = proto_type;
		trans->msg_t = msg_t;
	}
	
	return trans;
}

void json_rpc_transaction_free(jsonrpc_transaction_t *trans)
{
	if(trans != NULL) mem_free(trans);
}

void json_rpc_transaction_view(void)
{
	jsonrpc_transaction_t *tmp = NULL;
	
	if(jsonrpc_transaction_lst != NULL) tmp = jsonrpc_transaction_lst;
	
	while(tmp) {
		if((tmp->id)&&(tmp->proto_type_ptr != NULL)) printf("\ntid: %d,method: %s,msg_t: %d\n",tmp->id,tmp->proto_type_ptr->method,tmp->msg_t);
		else printf("\ntmp is NULL!(json_rpc_transaction_view())\n");
		
		tmp = tmp->next;
	}
}

void json_rpc_transaction_new(jsonrpc_transaction_t *trans)
{
	jsonrpc_transaction_t *tmp;
	
	if(trans != NULL) {
		if(jsonrpc_transaction_lst == NULL) jsonrpc_transaction_lst = trans;
		else {
			tmp = jsonrpc_transaction_lst;
			while(tmp) {
				if(tmp->next == NULL) {
					tmp->next = trans;
					break;
				}
				
				tmp = tmp->next;
			}
		}
	}
}

void json_rpc_transaction_delete(unsigned int id)
{
	jsonrpc_transaction_t *tmp,*prev;

	if(id > 0) {
		tmp = jsonrpc_transaction_lst;
		while(tmp) {
			if(tmp->id == id) {
				prev->next = tmp->next;
				json_rpc_transaction_free(prev);
				
				break;
			}
			
			prev = tmp;
			tmp = tmp->next;
		}
	}
}

jsonrpc_transaction_t *json_rpc_transaction_find(unsigned int id,int msg_t)
{
	jsonrpc_transaction_t *tmp;
	
	if(id <= 0) return NULL;
	
	tmp = jsonrpc_transaction_lst;
	while(tmp) {
		if((tmp->id == id)&&(tmp->msg_t == msg_t)) break;
			
		tmp = tmp->next;
	}
	
	return tmp;
}

void json_rpc_init(void)
{
	jsonrpc_transaction_id = 0;
	jsonrpc_transaction_lst = NULL;
	jsonrpc_mode = JSONRPC_MODE_SYNC;
	
	pthread_mutex_init(&jsonrpc_mutex,NULL);
}

void json_rpc_free(void)
{
	jsonrpc_transaction_t *tmp;
	
	if(jsonrpc_transaction_lst != NULL) {
		while(jsonrpc_transaction_lst) {
			tmp = jsonrpc_transaction_lst;
			jsonrpc_transaction_lst = jsonrpc_transaction_lst->next;
			
			json_rpc_transaction_free(tmp);
		}
	}
	
	pthread_mutex_destroy(&jsonrpc_mutex);
}

void json_rpc_mode(unsigned short mode)
{
	jsonrpc_mode = mode;
}
