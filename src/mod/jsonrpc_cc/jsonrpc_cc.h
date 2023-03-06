#ifndef JSONRPC_CC_H
#define JSONRPC_CC_H

#include "../../misc/exten/json_rpc.h"

#define JSONRPC_CC_MAXSEC_STR   "maxsec"
#define JSONRPC_CC_BALANCE_STR  "balance"
#define JSONRPC_CC_CPRICE_STR   "cprice"
#define JSONRPC_CC_RATE_STR     "rate"
#define JSONRPC_CC_TERN_STR     "term"
#define JSONRCP_CC_STATE_STR    "state"

#define JSONRPC_CC_CLG_STR      "clg"
#define JSONRPC_CC_CLD_STR      "cld"
#define JSONRPC_CC_CALL_UID_STR "call-uid"
#define JSONRPC_CC_CDR_SERV_ID  "cdr_server_id"

#define JSONRPC_CC_AMOUNT_STR   "amount"

#define JSONRPC_CC_BILLSEC_STR  "billsec"
#define JSONRPC_CC_DURATION_STR "duration"

#define JSONRPC_CC_SRV_STATE    "state"
#define JSONRPC_CC_SRV_STATUS   "status"

/* JSON-RPC CallControl errors: -50...-59 */
#define JSONRPC_CC_ERR_P_NUL       -50
#define JSONRPC_CC_ERR_MAX_NUL     -51
#define JSONRPC_CC_ERR_BAL_NUL     -52
#define JSONRPC_CC_ERR_TRM_NUL     -53
#define JSONRPC_CC_ERR_TRM_IDX_NUL -54

#define JSONRPC_CC_ERR_EVENT_UNKN  -59

//cc_funcs_t jsonrpc_cc_api;

#endif
