#include "../../misc/globals.h"
#include "../../misc/exten/str_ext.h"
#include "../../misc/exten/time_funcs.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "prefix_filter.h"
#include "cdr.h"
#include "cdrm_json.h"

cdr_table_t *cdr_tbl_cpy_ptr;

cdr_table_t cdr_tbl[] = {
	{"id",cdr_add_id},
	{"cdr_server_id",cdr_add_cdr_server_id},
	{"cdr_rec_type_id",cdr_add_cdr_rec_type},
	{"leg_a",cdr_add_leg_a},
	{"leg_b",cdr_add_leg_b},
	{"call_uid",cdr_add_call_uid},
	{"start_ts",cdr_add_start_ts},
	{"answer_ts",cdr_add_answer_ts},
	{"end_ts",cdr_add_end_ts},
	{"start_epoch",cdr_add_start_epoch},
	{"answer_epoch",cdr_add_answer_epoch},
	{"end_epoch",cdr_add_end_epoch},
	{"src",cdr_add_src},
	{"dst",cdr_add_dst},
	{"calling_number",cdr_add_calling_number},
	{"clg_nadi",cdr_add_clg_nadi},
	{"called_number",cdr_add_called_number},
	{"cld_nadi",cdr_add_cld_nadi},
	{"rdnis",cdr_add_rdnis},
	{"rdnis_nadi",cdr_add_rdnis_nadi},
	{"ocn",cdr_add_ocn},
	{"ocn_nadi",cdr_add_ocn_nadi},
	{"prefix_filter_id",cdr_add_prefix_filter_id},
	{"account_code",cdr_add_account_code},
	{"src_context",cdr_add_src_context},
	{"src_tgroup",cdr_add_src_tgroup},
	{"dst_context",cdr_add_dst_context},
	{"dst_tgroup",cdr_add_dst_tgroup},
	{"billsec",cdr_add_billsec},
	{"duration",cdr_add_duration},
	{"uduration",cdr_add_uduration},
	{"billusec",cdr_add_billusec},
	{"",NULL}
};

char *cdr_tbl_sql_columns(void)
{
	int len,cols;
	char *sql_cols_query;
	char *buf;
	char columns[DB_BUF_LEN];
	
	bzero(columns,DB_BUF_LEN);

	cols = 0;
	while(strcmp(cdr_tbl[cols].name,"")) {
		if(strcmp(columns,"") == 0) sprintf(columns,"%s",cdr_tbl[cols].name); 
		else {
			if(strcmp(cdr_tbl[cols].name,"")) {
				buf = strdup(columns);
				bzero(columns,DB_BUF_LEN);
				sprintf(columns,"%s,%s",buf,cdr_tbl[cols].name);
				free(buf);
			}
		}
		cols++;
	}

	len = strlen(columns);
	if(len > 0) {
		sql_cols_query = strdup(columns);
		return sql_cols_query;
	}
	
	return NULL;
}

cdr_table_t *cdr_tbl_cpy(void)
{
	cdr_table_t *tbl = NULL;

	tbl = (cdr_table_t *)mem_alloc_arr(CDR_TBL_NUM,sizeof(cdr_table_t));
	if(tbl != NULL) {
		memcpy(tbl,cdr_tbl,sizeof(cdr_tbl));
	}
	
	return tbl;
}

/* CDR struct create,init,delete 
void cdr_init(cdr_t *cdr_pt)
{
	memset(cdr_pt,0,(sizeof(cdr_t)));	
}*/

cdr_t *cdr_mem_init(int num)
{
	return (cdr_t *)mem_alloc_arr(num,sizeof(cdr_t));
}

/* Bounded copy into a fixed-size CDR field.
 * CDR string fields come from external sources (CSV files,remote DB columns)
 * and must never be copied unbounded - truncate instead of overflowing. */
static void cdr_field_set(char *dst,const char *src,size_t dst_size)
{
	if((dst == NULL)||(src == NULL)||(dst_size == 0)) return;

	strncpy(dst,src,dst_size - 1);
	dst[dst_size - 1] = '\0';
}

/* Put in the CDR struct functions */
void cdr_add_id(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->id = atoi(txt);
}

void cdr_add_leg_a(cdr_t *cdr_ptr,char *txt)
{
	cdr_ptr->leg_a = atoi(txt);
}

void cdr_add_leg_b(cdr_t *cdr_ptr,char *txt)
{
	cdr_ptr->leg_b = atoi(txt);
}

void cdr_add_cdr_server_id(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->cdr_server_id = atoi(txt);
}

void cdr_add_prefix_filter_id(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->prefix_filter_id = atoi(txt);
}

void cdr_add_cdr_rec_type(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->cdr_rec_type_id = atoi(txt);
}

void cdr_add_call_uid(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->call_uid,txt,sizeof(cdr_ptr->call_uid));
}

void cdr_add_start_ts(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->start_ts,txt,sizeof(cdr_ptr->start_ts));
}

void cdr_add_answer_ts(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->answer_ts,txt,sizeof(cdr_ptr->answer_ts));
}

void cdr_add_end_ts(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->end_ts,txt,sizeof(cdr_ptr->end_ts));
}

void cdr_add_start_epoch(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->start_epoch = atoi(txt);
}

void cdr_add_answer_epoch(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->answer_epoch = atoi(txt);
}

void cdr_add_end_epoch(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->end_epoch = atoi(txt);
}

void cdr_add_src(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->src,txt,sizeof(cdr_ptr->src));
}

void cdr_add_dst(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->dst,txt,sizeof(cdr_ptr->dst));
}

void cdr_add_calling_number(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->calling_number,txt,sizeof(cdr_ptr->calling_number));
}

void cdr_add_clg_nadi(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->clg_nadi = atoi(txt);
}

void cdr_add_called_number(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->called_number,txt,sizeof(cdr_ptr->called_number));
}

void cdr_add_cld_nadi(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->cld_nadi = atoi(txt);
}

void cdr_add_rdnis(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->rdnis,txt,sizeof(cdr_ptr->rdnis));
}

void cdr_add_rdnis_nadi(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->rdnis_nadi = atoi(txt);
}

void cdr_add_ocn(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->ocn,txt,sizeof(cdr_ptr->ocn));
}

void cdr_add_ocn_nadi(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->ocn_nadi = atoi(txt);
}

void cdr_add_account_code(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->account_code,txt,sizeof(cdr_ptr->account_code));
}

void cdr_add_src_context(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->src_context,txt,sizeof(cdr_ptr->src_context));
}

void cdr_add_src_tgroup(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->src_tgroup,txt,sizeof(cdr_ptr->src_tgroup));
}

void cdr_add_dst_context(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->dst_context,txt,sizeof(cdr_ptr->dst_context));
}

void cdr_add_dst_tgroup(cdr_t *cdr_ptr,char *txt)
{
	cdr_field_set(cdr_ptr->dst_tgroup,txt,sizeof(cdr_ptr->dst_tgroup));
}

void cdr_add_billsec(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->billsec = atoi(txt);
}

void cdr_add_duration(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->duration = atoi(txt);
}

void cdr_add_uduration(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->uduration = atoi(txt);
}

void cdr_add_billusec(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->billusec = atoi(txt);
}

/* A CDR struct view */
void cdr_struct_view(cdr_t *cdr_pt)
{
    char msg[2024];
    
    sprintf(msg,"id=%d,cdr_server_id=%d,cdr_server=%s,cdr_rec_type_id=%d,leg_a=%d,leg_b=%d,\n\t"
			"call_uid=%s,start_ts=%s,answer_ts=%s,end_ts=%s,start_epoch=%d,answer_epoch=%d,end_epoch=%d,dow=%d,\n\t"
			"dow=%d,src=%s,dst=%s,calling_number=%s,clg_nadi=%d,called_number=%s,cld_nadi=%d,\n\t"
			"rdnis=%s,rdnis_nadi=%d,ocn=%s,ocn_nadi=%d,\n\t,"
			"prefix_filter_id=%d,account_code=%s,src_context=%s,src_tgroup=%s,dst_context=%s,dst_tgroup=%s,\n\t"
			"billsec=%d,duration=%d,uduration=%d,billusec=%d\n",
			cdr_pt->id,cdr_pt->cdr_server_id,cdr_pt->profile_name,cdr_pt->cdr_rec_type_id,cdr_pt->leg_a,cdr_pt->leg_b,
			cdr_pt->call_uid,cdr_pt->start_ts,cdr_pt->answer_ts,cdr_pt->end_ts,cdr_pt->start_epoch,cdr_pt->answer_epoch,cdr_pt->end_epoch,cdr_pt->dow,
			cdr_pt->dow,cdr_pt->src,cdr_pt->dst,cdr_pt->calling_number,cdr_pt->clg_nadi,cdr_pt->called_number,cdr_pt->cld_nadi,
			cdr_pt->rdnis,cdr_pt->rdnis_nadi,cdr_pt->ocn,cdr_pt->ocn_nadi,
			cdr_pt->prefix_filter_id,cdr_pt->account_code,cdr_pt->src_context,cdr_pt->src_tgroup,cdr_pt->dst_context,cdr_pt->dst_tgroup,
			cdr_pt->billsec,cdr_pt->duration,cdr_pt->uduration,cdr_pt->billusec);

	LOG("cdr_struct_view()",msg);
}

/* Matching 'CalledNumber' with filtering number and replace if it's need */
void cdr_called_number_filtering_match(filter *filters,cdr_t *cdr_pt)
{
    int p;
    char ret[80];

    p=0;
    while(filters[p].id) {
//		if((cdr_pt->cdr_server_id == filters[p].cdr_server_id) || (filters[p].cdr_server_id == 0)) {
			bzero(ret,80);
			
			prefix_filter_cuti_replace(cdr_pt->called_number,filters[p].filtering_prefix,filters[p].filtering_number,filters[p].replace_str,ret,filters[p].len);

			if(strcmp(ret,"")) {
				strcpy(cdr_pt->called_number,ret);
				cdr_pt->prefix_filter_id = filters[p].id;
				
				DBG("cdr_called_number_filtering()","replace: %s",ret);
				
				break;
			}
//		}	
		p++;
    }
}

/* Get 'CDR ID' from the 'cdrs' table */
int cdr_get_cdr_id(db_t *dbp,cdr_t *the_cdr)
{
	int id,ret;
    char str[DB_BUF_LEN] = {0};
    	
	db_sql_result_t *result;

	id = 0;

	if(dbp->t == sql) {
		char e_call_uid[CALL_UID_LEN*2+1];

		db_sql_escape(the_cdr->call_uid,e_call_uid,sizeof(e_call_uid));

		sprintf(str,"select id from %s where call_uid = '%s' and cdr_server_id = %d",
				CDR_TABLE_NAME,e_call_uid,the_cdr->cdr_server_id);

		db_select(dbp,str);
		db_fetch(dbp);

		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
		
			if(result->rows == 1) id = atoi(result->cols_list[0].rows_list[0].row);
		
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	} else if(dbp->t == nosql) { 
		sprintf(str,"get %s%s",CDR_JSON_HDR_PATTERN,the_cdr->call_uid);
		
		ret = db_get(dbp,str);
		if(ret < 0) {
			db_error(ret);
			
/*			memset(str,0,DB_BUF_LEN);
			sprintf(str,"get %s%s",CDR_JSON_HDR_PATTERN_R,the_cdr->call_uid);

			ret = db_get(dbp,str);
			if(ret == DB_OK) return 1; */

			return 0; // ??? ne e sigurno,moje da e nqkakva gre6ka ...reply = null ili reply->str = null ???
		}
					
		db_free_result(dbp);
		db_nosql_result_free((db_nosql_result_t *)dbp->conn->result);
		dbp->conn->result = NULL;
		
		return 1;
	} else return -1;
    
    return id;
}

/* ??? 
cdr_t *cdr_get_from_db(char *call_uid)
{
	cdr_t *the_cdr;
	
	the_cdr = cdr_mem_init(1);
	
	if(the_cdr != NULL) {
		
	}
	
	return the_cdr;
}*/

/* Get all no rating CDRs */
cdr_t *cdr_get_cdrs(db_t *dbp,char leg,int dig)
{
	int c,p;
	char *columns;	
    char str[SQL_BUF_LEN];

	db_sql_result_t *result;
	db_nosql_result_t *tmp;
	
	cdr_t *cdrs = NULL;
	
	if(dbp->t == sql) {	
		columns = cdr_tbl_sql_columns();

		if(columns != NULL) {
			bzero(str,sizeof(str));
			
			sprintf(str,"select %s from %s where leg_%c = %d order by id limit %d",columns,CDR_TABLE_NAME,leg,dig,CDR_BATCH_LIMIT);

			mem_free(columns);
			
			db_select(dbp,str);
			db_fetch(dbp);
			
			if(dbp->conn->result != NULL) {
				result = (db_sql_result_t *)dbp->conn->result;

				if(result->rows > 0) {
					cdrs = cdr_mem_init(result->rows+1);
					if(cdrs != NULL) {
						for(c=0;c<result->rows;c++) {
							for(p = 0;p < (CDR_TBL_NUM - 1);p++) {
								(*cdr_tbl[p].func)(&cdrs[c],result->cols_list[p].rows_list[c].row);
							}
						}
					}
				}
			
				db_sql_result_free(result);
				dbp->conn->result = NULL;
			}
		}
	} else if(dbp->t == nosql) {
		int ret;
		
		ret = db_command(dbp,"keys cdr_call_*");
		
		if(ret < 0) db_error(ret);
		else {
			int n;
			char buf[SQL_BUF_LEN];

			cdrm_json_t cdr_json_pt;
			cdrm_json_list_t *list;
			
			if(dbp->conn->result != NULL) {
				tmp = (db_nosql_result_t *)dbp->conn->result;
				n = tmp->elements + 1;
			
				list = (cdrm_json_list_t *)mem_alloc_arr(n,sizeof(cdrm_json_list_t));
			
				for(c=0;c<tmp->elements;c++) strcpy(list[c].header,tmp->arr[c].key);

				db_nosql_result_free(tmp);
				dbp->conn->result = NULL;
				
				db_free_result(dbp);
				dbp->conn->res = NULL;
				
				cdrs = cdr_mem_init(n);

				c=0;
				while(strlen(list[c].header)>0) {
					//printf("header: %s\n",list[c].header);
					memset(buf,0,SQL_BUF_LEN);
					sprintf(buf,"get %s",list[c].header);
				
					ret = db_get(dbp,buf);
					if(ret == DB_OK) {
						tmp = (db_nosql_result_t *)dbp->conn->result;

						if(tmp->len > 0) {
//							if(strcmp(tmp->str,"'\x80'") == 0) printf("\nERROR\n");
//							else {
							memset(&cdr_json_pt,0,sizeof(cdrm_json_t));
							//printf("json str: %s\n\n",tmp->str);
							cdr_json_pt.msg = str_ext_clear_str(tmp->str,'\'');
							
							cdrm_json_struct(&cdr_json_pt,&cdrs[c]);
	
							//cdr_struct_view(&cdrs[c]);
							
							mem_free(cdr_json_pt.msg);
//							}
						}

						db_nosql_result_free(tmp);
						dbp->conn->result = NULL;
				
						db_free_result(dbp);
						dbp->conn->res = NULL;
					}
					
					c++;
				}
			}
		}
	}
	
	return cdrs;
}

void cdr_update_cdr(db_t *dbp,int rating_id,int cdr_id,char leg,char *call_uid)
//void cdr_update_cdr(db_t *dbp,rating pre,char leg)
{
	int ret;
    char str[SQL_BUF_LEN];

	if((cdr_id > 0)&&((leg == 'a')||(leg == 'b'))) {
		if(dbp->t == sql) {
			bzero(str,SQL_BUF_LEN);
			sprintf(str,"update %s set leg_%c = %d where id = %d",CDR_TABLE_NAME,leg,rating_id,cdr_id);
			
			db_update(dbp,str);	
		} else if(dbp->t == nosql) {
			bzero(str,SQL_BUF_LEN);
			sprintf(str,"RENAME %s%s %s%s",CDR_JSON_HDR_PATTERN,call_uid,CDR_JSON_HDR_PATTERN_R,call_uid);
			
			ret = db_command(dbp,str);
			if(ret < 0) {
				LOG("cdr_update_cdr()","cdr update no success!");
				db_error(ret);
			}
		}
	}
}

/* insert data from the CDR struct in the DB,table cdrs */
int cdr_add_in_db(db_t *dbp,cdr_t *cdr_pt,filter *filters)
{	
	char buf[DT_LEN];
    
	if(strlen(cdr_pt->start_ts) > DT_LEN) {
		convert_datetime(buf,cdr_pt->start_ts);
		
		memset(cdr_pt->start_ts,0,strlen(cdr_pt->start_ts));
		strcpy(cdr_pt->start_ts,buf);
	}
		
	if(strlen(cdr_pt->answer_ts) > DT_LEN) {
		convert_datetime(buf,cdr_pt->answer_ts);
		
		memset(cdr_pt->answer_ts,0,strlen(cdr_pt->answer_ts));
		strcpy(cdr_pt->answer_ts,buf);
	} 
				
	if(strlen(cdr_pt->end_ts) > DT_LEN) {
		convert_datetime(buf,cdr_pt->end_ts);
		
		memset(cdr_pt->end_ts,0,strlen(cdr_pt->end_ts));
		strcpy(cdr_pt->end_ts,buf);
	}
			
    if(filters != NULL) cdr_called_number_filtering_match(filters,cdr_pt);
	
	if(cdr_get_cdr_id(dbp,cdr_pt) == 0) {
		if(dbp->t == sql) return cdr_add_in_db_query(dbp,cdr_pt);
		else if(dbp->t == nosql) return cdr_add_in_db_set(dbp,cdr_pt);
	}

	return -1;
}

int cdr_add_in_db_set(db_t *dbp,cdr_t *cdr_pt)
{
	int ret;
	cdrm_json_t cdr_json_pt;
	char buf[CDR_JSON_BUF_LEN];
		
	cdr_pt->cdr_server_id = 1; // temp patch ???
		
	if((cdr_pt->start_epoch == 0)&&(strlen(cdr_pt->start_ts) > 0)) cdr_pt->start_epoch = convert_ts_to_epoch(cdr_pt->start_ts);
	if((cdr_pt->answer_epoch == 0)&&(strlen(cdr_pt->answer_ts) > 0)) cdr_pt->answer_epoch = convert_ts_to_epoch(cdr_pt->answer_ts);
	if((cdr_pt->end_epoch == 0)&&(strlen(cdr_pt->end_ts) > 0)) cdr_pt->end_epoch = convert_ts_to_epoch(cdr_pt->end_ts);
		
	memset(buf,0,CDR_JSON_BUF_LEN);
		
	cdrm_struct_json(cdr_pt,&cdr_json_pt);
		
	sprintf(buf,"SET %s '%s'",cdr_json_pt.header,cdr_json_pt.msg);
	
	ret = db_set(dbp,buf,NULL);
	if(ret < 0) {
		db_error(ret);
		LOG("cdr_add_in_db()","The CDR (%s) is not inserted!",cdr_pt->call_uid);
			
		json_ext_write_file(cdr_json_pt.header,cdr_json_pt.msg);
	}
		
	mem_free(cdr_json_pt.msg);

	return ret;
}

int cdr_add_in_db_query(db_t *dbp,cdr_t *cdr_pt)
{
	int ret;
    char str[SQL_BUF_LEN*2]; /* escaped fields can ~double - size for worst case */

	/* Escape every string field - CDR data is externally sourced */
	char e_call_uid[CALL_UID_LEN*2+1];
	char e_start_ts[TS_LEN*2+1],e_answer_ts[TS_LEN*2+1],e_end_ts[TS_LEN*2+1];
	char e_src[CLG_NUM_LEN*2+1],e_dst[CLD_NUM_LEN*2+1];
	char e_calling[CLG_NUM_LEN*2+1],e_called[CLD_NUM_LEN*2+1];
	char e_rdnis[CLD_NUM_LEN*2+1],e_ocn[CLD_NUM_LEN*2+1];
	char e_acc[ACC_CODE_LEN*2+1];
	char e_src_ctx[CONTEXT_LEN*2+1],e_src_tg[TGROUP_LEN*2+1];
	char e_dst_ctx[CONTEXT_LEN*2+1],e_dst_tg[TGROUP_LEN*2+1];

	db_sql_escape(cdr_pt->call_uid,e_call_uid,sizeof(e_call_uid));
	db_sql_escape(cdr_pt->start_ts,e_start_ts,sizeof(e_start_ts));
	db_sql_escape(cdr_pt->answer_ts,e_answer_ts,sizeof(e_answer_ts));
	db_sql_escape(cdr_pt->end_ts,e_end_ts,sizeof(e_end_ts));
	db_sql_escape(cdr_pt->src,e_src,sizeof(e_src));
	db_sql_escape(cdr_pt->dst,e_dst,sizeof(e_dst));
	db_sql_escape(cdr_pt->calling_number,e_calling,sizeof(e_calling));
	db_sql_escape(cdr_pt->called_number,e_called,sizeof(e_called));
	db_sql_escape(cdr_pt->rdnis,e_rdnis,sizeof(e_rdnis));
	db_sql_escape(cdr_pt->ocn,e_ocn,sizeof(e_ocn));
	db_sql_escape(cdr_pt->account_code,e_acc,sizeof(e_acc));
	db_sql_escape(cdr_pt->src_context,e_src_ctx,sizeof(e_src_ctx));
	db_sql_escape(cdr_pt->src_tgroup,e_src_tg,sizeof(e_src_tg));
	db_sql_escape(cdr_pt->dst_context,e_dst_ctx,sizeof(e_dst_ctx));
	db_sql_escape(cdr_pt->dst_tgroup,e_dst_tg,sizeof(e_dst_tg));

		snprintf(str,sizeof(str),
			"insert into %s "
			"(cdr_server_id,cdr_rec_type_id,leg_a,leg_b,"
			"call_uid,start_ts,answer_ts,end_ts,start_epoch,answer_epoch,end_epoch,"
			"src,dst,calling_number,clg_nadi,called_number,cld_nadi,"
			"rdnis,rdnis_nadi,ocn,ocn_nadi,prefix_filter_id,"
			"account_code,src_context,src_tgroup,dst_context,dst_tgroup,"
			"billsec,duration,uduration,billusec) "
			"values "
			"(%d,%d,%d,%d,"
			"'%s','%s','%s','%s',%d,%d,%d,"
			"'%s','%s','%s',%d,'%s',%d,"
			"'%s',%d,'%s',%d,%d,"
			"'%s','%s','%s','%s','%s',"
			"%d,%d,%d,%d)",
			CDR_TABLE_NAME,
			cdr_pt->cdr_server_id,cdr_pt->cdr_rec_type_id,cdr_pt->leg_a,cdr_pt->leg_b,
			e_call_uid,e_start_ts,e_answer_ts,e_end_ts,cdr_pt->start_epoch,cdr_pt->answer_epoch,cdr_pt->end_epoch,
			e_src,e_dst,e_calling,cdr_pt->clg_nadi,e_called,cdr_pt->cld_nadi,
			e_rdnis,cdr_pt->rdnis_nadi,e_ocn,cdr_pt->ocn_nadi,cdr_pt->prefix_filter_id,
			e_acc,e_src_ctx,e_src_tg,e_dst_ctx,e_dst_tg,
			cdr_pt->billsec,cdr_pt->duration,cdr_pt->uduration,cdr_pt->billusec);

		ret = db_insert(dbp,str);

	return ret;
}
