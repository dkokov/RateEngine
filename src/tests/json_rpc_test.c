/*
 * gcc -Wall -g -o json_rpc_test json_rpc_test.c  -ljson-c -L../ -lre7core
 * 
 * valgrind --leak-check=full --show-leak-kinds=all ./json_rpc_test req|resp|error
 * 
 * */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../misc/exten/json_rpc.h"

static json_ext_obj_t proto_params[] = {
	{json_str,"call-uid"},
	{json_str,"clg"},
	{json_str,"cld"},
	{0,""}
};

static json_ext_obj_t result[] = {
	{json_int,"maxsec"},
	{0,""}
};

static json_ext_obj_t test_params[] = {
	{json_str,"cdr_server_id"},
	{0,""}
};

static json_ext_obj_t test_result[] = {
	{json_str,"status"},
	{0,""}
};

static jsonrpc_proto_type_t proto_type[] = {
	{"maxsec",proto_params,result},
	{"test",test_params,test_result},
	{"",NULL,NULL}
};

void help(void)
{
	printf("Help:\n\t./json_rpc_test req \n\t./json_rpc_test resp\n\t./json_rpc_test error\n\n");	
}

int main(int c,char **argv)
{	
	int t;
	char *buf;
	
	jsonrpc_t *jrpc;
	json_ext_obj_t *new;
	jsonrpc_transaction_t *trans;
	

	if(c <= 1) {
		help();
		return -1;		
	}
		
	buf = NULL;
	trans = NULL;

	json_rpc_init();
	
	/* create 'JSON-RPC 2.0' messages : request,response,error */
	if(strcmp(argv[1],"error") == 0) {
		json_rpc_put_error("CallContol server is shutdown");
		buf = json_rpc_proto_err_init(JSONRPC_ERR_SERV_DEF,0);
	} else if(strcmp(argv[1],"resp") == 0) {		
		char status[] = "idle";
			
		trans = json_rpc_transaction_init(json_rpc_proto_type_get("test",proto_type),jsonrpc_send);
		
		if(trans == NULL) goto _end;
		
		json_rpc_new_transaction(trans);

		new = json_ext_new_obj(test_result);
		
		json_ext_init_value(new,"status",(void *)status,0);
		
		buf = json_rpc_proto_resp_init(new,0,trans->id);
	} else if(strcmp(argv[1],"req") == 0) {
		char cid[] = "1234";
		char clg[] = "35924119998";
		char cld[] = "0886893345";
			
		trans = json_rpc_transaction_init(json_rpc_proto_type_get("maxsec",proto_type),jsonrpc_send);
		
		if(trans == NULL) goto _end;
		
		json_rpc_new_transaction(trans);
		
		new = json_ext_new_obj(proto_params);

		json_ext_init_value(new,"call-uid",(void *)cid,0);
		json_ext_init_value(new,"clg",(void *)clg,0);
		json_ext_init_value(new,"cld",(void *)cld,0);
	
		buf = json_rpc_proto_req_init(new,0,"maxsec",trans->id);
	} else {
		help();
		goto _end;;
	}
		
	if(buf != NULL) {
		printf("\nbuf(before created message): %s\n\n",buf);
	
		/* recognize received 'JSON-RPC 2.0' messages: request,response,error */
		jrpc = json_rpc_jrpc_init();
	
		jrpc->msg = buf;
	
		t = json_rpc_proto_get(jrpc,proto_type,trans);
		if(t == JSONRPC_OK) {
			if(jrpc->obj != NULL) json_ext_print_obj(jrpc->obj,0);
		} else {
			printf("\nt: %d\n",t);
		}

		json_rpc_jrpc_free(jrpc);
	
		free(buf);
	}
	
_end:
	json_rpc_free();
	
	return 0;
}
