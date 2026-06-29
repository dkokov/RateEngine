/*
 * gcc -Wall -g -o json_rpc_test_2 json_rpc_test_2.c  -ljson-c -L../ -lre7core
 * 
 * valgrind --leak-check=full --show-leak-kinds=all ./json_rpc_test_2 send|recv
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

static jsonrpc_proto_type_t proto_type[] = {
	{"maxsec",proto_params,result},
	{"heartbeat",NULL,NULL},
	{"",NULL,NULL}
};

void help(void)
{
	printf("Help:\n\t./json_rpc_test_2 send \n\t./json_rpc_test recv\n\n");	
}

int main(int c,char **argv)
{	
	int t,p,id;
	char *buf;
	
	jsonrpc_t *jrpc;
	json_ext_obj_t *new;
	jsonrpc_transaction_t *trans;

	if(c <= 1) {
		help();
		return -1;		
	}
	
	id = 0;
	buf = NULL;
	trans = NULL;

	json_rpc_init();
	
	if(strcmp(argv[1],"recv") == 0) {
		char *recv_req;
		
		if(strlen(argv[2]) == 0) goto _end;
		else recv_req = argv[2];
		
		printf("\nrecv_req: %s\n\n",recv_req);
	
		jrpc = json_rpc_jrpc_init(recv_req);
	
		t = json_rpc_proto_get(jrpc,proto_type);
		if(t == JSONRPC_OK) {
			if(jrpc->obj != NULL) {
				json_ext_print_obj(jrpc->obj,0);
				
				printf("\nmethod: %s\n",json_rpc_jrpc_get_method(jrpc));
				/* app logic and return 'JSON-RPC 2.0' response or error message 

				id = json_rpc_jrpc_get_id(jrpc);
			
				trans = json_rpc_transaction_find(id,jsonrpc_recv);
				if(trans != NULL) printf("\ntransaction id: %d\n",trans->id);
				else printf("\nNo transaction\n");*/
			}
		} else {
			printf("\njson_rpc_proto_get() return error: %d\n",t);
			
			if(jrpc->obj != NULL) {
				p = json_ext_find_obj(jrpc->obj,json_uint,JSONRPC_ID);
				if(p < 0) goto _end;
			
				if(jrpc->obj[p].u.ui != NULL) id = *jrpc->obj[p].u.ui;
			}
			
			trans = json_rpc_transaction_find(id,jsonrpc_recv);
			if(trans != NULL) printf("\ntid: %d\n",trans->id);
			else printf("\nNo transaction\n");
			
			buf = json_rpc_proto_err_init(t,id);
			printf("\nsend/return error buf: %s\n",buf);
			free(buf);
		}

		json_rpc_jrpc_free(jrpc);
	} else if(strcmp(argv[1],"send") == 0) {
		char *method;

		if(strlen(argv[2]) > 0) method = argv[2];
		else goto _end;
		
		JSON_RPC_MODE_ASYNC;
		
		trans = json_rpc_transaction_init(json_rpc_proto_type_get(method,proto_type),0,jsonrpc_send);
		
		if(trans == NULL) {
			printf("\ntransaction init is unsuccessfull\n");
			goto _end;
		}
		
		json_rpc_transaction_new(trans);
		
		new = NULL;
		if(strcmp(method,"maxsec") == 0) {
			char cid[] = "1234";
			char clg[] = "35924119998";
			char cld[] = "0886893345";

			new = json_ext_new_obj(proto_params);

			json_ext_init_value(new,"call-uid",(void *)cid,0);
			json_ext_init_value(new,"clg",(void *)clg,0);
			json_ext_init_value(new,"cld",(void *)cld,0);
		}
		
		buf = json_rpc_proto_req_init(new,0,method,trans->id);
		printf("\nsend_req: %s\n\n",buf);
		free(buf);
		buf = NULL;
	} else {
		help();
		goto _end;;
	}
	
_end:
	json_rpc_transaction_view();
	json_rpc_free();
	
	return 0;
}
