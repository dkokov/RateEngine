#ifndef CDR_CFG_H
#define CDR_CFG_H

#define PROF_VER_LEN 4
#define PROF_NAME_LEN 32

#define CDR_TYPE_LEN 5
#define CDR_SEPARATOR_LEN 2

#define REM_ACC_TYPE_LEN 5
#define REM_ACC_SERV_LEN 128
#define REM_ACC_USER_LEN 64
#define REM_ACC_PASS_LEN 64

#define FILE_CDR_TYPE "file"
#define DB_CDR_TYPE   "db"

#define DB_TYPE_PGSQL  "pgsql"
#define DB_TYPE_MYSQL  "mysql"
#define DB_TYPE_ORACLE "oracle"

#define CDR_REC_TYPE_UNKN "unkn"
#define CDR_REC_TYPE_ISUP "isup"
#define CDR_REC_TYPE_SMS "sms"
#define CDR_REC_TYPE_VA "voip-audio"
#define CDR_REC_TYPE_VV "voip-video"
#define CDR_REC_TYPE_VT "voip-trunk"

#define CDR_TCP_CONN 4

typedef struct cdr_profiles_list {
	
	char filename[256];
	
}cdr_profiles_list_t;

/* profile type */
typedef enum cdr_profile_type {
	db,
	file
}cdr_profile_type_t;

typedef struct cdr_profile_cfg {

	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
	
	unsigned short profile_version;
	char profile_name[PROF_NAME_LEN];
	
	cdr_profile_type_t t;
	
	cdr_rec_type_t t2;
	
	/* CDR fields from file */
	cdr_file_profile_t *profile_file_type;
	
	/* CDR columns from db table */
	cdr_storage_profile_t *profile_db_type;
	
	unsigned short cdr_profile_id;
	unsigned short cdr_server_id;
	
    int cdr_interval;
    int cdr_replies;
    
    char called_number_filtering;
//    unsigned short filter_number;
    
    PGconn *conn;
    filter *filters;
	
	char cdr_active_flag;
	
}cdr_profile_cfg_t;

typedef struct cdr_cfg {
	
	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
	    
	unsigned short cdr_tcp_conn;
	char cdr_profiles_dir[256];
	cdr_profiles_list_t *list;
	
	unsigned short profiles_number;
	cdr_profile_cfg_t *profiles;
	
}cdr_cfg_t; 

cdr_cfg_t *cdr_cfg_init(void);
void cdr_cfg_get(cdr_cfg_t *cfg);
cdr_profile_cfg_t *cdr_profile_cfg_init(int n);
void cdr_profile_cfg_get(cdr_profile_cfg_t *cfg);
cdr_cfg_t *cdr_cfg_main(char *xmlFileName);
int cdr_profile_params_num(xml_param_t *params);
void cdr_cfg_file_profile_xml(cdr_profile_cfg_t *cfg);
void cdr_cfg_storage_profile_xml(cdr_profile_cfg_t *cfg);
void cdr_cfg_view(cdr_cfg_t *cfg);

#endif
