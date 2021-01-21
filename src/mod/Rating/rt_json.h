#ifndef RT_JSON_H
#define RT_JSON_H

#include <json-c/json.h>

#define RT_JSON_HDR_LEN  160

#define RT_JSON_HDR_PATTERN_BACC "bacc_"

#define RT_JSON_BACC_USR_STR     "username"
#define RT_JSON_CURR_STR         "currency"
#define RT_JSON_BDAY_STR         "billing_day"
#define RT_JSON_DOP_STR          "day_of_payment"
#define RT_JSON_ROUND_STR        "round_mode"

#define RT_JSON_PCARD_STR            "pcard"
#define RT_JSON_PCARD_ID_STR         "id"
#define RT_JSON_PCARD_TYPE_STR       "type"
#define RT_JSON_PCARD_STATUS_STR     "status"
#define RT_JSON_PCARD_START_STR      "start_date"
#define RT_JSON_PCARD_END_STR        "end_date"
#define RT_JSON_PCARD_AMOUNT_STR     "amount"
#define RT_JSON_PCARD_SIM_CALLS_STR  "sim_calls"

#define RT_JSON_HDR_PATTERN_RACC_CLG    "racc_clg_"
#define RT_JSON_HDR_PATTERN_RACC_ACODE  "racc_acode_"
#define RT_JSON_HDR_PATTERN_RACC_SRCC   "racc_srcc_"
#define RT_JSON_HDR_PATTERN_RACC_DSTC   "racc_dstc_"
#define RT_JSON_HDR_PATTERN_RACC_SRCTG  "racc_srctg_"
#define RT_JSON_HDR_PATTERN_RACC_DSTTG  "racc_dsttg_"
#define RT_JSON_HDR_PATTERN_RACC_SMS    "racc_sms_"

#define RT_JSON_RACC_USR_STR     "rating_account"
#define RT_JSON_RACC_MODE_STR    "rating_mode"
#define RT_JSON_RACC_BPLAN_STR   "bplan"
#define RT_JSON_RACC_BACC_STR    "bacc"

#define RT_JSON_HDR_PATTERN_BPLAN  "bplan_"
#define RT_JSON_HDR_PATTERN_TARIFF "tariff_"
#define RT_JSON_HDR_PATTERN_RATE   "rate_"

#define RT_JSON_TARIFF_NAME_STR    "tariff_name"
#define RT_JSON_TARIFF_START_STR   "start_period"
#define RT_JSON_TARIFF_END_STR     "end_period"
#define RT_JSON_TARIFF_FSEC_STR    "free_billsec"
#define RT_JSON_TARIFF_TC_STR      "time_cond"
#define RT_JSON_TARIFF_CALC_STR    "calc_functions"

#define RT_JSON_CALC_POS_STR    "pos"
#define RT_JSON_CALC_DELTA_STR  "delta"
#define RT_JSON_CALC_TEE_STR    "fee"
#define RT_JSON_CALC_ITER_STR   "iterations"

#define RT_JSON_BPLAN_START_STR  "start_period"
#define RT_JSON_BPLAN_END_STR    "end_period"

#define RT_JSON_RATE_TARIFF_STR  "tariff"
#define RT_JSON_RATE_PREFIX_STR  "prefix"

#define RT_JSON_HDR_PATTERN_RT   "rated_"

#define RT_JSON_RT_CALL_UID_STR    "cdr_call_uid"
#define RT_JSON_RT_CDR_SERVER_STR  "cdr_server"
#define RT_JSON_RT_CPRICE_STR      "cprice"
#define RT_JSON_RT_BILLSEC_STR     "billsec"
#define RT_JSON_RT_TS_STR          "last_update"

#define RT_JSON_HDR_PATTERN_BAL    "balance_"

#define RT_JSON_BAL_AMOUNT_STR     "amount"
#define RT_JSON_BAL_START_STR      "start_period"
#define RT_JSON_BAL_END_STR        "end_period"

typedef struct rt_json {
	json_object *jobj;

	char header[RT_JSON_HDR_LEN];
	
	char *msg;
	char *filename;
} rt_json_t;

typedef struct rt_json_list {
	char header[RT_JSON_HDR_LEN];
} rt_json_list_t;

int rt_json_bacc_get(rt_json_t *rt_ptr,bacc_t *bpt);
int rt_json_racc_get(rt_json_t *rt_ptr,racc_t *rpt);
int rt_json_bplan_get(rt_json_t *rt_ptr,bplan_t *bpt);
int rt_json_rate_get(rt_json_t *rt_ptr,rate_t *rtp);
int rt_json_tariff_get(rt_json_t *rt_ptr,rate_t *rpt);
int rt_json_rated_put(rating_t *pre,rt_json_t *rt_ptr);
int rt_json_rated_get(rt_json_t *rt_ptr,rating_t *pre);
int rt_json_balance_get(rt_json_t *rt_ptr,racc_t *rtp);
int rt_json_balance_put(racc_t *rpt,rt_json_t *rt_ptr);

#endif
