#ifndef CDRM_JSON_H
#define CDRM_JSON_H

#include "../../misc/exten/json_ext.h"

#include "prefix_filter.h"
#include "cdr.h"
#include "cdr_file.h"
#include "cdr_storage.h"
#include "cdr_storage_sched.h"
#include "cdr_cfg.h"

#define CDR_HDR_LEN      160
#define CDR_JSON_BUF_LEN 2048

#define CDR_JSON_OK 0

#define CDR_JSON_CDR_PROFILE_HDR_PATTERN  "cdrm_profile_"
#define CDR_JSON_CDR_PROFILE_CFG_FILE_STR "cdr_cfg_file"

#define CDR_JSON_CDR_SCHED_HDR_PATTERN    "cdrm_sched_"
#define CDR_JSON_CDR_SCHED_TS_STR         "ts"
#define CDR_JSON_CDR_SCHED_LAST_STR       "last"
#define CDR_JSON_CDR_SCHED_START_STR      "start"
#define CDR_JSON_CDR_SCHED_REPLIES_STR    "replies"

/* CDR */
#define CDR_JSON_HDR_PATTERN       "cdr_call_"
#define CDR_JSON_HDR_PATTERN_R     "rcdr_call_"

#define CDR_JSON_CDR_SERVER_STR    "cdr-server"
#define CDR_JSON_CDR_REC_TYPE_STR  "cdr-rec-type"

#define CDR_JSON_CALL_UID_STR      "call-uid"
#define CDR_JSON_CLG_STR           "clg"
#define CDR_JSON_CLD_STR           "cld"

#define CDR_JSON_START_TS_STR      "start_ts"
#define CDR_JSON_ANSWER_TS_STR     "answer_ts"
#define CDR_JSON_END_TS_STR        "end_ts"

#define CDR_JSON_START_EPOCH_STR   "start_epoch"
#define CDR_JSON_ANSWER_EPOCH_STR  "answer_epoch"
#define CDR_JSON_END_EPOCH_STR     "end_epoch"

#define CDR_JSON_BILLSEC_STR       "billsec"
#define CDR_JSON_DURATION_STR      "duration"

#define CDR_JSON_SRC_CONTEXT_STR   "src_context"
#define CDR_JSON_DST_CONTEXT_STR   "dst_context"

#define CDR_JSON_SRC_TGROUP_STR    "src_tgroup"
#define CDR_JSON_DST_TGROUP_STR    "dst_tgroup"

/* prefix filter */
#define CDR_JSON_PFILTER_ARR_STR      "filters"    
#define CDR_JSON_PFILTER_PREFIX_STR   "filtering_prefix"
#define CDR_JSON_PFILTER_NUMBER_STR   "filtering_number"
#define CDR_JSON_PFILTER_REPLACE_STR  "replace_str"
#define CDR_JSON_PFILTER_LEN_STR      "len"


typedef struct cdrm_json {
	json_object *jobj;

	char header[CDR_HDR_LEN];
	
	char *msg;
} cdrm_json_t;

typedef struct cdrm_json_list {
	char header[CDR_HDR_LEN];
} cdrm_json_list_t;

int cdrm_struct_json(cdr_t *cdr_ptr,cdrm_json_t *cdr_json_ptr);
int cdrm_json_struct(cdrm_json_t *cdr_json_ptr,cdr_t *cdr_ptr);

int cdrm_profile_struct_json(cdr_profile_cfg_t *profile,cdrm_json_t *cdr_json_ptr);

int cdrm_sched_struct_json(cdr_storage_profile_t *cfg,cdrm_json_t *cdr_json_ptr);
int cdrm_json_sched_struct(cdrm_json_t *cdr_json_ptr,cdr_storage_sched_t *sched);

#endif
