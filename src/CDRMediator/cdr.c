#include "../misc/globals.h"
#include "../misc/mem/mem.h"

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
	{"billusec",cdr_add_billusec}
};

char *cdr_tbl_sql_columns(void)
{
	int len,cols;
	char *sql_cols_query;
	char *buf;
	char columns[SQL_BUF_LEN];
	
	bzero(columns,sizeof(columns));

	cols = 0;
	while(strcmp(cdr_tbl[cols].name,"")) {
		if(strcmp(columns,"") == 0) sprintf(columns,"%s",cdr_tbl[cols].name); 
		else {
			if(strcmp(cdr_tbl[cols].name,"")) {
				buf = strdup(columns);
				bzero(columns,SQL_BUF_LEN);
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

/* CDR struct create,init,delete */
void cdr_init(cdr_t *cdr_pt)
{
	memset(cdr_pt,0,(sizeof(cdr_t)));	
}

cdr_t *cdr_mem_init(int num)
{
    cdr_t *cdr_pt;
    
    cdr_pt = 0;
    
    cdr_pt = (cdr_t *)mem_alloc_arr(num,sizeof(cdr_t));
        
    return cdr_pt;
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
	strcpy(cdr_ptr->call_uid,txt);
}

void cdr_add_start_ts(cdr_t *cdr_ptr,char *txt) 
{
	strcpy(cdr_ptr->start_ts,txt);
}

void cdr_add_answer_ts(cdr_t *cdr_ptr,char *txt) 
{
	strcpy(cdr_ptr->answer_ts,txt);
}

void cdr_add_end_ts(cdr_t *cdr_ptr,char *txt) 
{
	strcpy(cdr_ptr->end_ts,txt);
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
	strcpy(cdr_ptr->src,txt);
}

void cdr_add_dst(cdr_t *cdr_ptr,char *txt) 
{
	strcpy(cdr_ptr->dst,txt);
}

void cdr_add_calling_number(cdr_t *cdr_ptr,char *txt) 
{
	strcpy(cdr_ptr->calling_number,txt);
}

void cdr_add_clg_nadi(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->clg_nadi = atoi(txt);
}

void cdr_add_called_number(cdr_t *cdr_ptr,char *txt) 
{
	strcpy(cdr_ptr->called_number,txt);
}

void cdr_add_cld_nadi(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->cld_nadi = atoi(txt);
}

void cdr_add_rdnis(cdr_t *cdr_ptr,char *txt)
{
	strcpy(cdr_ptr->rdnis,txt);
}

void cdr_add_rdnis_nadi(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->rdnis_nadi = atoi(txt);
}

void cdr_add_ocn(cdr_t *cdr_ptr,char *txt)
{
	strcpy(cdr_ptr->ocn,txt);
}

void cdr_add_ocn_nadi(cdr_t *cdr_ptr,char *txt) 
{
	cdr_ptr->ocn_nadi = atoi(txt);
}

void cdr_add_account_code(cdr_t *cdr_ptr,char *txt)
{
	strcpy(cdr_ptr->account_code,txt);
}

void cdr_add_src_context(cdr_t *cdr_ptr,char *txt)
{
	strcpy(cdr_ptr->src_context,txt);
}

void cdr_add_src_tgroup(cdr_t *cdr_ptr,char *txt)
{
	strcpy(cdr_ptr->src_tgroup,txt);
}

void cdr_add_dst_context(cdr_t *cdr_ptr,char *txt)
{
	strcpy(cdr_ptr->dst_context,txt);
}

void cdr_add_dst_tgroup(cdr_t *cdr_ptr,char *txt)
{
	strcpy(cdr_ptr->dst_tgroup,txt);
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
    char msg[1024];
    
    sprintf(msg,"id=%d,cdr_rec_type_id=%d,leg_a=%d,leg_b=%d,\n\t"
			"call_uid=%s,start_ts=%s,answer_ts=%s,end_ts=%s,start_epoch=%d,answer_epoch=%d,end_epoch=%d,dow=%d,\n\t"
			"dow=%d,src=%s,dst=%s,calling_number=%s,clg_nadi=%d,called_number=%s,cld_nadi=%d,\n\t"
			"rdnis=%s,rdnis_nadi=%d,ocn=%s,ocn_nadi=%d,\n\t,"
			"prefix_filter_id=%d,account_code=%s,src_context=%s,src_tgroup=%s,dst_context=%s,dst_tgroup=%s,\n\t"
			"billsec=%d,duration=%d,uduration=%d,billusec=%d\n",
			cdr_pt->id,cdr_pt->cdr_rec_type_id,cdr_pt->leg_a,cdr_pt->leg_b,
			cdr_pt->call_uid,cdr_pt->start_ts,cdr_pt->answer_ts,cdr_pt->end_ts,cdr_pt->start_epoch,cdr_pt->answer_epoch,cdr_pt->end_epoch,cdr_pt->dow,
			cdr_pt->dow,cdr_pt->src,cdr_pt->dst,cdr_pt->calling_number,cdr_pt->clg_nadi,cdr_pt->called_number,cdr_pt->cld_nadi,
			cdr_pt->rdnis,cdr_pt->rdnis_nadi,cdr_pt->ocn,cdr_pt->ocn_nadi,
			cdr_pt->prefix_filter_id,cdr_pt->account_code,cdr_pt->src_context,cdr_pt->src_tgroup,cdr_pt->dst_context,cdr_pt->dst_tgroup,
			cdr_pt->billsec,cdr_pt->duration,cdr_pt->uduration,cdr_pt->billusec);

	re_write_syslog_2(config.log,"cdr_struct_view()",msg);
}

/* Matching 'CalledNumber' with filtering number and replace if it's need */
void cdr_called_number_filtering_match(filter *filters,cdr_t *cdr_pt)
{
    int p;
    char ret[80];

    p=0;
    while(filters[p].id) {
		if((cdr_pt->cdr_server_id == filters[p].cdr_server_id) || (filters[p].cdr_server_id == 0)) {
			bzero(ret,80);
			
			prefix_filter_cuti_replace(cdr_pt->called_number,filters[p].filtering_prefix,filters[p].filtering_number,filters[p].replace_str,ret,filters[p].len);

			if(strcmp(ret,"")) {
				strcpy(cdr_pt->called_number,ret);
				cdr_pt->prefix_filter_id = filters[p].id;
				
				if(log_debug_level > LOG_LEVEL_DEBUG)
					LOG("called_number_filtering()","replace: %s",ret);
				
				break;
			}
		}	
		p++;
    }
}

/* Get 'CDR ID' from the 'cdrs' table */
int cdr_get_cdr_id(PGconn *conn,cdr_t *the_cdr)
{
    PGresult *res;

    char str[SQL_BUF_LEN];
    int i,num,id;
    int fnum;

	id = 0;
    num = 0;

    sprintf(str,"select id from %s where call_uid = '%s' and cdr_server_id = %d",
			CDR_TABLE_NAME,the_cdr->call_uid,the_cdr->cdr_server_id);

	res = db_pgsql_exec(conn,str);
	if(res != NULL) {
		num = PQntuples(res);

		if(num > 0) {
			fnum = PQfnumber(res,"id");

			for (i = 0; i < num; i++) {
				id = atoi(PQgetvalue(res,i,fnum));
			}
		}

		PQclear(res);
	}
    
    return id;
}

/* ??? */
cdr_t *cdr_get_from_db(char *call_uid)
{
	cdr_t *the_cdr;
	
	the_cdr = cdr_mem_init(1);
	
	if(the_cdr != NULL) {
		
	}
	
	return the_cdr;
}

/* Get all no rating CDRs */
cdr_t *cdr_get_cdrs(PGconn *conn,char leg,int dig)
{
	int *fnum;
	int i,c,p,rows;
	PGresult *res;
    char str[SQL_BUF_LEN];
	char *columns;
	
	cdr_t *cdrs = NULL;
	
	columns = cdr_tbl_sql_columns();
	
	if(columns != NULL) {
		bzero(str,sizeof(str));
		sprintf(str,
				"select %s from %s where leg_%c = %d",
				columns,CDR_TABLE_NAME,leg,dig);
		mem_free(columns);
	
		res = db_pgsql_exec(conn,str);
		if(res == NULL) return NULL;
			
		fnum = (int *)mem_alloc_arr(CDR_TBL_NUM,sizeof(int));
		if(fnum != NULL) {
			for(i=0;i < CDR_TBL_NUM;i++) fnum[i] = PQfnumber(res,cdr_tbl[i].name);
	
			rows = PQntuples(res);
			if(rows > 0) {
				cdrs = cdr_mem_init(rows+1);
				if(cdrs != NULL) {
					for(c=0;c<rows;c++) {
						for(p = 0;p < (CDR_TBL_NUM - 1);p++) {
							(*cdr_tbl[p].func)(&cdrs[c],PQgetvalue(res,c,fnum[p]));
						}			
					}					
				}
			}
		
			mem_free(fnum);
		}
		
		PQclear(res);
	}
	
	return cdrs;
}

void cdr_update_cdr(PGconn *conn,int rating_id,int cdr_id,char leg)
{
    PGresult *res;
    char str[512];

	if((cdr_id > 0)&&((leg == 'a')||(leg == 'b'))) {
		sprintf(str,"update %s set leg_%c = %d where id = %d",CDR_TABLE_NAME,leg,rating_id,cdr_id);
		
		res = db_pgsql_exec(conn,str);
		
		if(res != NULL) PQclear(res);
	}
}

/* insert data from the CDR struct in the DB,table cdrs */
int cdr_add_in_db(PGconn *conn,cdr_t *cdr_pt,filter *filters)
{
    if(filters != NULL) cdr_called_number_filtering_match(filters,cdr_pt);
    
    if(cdr_get_cdr_id(conn,cdr_pt) == 0) return cdr_add_in_db_query(conn,cdr_pt);
	else return 0;
}

int cdr_add_in_db_query(PGconn *conn,cdr_t *cdr_pt)
{
	PGresult *res;
    char str[SQL_BUF_LEN];
    
    sprintf(str,
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
			cdr_pt->call_uid,cdr_pt->start_ts,cdr_pt->answer_ts,cdr_pt->end_ts,cdr_pt->start_epoch,cdr_pt->answer_epoch,cdr_pt->end_epoch,
			cdr_pt->src,cdr_pt->dst,cdr_pt->calling_number,cdr_pt->clg_nadi,cdr_pt->called_number,cdr_pt->cld_nadi,
			cdr_pt->rdnis,cdr_pt->rdnis_nadi,cdr_pt->ocn,cdr_pt->ocn_nadi,cdr_pt->prefix_filter_id,
			cdr_pt->account_code,cdr_pt->src_context,cdr_pt->src_tgroup,cdr_pt->dst_context,cdr_pt->dst_tgroup,
			cdr_pt->billsec,cdr_pt->duration,cdr_pt->uduration,cdr_pt->billusec);

	res = db_pgsql_exec(conn,str);
	if(res != NULL) {
		PQclear(res);
		return 1;
	} else return 0;
}
