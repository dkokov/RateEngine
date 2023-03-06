#ifndef CDR_H
#define CDR_H

#define CDR_TABLE_NAME "cdrs"

/* CDR elements len */
#define CALL_UID_LEN 128
#define CLG_NUM_LEN  80
#define CLD_NUM_LEN  80
#define ACC_CODE_LEN 80
#define CONTEXT_LEN  80
#define TGROUP_LEN   80
#define TS_LEN       32

/* SMS RECORD TYPE */
#define SMS_MO 1
#define SMS_MT 2

/* CDR REC TYPE ID :
 * unknown,
 * ISUP,
 * VoIP(sip,audio),
 * VoIP(sip,video),
 * SMS { REC TYPE : SMS-MO,SMS-MT},
 * MAP {SMS-MAP} ?mobile interconnects?,
 * chat ?,
 * ... ??? 
 * */
typedef enum cdr_rec_type {
	unkn,
	isup,
	sms,
	voip_a,
	voip_v,
	voip_t
} cdr_rec_type_t;

/* CDR elements */
typedef struct cdr {
    /* CDR ID */
    unsigned int id; 
    
    /* CDR Server ID */
    unsigned short cdr_server_id;
    
    /* CDR Server , Profile Name */
    char profile_name[32];
    
    /* Service(REC) Type ID */ 
    unsigned short cdr_rec_type_id;
    
    /* LegA ID in the rating */
    unsigned int leg_a;
    
    /* LegB ID in the rating */
    unsigned int leg_b;    
    
    /* Call Unique ID */
    char call_uid[CALL_UID_LEN]; 
    
    /* Start Call Timestamp (2013-05-20 13:45:45+03) */
    char start_ts[TS_LEN];
    
    /* Answer Call Timestamp (2013-05-20 13:45:45+03) */
    char answer_ts[TS_LEN];
    
    /* End Call Timestamp (2013-05-20 13:45:45+03) */
    char end_ts[TS_LEN];
    
    /* Start Call Timestamp in epoch format (seconds) */
    unsigned int start_epoch;
    
    /* Answer Call Timestamp in epoch format (seconds) */
    unsigned int answer_epoch;
    
    /* End Call Timestamp in epoch format (seconds) */
    unsigned int end_epoch;      
  
    /* ??? */
    unsigned int c_epoch;
      
    /* Day of week,from 0 to 6 as 0 is Sun */
    unsigned short dow;    
  
    /* Source username */
    char src[CLG_NUM_LEN];
    
    /* Destination username */
    char dst[CLD_NUM_LEN];
    
    /* Calling Number */
    char calling_number[CLG_NUM_LEN];
    
    /* Calling Nature of Address Indicator as digit */
    unsigned short clg_nadi;    
    
    /* Called Number */
    char called_number[CLD_NUM_LEN];
        
    /* Called Nature of Address Indicator as digit */
    unsigned short cld_nadi;    
    
    /* Redirected Dialed Number */
    char rdnis[CLD_NUM_LEN];
    
    /* RDNIS Nature of Address Indicator */
    unsigned short rdnis_nadi;
    
    /* Original Called Number */
    char ocn[CLD_NUM_LEN];
    
    /* OCN Nature of Address Indicator */
    unsigned short ocn_nadi;
    
    /* Prefix Filter ID */
    unsigned short prefix_filter_id;
    
    /* Account Code */
    char account_code[ACC_CODE_LEN];
    
    /* Source Context */
    char src_context[CONTEXT_LEN];
    
    /* Source Trunk Group */
    char src_tgroup[TGROUP_LEN];
    
    /* Destination Context */
    char dst_context[CONTEXT_LEN];
    
    /* Destination Trunk Group */
    char dst_tgroup[TGROUP_LEN];
    
    /* Billing seconds time (realy call time) */
    unsigned int billsec;
    
    /* Call duration seconds time (full call time,included setup time)*/
    unsigned int duration;
    
    /* Call duration microseconds time (full call time,included setup time)*/
    unsigned int uduration;
    
    /* Billing microseconds time (realy call time) */
    unsigned int billusec;  
    
    /* SMS REC TYPE */
    //unsigned short sms_rec_type;
} cdr_t; 

/* CDR table for matching */
typedef struct cdr_table {
	
	char name[256];
	void (*func)(cdr_t *,char *);
	
} cdr_table_t;

/* A Number of the elements - CDR static table */
#define CDR_TBL_NUM 33

//static cdr_table_t cdr_tbl[CDR_TBL_NUM];

cdr_table_t *cdr_tbl_cpy(void);

/* Put in the CDR struct functions */
void cdr_add_cdr_server_id(cdr_t *cdr_ptr,char *txt);
void cdr_add_cdr_rec_type(cdr_t *cdr_ptr,char *txt);
void cdr_add_call_uid(cdr_t *cdr_ptr,char *txt);
void cdr_add_start_ts(cdr_t *cdr_ptr,char *txt);
void cdr_add_answer_ts(cdr_t *cdr_ptr,char *txt); 
void cdr_add_end_ts(cdr_t *cdr_ptr,char *txt);
void cdr_add_start_epoch(cdr_t *cdr_ptr,char *txt);
void cdr_add_answer_epoch(cdr_t *cdr_ptr,char *txt);
void cdr_add_end_epoch(cdr_t *cdr_ptr,char *txt);
void cdr_add_src(cdr_t *cdr_ptr,char *txt);
void cdr_add_dst(cdr_t *cdr_ptr,char *txt);
void cdr_add_calling_number(cdr_t *cdr_ptr,char *txt);
void cdr_add_clg_nadi(cdr_t *cdr_ptr,char *txt);
void cdr_add_called_number(cdr_t *cdr_ptr,char *txt);
void cdr_add_cld_nadi(cdr_t *cdr_ptr,char  *txt);
void cdr_add_rdnis(cdr_t *cdr_ptr,char *txt);
void cdr_add_rdnis_nadi(cdr_t *cdr_ptr,char *txt);
void cdr_add_ocn(cdr_t *cdr_ptr,char *txt);
void cdr_add_ocn_nadi(cdr_t *cdr_ptr,char *txt);
void cdr_add_account_code(cdr_t *cdr_ptr,char *txt);
void cdr_add_src_context(cdr_t *cdr_ptr,char *txt);
void cdr_add_src_tgroup(cdr_t *cdr_ptr,char *txt);
void cdr_add_dst_context(cdr_t *cdr_ptr,char *txt);
void cdr_add_dst_tgroup(cdr_t *cdr_ptr,char *txt);
void cdr_add_billsec(cdr_t *cdr_ptr,char *txt);
void cdr_add_duration(cdr_t *cdr_ptr,char *txt);
void cdr_add_uduration(cdr_t *cdr_ptr,char *txt);
void cdr_add_billusec(cdr_t *cdr_ptr,char *txt);
void cdr_add_id(cdr_t *cdr_ptr,char *txt);
void cdr_add_leg_a(cdr_t *cdr_ptr,char *txt);
void cdr_add_leg_b(cdr_t *cdr_ptr,char *txt);
void cdr_add_prefix_filter_id(cdr_t *cdr_ptr,char *txt);

int cdr_add_in_db(db_t *dbp,cdr_t *cdr_pt,filter *filters);
int cdr_add_in_db_query(db_t *dbp,cdr_t *cdr_pt);
int cdr_add_in_db_set(db_t *dbp,cdr_t *cdr_pt);

int cdr_get_cdr_id(db_t *dbp,cdr_t *the_cdr);
cdr_t *cdr_get_cdrs(db_t *dbp,char leg,int dig);
void cdr_update_cdr(db_t *dbp,int rating_id,int cdr_id,char leg,char *call_uid);

cdr_table_t *cdr_tbl_cpy_ptr;

/* *** end *** */
#endif
