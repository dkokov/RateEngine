/*
 * JSONRPC-CC Protocol , version 1
 *
 * --> {"jsonrpc":"2.0","method":"maxsec","params":{"cdr_server_id":1,"call-uid":"1111","clg":"359112","cld":"359111"},"id":3}
 * <-- {"jsonrpc":"2.0","result":{"maxsec":3600},"id":3}
 *
 * --> {"jsonrpc":"2.0","method":"balance","params":{"clg":"359112"},"id":31}
 * <-- {"jsonrpc":"2.0","result":{"amount":2.41},"id":31}
 *
 * --> {"jsonrpc":"2.0","method":"rate","params":{"clg":"359112","cld":"359880001"},"id":611}
 * <-- {"jsonrpc":"2.0","result":{"amount":0.023},"id":611}
 *
 * --> {"jsonrpc":"2.0","method":"cprice","params":{"clg":"359112","cld":"359880001","billsec":120},"id":61}
 * <-- {"jsonrpc":"2.0","result":{"amount":0.01},"id":61}
 *
 * --> {"jsonrpc":"2.0","method":"term","params":{"call-uid":"1111","status":"nc","billsec":120,"duration":125},"id":9}
 * <-- {"jsonrpc":"2.0","result":{"status":"ok"},"id":9}
 *
 * --> {"jsonrpc":"2.0","method":"state","params":{"cdr_server_id":1},"id":62}
 * <-- {"jsonrpc":"2.0","result":{"status":"idle"},"id":62}
 *
 * <-- {"jsonrpc":"2.0","error":{"code":-32600,"message":"Invalid Request"},"id":0}
 *
 * This module is a CallControl transport peer to my_cc: it parses a JSON-RPC
 * request, drives the same cc_event_manager() (per-worker DB context passed by
 * the parallel server via 'ctx'), then serialises the cc_t result back as a
 * JSON-RPC response. The lifecycle of cc_ptr matches my_cc exactly (a
 * successful maxsec keeps its cc_t in cc_tbl until 'term').
 */

#include <time.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "../Rating/rating.h"
#include "../CallControl/cc.h"
#include "../CallControl/cc_bind_api.h"

#include "jsonrpc_cc.h"

/* bound CallControl API (cc.so) */
static cc_funcs_t jcc_cc_api;

/* ---- JSON-RPC method prototypes (params + result) ---- */

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
	{json_ushrt,JSONRPC_CC_CDR_SERV_ID},
	{json_str,JSONRPC_CC_CLG_STR},
	{0,""}
};
static json_ext_obj_t balance_result[] = {
	{json_double,JSONRPC_CC_AMOUNT_STR},
	{0,""}
};

static json_ext_obj_t rate_params[] = {
	{json_ushrt,JSONRPC_CC_CDR_SERV_ID},
	{json_str,JSONRPC_CC_CLG_STR},
	{json_str,JSONRPC_CC_CLD_STR},
	{0,""}
};
static json_ext_obj_t rate_result[] = {
	{json_double,JSONRPC_CC_AMOUNT_STR},
	{0,""}
};

static json_ext_obj_t cprice_params[] = {
	{json_ushrt,JSONRPC_CC_CDR_SERV_ID},
	{json_str,JSONRPC_CC_CLG_STR},
	{json_str,JSONRPC_CC_CLD_STR},
	{json_int,JSONRPC_CC_BILLSEC_STR},
	{0,""}
};
static json_ext_obj_t cprice_result[] = {
	{json_double,JSONRPC_CC_AMOUNT_STR},
	{0,""}
};

static json_ext_obj_t term_params[] = {
	{json_ushrt,JSONRPC_CC_CDR_SERV_ID},
	{json_str,JSONRPC_CC_CALL_UID_STR},
	{json_str,JSONRPC_CC_SRV_STATUS},
	{json_int,JSONRPC_CC_BILLSEC_STR},
	{json_int,JSONRPC_CC_DURATION_STR},
	{0,""}
};
static json_ext_obj_t term_result[] = {
	{json_str,JSONRPC_CC_SRV_STATUS},
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

/* method table (terminated by an all-NULL row: json_rpc_proto_get() loops
 * while req_proto_type != NULL, proto_type_get() while method != NULL) */
static jsonrpc_proto_type_t cc_proto_type[] = {
	{JSONRPC_CC_MAXSEC_STR ,maxsec_params ,maxsec_result},
	{JSONRPC_CC_BALANCE_STR,balance_params,balance_result},
	{JSONRPC_CC_RATE_STR   ,rate_params   ,rate_result},
	{JSONRPC_CC_CPRICE_STR ,cprice_params ,cprice_result},
	{JSONRPC_CC_TERN_STR   ,term_params   ,term_result},
	{JSONRPC_CC_SRV_STATE  ,state_params  ,state_result},
	{NULL,NULL,NULL}
};

/* ---- param accessors (read from the parsed params sub-object) ---- */

static char *jcc_pstr(json_ext_obj_t *p,char *name)
{
	int q;

	if(p == NULL) return NULL;
	q = json_ext_find_obj(p,json_str,name);
	if(q < 0) return NULL;

	return p[q].u.str;
}

static unsigned short jcc_pushrt(json_ext_obj_t *p,char *name)
{
	int q;

	if(p == NULL) return 0;
	q = json_ext_find_obj(p,json_ushrt,name);
	if(q < 0 || p[q].u.us == NULL) return 0;

	return *p[q].u.us;
}

static int jcc_pint(json_ext_obj_t *p,char *name)
{
	int q;

	if(p == NULL) return 0;
	q = json_ext_find_obj(p,json_int,name);
	if(q < 0 || p[q].u.num == NULL) return 0;

	return *p[q].u.num;
}

/* ---- request -> cc_t ---- */

static cc_t *jsonrpc_cc_build_cc(char *method,json_ext_obj_t *params)
{
	char *s;
	cc_t *cc_ptr;

	if(method == NULL) return NULL;

	cc_ptr = jcc_cc_api.cc_alloc();
	if(cc_ptr == NULL) return NULL;

	cc_ptr->t = unkn_event;
	cc_ptr->cdr_server_id = jcc_pushrt(params,JSONRPC_CC_CDR_SERV_ID);

	if(strcmp(method,JSONRPC_CC_MAXSEC_STR) == 0) {
		cc_ptr->max = (cc_maxsec_t *)mem_alloc(sizeof(cc_maxsec_t));
		if(cc_ptr->max == NULL) goto err;

		cc_ptr->t = maxsec_event;

		if((s = jcc_pstr(params,JSONRPC_CC_CLG_STR)) != NULL) strncpy(cc_ptr->max->clg,s,CC_CLG_LEN-1);
		if((s = jcc_pstr(params,JSONRPC_CC_CLD_STR)) != NULL) strncpy(cc_ptr->max->cld,s,CC_CLD_LEN-1);
		if((s = jcc_pstr(params,JSONRPC_CC_CALL_UID_STR)) != NULL) strncpy(cc_ptr->max->call_uid,s,CC_CALL_UID-1);

		cc_ptr->max->ts = time(NULL);

		return cc_ptr;
	}

	if(strcmp(method,JSONRPC_CC_BALANCE_STR) == 0) {
		cc_ptr->bal = (cc_balance_t *)mem_alloc(sizeof(cc_balance_t));
		if(cc_ptr->bal == NULL) goto err;

		cc_ptr->t = balance_event;

		if((s = jcc_pstr(params,JSONRPC_CC_CLG_STR)) != NULL) strncpy(cc_ptr->bal->clg,s,CC_CLG_LEN-1);

		return cc_ptr;
	}

	if(strcmp(method,JSONRPC_CC_RATE_STR) == 0) {
		cc_ptr->rate = (cc_rate_t *)mem_alloc(sizeof(cc_rate_t));
		if(cc_ptr->rate == NULL) goto err;

		cc_ptr->t = rate_event;

		if((s = jcc_pstr(params,JSONRPC_CC_CLG_STR)) != NULL) strncpy(cc_ptr->rate->clg,s,CC_CLG_LEN-1);
		if((s = jcc_pstr(params,JSONRPC_CC_CLD_STR)) != NULL) strncpy(cc_ptr->rate->cld,s,CC_CLD_LEN-1);

		return cc_ptr;
	}

	if(strcmp(method,JSONRPC_CC_CPRICE_STR) == 0) {
		cc_ptr->cprice = (cc_cprice_t *)mem_alloc(sizeof(cc_cprice_t));
		if(cc_ptr->cprice == NULL) goto err;

		cc_ptr->t = cprice_event;

		if((s = jcc_pstr(params,JSONRPC_CC_CLG_STR)) != NULL) strncpy(cc_ptr->cprice->clg,s,CC_CLG_LEN-1);
		if((s = jcc_pstr(params,JSONRPC_CC_CLD_STR)) != NULL) strncpy(cc_ptr->cprice->cld,s,CC_CLD_LEN-1);
		cc_ptr->cprice->billsec = jcc_pint(params,JSONRPC_CC_BILLSEC_STR);

		return cc_ptr;
	}

	if(strcmp(method,JSONRPC_CC_TERN_STR) == 0) {
		cc_ptr->term = (cc_term_t *)mem_alloc(sizeof(cc_term_t));
		if(cc_ptr->term == NULL) goto err;

		cc_ptr->t = term_event;

		s = jcc_pstr(params,JSONRPC_CC_SRV_STATUS);
		if(s != NULL) {
			if(strcmp(s,JSONRPC_CC_TERM_NC) == 0)      cc_ptr->term->status = normal_clear;
			else if(strcmp(s,JSONRPC_CC_TERM_C) == 0)  cc_ptr->term->status = cancel;
			else if(strcmp(s,JSONRPC_CC_TERM_B) == 0)  cc_ptr->term->status = busy;
			else if(strcmp(s,JSONRPC_CC_TERM_E) == 0)  cc_ptr->term->status = error;
			else cc_ptr->term->status = unkn_status;
		} else cc_ptr->term->status = unkn_status;

		if((s = jcc_pstr(params,JSONRPC_CC_CALL_UID_STR)) != NULL) strncpy(cc_ptr->term->call_uid,s,CC_CALL_UID-1);

		cc_ptr->term->billsec  = jcc_pint(params,JSONRPC_CC_BILLSEC_STR);
		cc_ptr->term->duration = jcc_pint(params,JSONRPC_CC_DURATION_STR);

		return cc_ptr;
	}

	if(strcmp(method,JSONRPC_CC_SRV_STATE) == 0) {
		cc_ptr->t = status_event;
		return cc_ptr;
	}

	err:
	mem_free(cc_ptr);
	return NULL;
}

/* ---- cc_t result -> JSON-RPC response ---- */

static char *jsonrpc_cc_comp_resp(cc_t *cc_ptr,unsigned int id)
{
	int mx;
	double amount;
	char *status;
	json_ext_obj_t *res;

	switch(cc_ptr->t) {
		case maxsec_event:
			if(cc_ptr->max != NULL) {
				mx = cc_ptr->max->maxsec;
				/* failure (maxsec <= 0): the cc_t is done; success keeps it in cc_tbl */
				if(cc_ptr->max->maxsec <= 0) cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			} else {
				mx = JSONRPC_CC_ERR_MAX_NUL;
				cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			}

			res = json_ext_new_obj(maxsec_result);
			json_ext_init_value(res,JSONRPC_CC_MAXSEC_STR,(void *)&mx,0);
			return json_rpc_proto_resp_init(res,0,id);

		case balance_event:
			amount = (cc_ptr->bal != NULL) ? cc_ptr->bal->amount : 0;
			cc_ptr->cc_status = CC_STATUS_DEACTIVE;

			res = json_ext_new_obj(balance_result);
			json_ext_init_value(res,JSONRPC_CC_AMOUNT_STR,(void *)&amount,0);
			return json_rpc_proto_resp_init(res,0,id);

		case rate_event:
			amount = (cc_ptr->rate != NULL) ? cc_ptr->rate->rate_price : 0;
			cc_ptr->cc_status = CC_STATUS_DEACTIVE;

			res = json_ext_new_obj(rate_result);
			json_ext_init_value(res,JSONRPC_CC_AMOUNT_STR,(void *)&amount,0);
			return json_rpc_proto_resp_init(res,0,id);

		case cprice_event:
			amount = (cc_ptr->cprice != NULL) ? cc_ptr->cprice->cprice : 0;
			cc_ptr->cc_status = CC_STATUS_DEACTIVE;

			res = json_ext_new_obj(cprice_result);
			json_ext_init_value(res,JSONRPC_CC_AMOUNT_STR,(void *)&amount,0);
			return json_rpc_proto_resp_init(res,0,id);

		case term_event:
			if((cc_ptr->term != NULL) && (cc_ptr->term->tbl_idx >= 0)) {
				status = JSONRPC_CC_OK;
			} else {
				status = JSONRPC_CC_NOK;
				cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			}

			res = json_ext_new_obj(term_result);
			json_ext_init_value(res,JSONRPC_CC_SRV_STATUS,(void *)status,0);
			return json_rpc_proto_resp_init(res,0,id);

		case status_event:
			cc_ptr->cc_status = CC_STATUS_DEACTIVE;

			res = json_ext_new_obj(state_result);
			json_ext_init_value(res,JSONRPC_CC_SRV_STATUS,(void *)JSONRPC_CC_IDLE,0);
			return json_rpc_proto_resp_init(res,0,id);

		default:
			cc_ptr->cc_status = CC_STATUS_DEACTIVE;
			return json_rpc_proto_err_init(JSONRPC_CC_ERR_EVENT_UNKN,id);
	}
}

int jsonrpc_cc_mod_init(void)
{
	int ret;
	void *func;
	mod_t *mod_ptr;
	int (*fptr)(cc_funcs_t *);

	memset(&jcc_cc_api,0,sizeof(cc_funcs_t));

	mod_ptr = mod_find_module("cc.so");

	if(mod_ptr == NULL) return RE_ERROR_N;
	if(mod_ptr->handle == NULL) return RE_ERROR_N;

	func = mod_find_sim(mod_ptr->handle,"cc_bind_api");
	if(func != NULL) {
		fptr = func;

		ret = fptr(&jcc_cc_api);
		if(ret < 0) {
			LOG("jsonrpc_cc_mod_init()","cc_bind_api(),ret: %d",ret);
			return RE_ERROR_N;
		}
	} else return RE_ERROR_N;

	/* SYNC request/response: no transaction list is used, so json_rpc_proto_get()
	 * stays thread-safe across the CallControl worker pool */
	json_rpc_mode(JSONRPC_MODE_SYNC);

	return RE_SUCCESS;
}

/* net_handler_f: buf holds the request on entry and the reply on return; ctx is
 * the calling worker's db_t* (from the parallel server's per-worker init) */
int jsonrpc_cc_proto_bind(char *buf,void *ctx)
{
	int ret,p;
	unsigned int id;
	char *method,*resp;

	cc_t *cc_ptr;
	jsonrpc_t *jrpc;
	json_ext_obj_t *params;

	/* bind cc.so on first use (mod init is not auto-invoked) */
	if(jcc_cc_api.cc_alloc == NULL) {
		if(jsonrpc_cc_mod_init() < 0) {
			resp = json_rpc_proto_err_init(JSONRPC_ERR_INTER,0);
			goto reply;
		}
	}

	resp = NULL;

	jrpc = json_rpc_jrpc_init(buf);
	if(jrpc == NULL) {
		resp = json_rpc_proto_err_init(JSONRPC_ERR_INTER,0);
		goto reply;
	}

	ret = json_rpc_proto_get(jrpc,cc_proto_type);

	if(ret != JSONRPC_OK || jrpc->t != JSONRPC_TYPE_REQUEST) {
		id = json_rpc_jrpc_get_id(jrpc);
		resp = json_rpc_proto_err_init((ret < 0) ? ret : JSONRPC_VALID_INVALID,id);
		json_rpc_jrpc_free(jrpc);
		goto reply;
	}

	method = json_rpc_jrpc_get_method(jrpc);
	id     = json_rpc_jrpc_get_id(jrpc);

	p = json_ext_find_obj(jrpc->obj,json_obj,JSONRPC_PARAMS);
	params = (p >= 0) ? jrpc->obj[p].u.jobj : NULL;

	cc_ptr = jsonrpc_cc_build_cc(method,params);

	if(cc_ptr == NULL) {
		resp = json_rpc_proto_err_init(JSONRPC_PARAMS_INVALID,id);
	} else {
		jcc_cc_api.cc_event_m(cc_ptr,(db_t *)ctx);

		resp = jsonrpc_cc_comp_resp(cc_ptr,id);

		/* same lifecycle as my_cc: a live (ACTIVE) maxsec stays in cc_tbl,
		 * a finished term_event frees only the cc_t wrapper */
		if(cc_ptr->cc_status == CC_STATUS_DEACTIVE) {
			jcc_cc_api.cc_free(cc_ptr);
		} else if(cc_ptr->t == term_event) {
			mem_free(cc_ptr);
		}
	}

	json_rpc_jrpc_free(jrpc);

	reply:
	memset(buf,0,strlen(buf));

	if(resp != NULL) {
		strcpy(buf,resp);
		free(resp);
	} else strcpy(buf,"{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32603,\"message\":\"Internal error\"},\"id\":0}");

	return RE_SUCCESS;
}
