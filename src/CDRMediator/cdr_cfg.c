#include "../misc/globals.h"
#include "../misc/mem/mem.h"

#include <dirent.h> 

#include "cdr_storage.h"
#include "cdr_file.h"
#include "cdr_cfg.h"

cdr_cfg_t *cdr_cfg_init(void)
{
	cdr_cfg_t *cfg;
	
	cfg = (cdr_cfg_t *)mem_alloc(sizeof(cdr_cfg_t));
	
	return cfg;
}

void cdr_cfg_view(cdr_cfg_t *cfg)
{
	//printf("cdr_cfg_view(),%c",cfg->cdr_active_flag);
}

void cdr_cfg_get(cdr_cfg_t *cfg)
{
	xml_param_t *params = NULL;
	
	if(cfg != NULL) {
		cfg->node = xml_cfg_node_init();
		strcpy(cfg->node->node_name,"CDRMediator");
	
		xml_cfg_params_get(cfg->root,cfg->node);

		params = cfg->node->params;
	
		while(params != NULL) {
			if(strcmp(params->name,"CDRProfilesDIR") == 0) {
				strcpy(cfg->cdr_profiles_dir,params->value);
			}
			
			if(strcmp(params->name,"CDRTCPConn") == 0) {
				cfg->cdr_tcp_conn = atoi(params->value);
			}			
			
			if(strcmp(params->name,"CDRMMonitoringInterval") == 0) {
				cfg->cdrm_interval = atoi(params->value);
			}			
			
			params = params->next_param;
		}
	
		xml_cfg_params_free(cfg->node->params);
	}
}

cdr_profiles_list_t *cdr_profiles_list_init(int n)
{
	cdr_profiles_list_t *list;
	
	list = (cdr_profiles_list_t *)mem_alloc((sizeof(cdr_profiles_list_t)*n));

	return list;
}

void cdr_profiles_list_get(cdr_cfg_t *cfg)
{
	int i;
	DIR  *d;
	struct dirent *dir;

	i = 0;

	d = opendir(cfg->cdr_profiles_dir);
	
	if(d) {
		while ((dir = readdir(d)) != NULL) {
			i++;
		}

		if(i > 2) {
			seekdir(d,0);
		
			cfg->profiles_number = (i - 2);

			cfg->list = cdr_profiles_list_init(cfg->profiles_number);
		
			i=0;
			while ((dir = readdir(d)) != NULL) {
				if((strcmp(dir->d_name,".") == 0)||(strcmp(dir->d_name,"..") == 0)) {
					continue;
				} else {
					sprintf(cfg->list[i].filename,"%s%s",cfg->cdr_profiles_dir,dir->d_name);
					i++;
				}
			}
		}
		
		closedir(d);
	}
}

cdr_profile_cfg_t *cdr_profile_cfg_init(int n)
{
	cdr_profile_cfg_t *cfg;
	
	cfg = (cdr_profile_cfg_t *)mem_alloc((sizeof(cdr_profile_cfg_t)*n));
	
	return cfg;
}

int cdr_profile_params_num(xml_param_t *params)
{
	int num;
	
	num = 0;
	while(params != NULL) {
		if(strcmp((params->value),"")) num ++;

		params = params->next_param;
	}

	return num;
}

void cdr_profile_cfg_get(cdr_profile_cfg_t *cfg)
{
	xml_param_t *params;
	
	if(cfg != NULL) {
		/* General section from a profile xml file */
		cfg->node = xml_cfg_node_init();
		strcpy(cfg->node->node_name,"General");
	
		xml_cfg_params_get(cfg->root,cfg->node);
	
		params = cfg->node->params;
	
		while(params != NULL) {
			if(strcmp(params->name,"profile-name") == 0) {
				strcpy(cfg->profile_name,params->value);
			}		
			
			if(strcmp(params->name,"profile-version") == 0) {
				cfg->profile_version = atoi(params->value);
			}	
			
			if(strcmp(params->name,"active") == 0) {
				if(strcmp(params->value,"yes") == 0) cfg->cdr_active_flag = 't';
				else cfg->cdr_active_flag = 'f';
			}
			
			if(strcmp(params->name,"CalledNumberFiltering") == 0) {
				if(strcmp(params->value,"yes") == 0) cfg->called_number_filtering = 't';
				else cfg->called_number_filtering = 'f';
			}
	
			if(strcmp(params->name,"getCDRsInterval") == 0) {
				cfg->cdr_interval = atoi(params->value);
			}
		
			if(strcmp(params->name,"getCDRsReplies") == 0) {
				cfg->cdr_replies = atoi(params->value);
			}
			
			if(strcmp(params->name,"cdr-type") == 0) {
				if(strcmp(params->value,FILE_CDR_TYPE) == 0) {
					cfg->t = file;
					cfg->profile_file_type = cdr_file_profile_init();
				}
				
				if(strcmp(params->value,DB_CDR_TYPE) == 0) {
					cfg->t = db;
					cfg->profile_db_type = cdr_storage_profile_init();
				}
			}				
			
			if((cfg->t == file)&&(cfg->profile_file_type != NULL)) {
				if(strcmp(params->name,"src-dir") == 0) {
					strcpy(cfg->profile_file_type->src_dir,params->value);
				}
			
				if(strcmp(params->name,"dst-dir") == 0) {
					strcpy(cfg->profile_file_type->dst_dir,params->value);
				}
			
				if(strcmp(params->name,"cols-separator") == 0) {
					strcpy(cfg->profile_file_type->col_sep,params->value);
				}
				
				if(strcmp(params->name,"col-delimiter") == 0) {
					strcpy(cfg->profile_file_type->col_delimiter,params->value);
				}
			
				if(strcmp(params->name,"line-end") == 0) {
					strcpy(cfg->profile_file_type->line_end,params->value);
				}
				
				if(strcmp(params->name,"file-field-num") == 0) {
					cfg->profile_file_type->file_field_num = atoi(params->value);
				}
			}
			
			if((cfg->t == db)&&(cfg->profile_db_type != NULL)) {
				if(strcmp(params->name,"dbhost") == 0) {
					strcpy(cfg->profile_db_type->dbhost,params->value);
				}
			
				if(strcmp(params->name,"dbuser") == 0) {
					strcpy(cfg->profile_db_type->dbuser,params->value);
				}
			
				if(strcmp(params->name,"dbpass") == 0) {
					strcpy(cfg->profile_db_type->dbpass,params->value);
				}
				
				if(strcmp(params->name,"dbname") == 0) {
					strcpy(cfg->profile_db_type->dbname,params->value);
				}
				
				if(strcmp(params->name,"dbtype") == 0) {
					if(strcmp(params->name,DB_TYPE_PGSQL)  == 0) cfg->profile_db_type->t = pgsql;
					if(strcmp(params->name,DB_TYPE_MYSQL)  == 0) cfg->profile_db_type->t = mysql;
					if(strcmp(params->name,DB_TYPE_ORACLE) == 0) cfg->profile_db_type->t = oracle;
				}
				
				if(strcmp(params->name,"cdr-table") == 0) {
					strcpy(cfg->profile_db_type->cdr_table,params->value);
				}
				
				if(strcmp(params->name,"sql-col-where") == 0) {
					strcpy(cfg->profile_db_type->sql_col_where,params->value);
				}
				
				if(strcmp(params->name,"sql-where-const") == 0) {
					strcpy(cfg->profile_db_type->sql_where_const,params->value);
				}
				
				if(strcmp(params->name,"SchedTS") == 0) {
					strcpy(cfg->profile_db_type->cdr_sched_ts,params->value);
				}
			}
			
			if(strcmp(params->name,"cdr-rec-type") == 0) {
				if(strcmp(params->value,CDR_REC_TYPE_UNKN) == 0) cfg->t2 = unkn;
				if(strcmp(params->value,CDR_REC_TYPE_ISUP) == 0) cfg->t2 = isup;
				if(strcmp(params->value,CDR_REC_TYPE_SMS) == 0) cfg->t2 = sms;
				if(strcmp(params->value,CDR_REC_TYPE_VA) == 0) cfg->t2 = voip_a;
				if(strcmp(params->value,CDR_REC_TYPE_VV) == 0) cfg->t2 = voip_v;
				if(strcmp(params->value,CDR_REC_TYPE_VT) == 0) cfg->t2 = voip_t;
			}
			
			params = params->next_param;
		}
	
		xml_cfg_params_free(cfg->node->params);
	
		/* CDRFormat section from a profile xml file */	
		if((cfg->t) == file) {			
			cdr_cfg_file_profile_xml(cfg);
		} else if(cfg->t == db) {
			cdr_cfg_storage_profile_xml(cfg);
		} else {
			/* ??? */
		}
		
		mem_free(cfg->node);
	}
}

void cdr_cfg_file_profile_xml(cdr_profile_cfg_t *cfg)
{
	int i;
	int num;
	int hdr_i;
	xml_param_t *params;
	cdr_file_header_t *hdr;

	if(cfg != NULL) {
		strcpy(cfg->node->node_name,"CDRFormat");
	
		xml_cfg_params_get(cfg->root,cfg->node);

		params = cfg->node->params;
		
		num = cdr_profile_params_num(params);

		if(num > 0) {
			num = num + 1;
		
			if(cfg->profile_file_type != NULL) {
				cfg->profile_file_type->hdr = cdr_file_header_init(num);
				cfg->profile_file_type->hdr_num = num;
				
				hdr_i = 0;
				hdr = cfg->profile_file_type->hdr;
			
				params = cfg->node->params;
	
				while(params != NULL) {
					for(i=0;i<CDR_TBL_NUM;i++) {
					//for(i=0;i<=CDR_TBL_NUM;i++) { ???!!!
						if((strcmp(params->name,cdr_tbl_cpy_ptr[i].name) == 0)&&(strcmp(params->value,""))) {
							
							//cdr_file_header_put(&hdr[hdr_i],&cdr_tbl_cpy_ptr[i],params->value,(hdr_i+1));
							cdr_file_header_put(&hdr[hdr_i],&cdr_tbl_cpy_ptr[i],params->value,atoi(params->value));
							hdr_i++;
							break;
						}					
					}
					params = params->next_param;
				} 
			}
		
			xml_cfg_params_free(cfg->node->params);
		}
	}
}

void cdr_cfg_storage_profile_xml(cdr_profile_cfg_t *cfg)
{
	int i;
	int num;
	int cols_i;
	xml_param_t *params;
		
	cdr_storage_col_t *cols;

	if(cfg != NULL) {
		strcpy(cfg->node->node_name,"CDRFormat");
	
		xml_cfg_params_get(cfg->root,cfg->node);

		params = cfg->node->params;
		
		num = cdr_profile_params_num(params);

		if(num > 0) {
			num = num + 1;
		
			if(cfg->profile_db_type != NULL) {
				cfg->profile_db_type->cols = cdr_storage_col_init(num);
				cfg->profile_db_type->cols_num = num;
				
				cols_i = 0;
				cols = cfg->profile_db_type->cols;
			
				params = cfg->node->params;
	
				while(params != NULL) {
					for(i=0;i<CDR_TBL_NUM;i++) {
					//for(i=0;i<=CDR_TBL_NUM;i++) { ???!!!
						if((strcmp(params->name,cdr_tbl_cpy_ptr[i].name) == 0)&&(strcmp(params->value,""))) {
							
							cdr_storage_col_put(&cols[cols_i],&cdr_tbl_cpy_ptr[i],params->value,(cols_i+1));
							cols_i++;
							break;
						}					
					}
					params = params->next_param;
				} 
			}
		
			xml_cfg_params_free(cfg->node->params);
		}
	}
}

int cdr_cfg_get_cdr_profile_id(PGconn *conn,char *profile_name)
{
	int num;
	int fnum;
	PGresult *res;
	char str[512];
	int profile_id;
	
	profile_id = 0;
	
	bzero(str,sizeof(str));
	sprintf(str,"select id from cdr_profiles where profile_name = '%s';",profile_name);
	
	res = db_pgsql_exec(conn,str);
	if(res != NULL) {
		num = PQntuples(res);
		
		if(num) {
			fnum = PQfnumber(res, "id");
			profile_id = atoi(PQgetvalue(res,0, fnum));
		}
		
		PQclear(res);
	}

	return profile_id;
}

int cdr_cfg_insert_profile_pgsql(PGconn *conn,cdr_profile_cfg_t *cfg)
{
	PGresult *res;
    char str[512];

	bzero(str,sizeof(str));

    sprintf(str,
			"insert into cdr_profiles (profile_name,profile_version) values ('%s',%d)",
			cfg->profile_name,cfg->profile_version);

	res = db_pgsql_exec(conn,str);
	if(res == NULL) return 0;
    
    PQclear(res);
    
    return cdr_cfg_get_cdr_profile_id(conn,cfg->profile_name);
}

int cdr_cfg_get_cdr_server_id(PGconn *conn,int cdr_profile_id)
{
	int num;
	int fnum;
	PGresult *res;
	char str[512];
	int cdr_server_id;
	
	cdr_server_id = 0;
	
	bzero(str,sizeof(str));
	sprintf(str,"select id from cdr_servers where cdr_profiles_id = %d;",cdr_profile_id);
	
	res = db_pgsql_exec(conn,str);
	if(res != NULL) {
		num = PQntuples(res);
    
		if(num) {
			fnum = PQfnumber(res, "id");	
			cdr_server_id = atoi(PQgetvalue(res,0, fnum));
		}		
		
		PQclear(res);
	}
	
	return cdr_server_id;
}

int cdr_cfg_insert_server_pgsql(PGconn *conn,cdr_profile_cfg_t *profile)
{
	PGresult *res;
    char str[512];

	bzero(str,sizeof(str));
    sprintf(str,"insert into cdr_servers "
			    "(cdr_profiles_id,get_mode_id,active) "
			    "values (%d,%d,'t')",
			    profile->cdr_profile_id,(profile->t + 1));

	res = db_pgsql_exec(conn,str);
	if(res == NULL) return 0;
    
    PQclear(res);
    
    return cdr_cfg_get_cdr_server_id(conn,profile->cdr_profile_id);
}

void cdr_cfg_profile_insert(PGconn *conn,cdr_profile_cfg_t *profile)
{
	profile->cdr_profile_id = cdr_cfg_get_cdr_profile_id(conn,profile->profile_name);
						
	if(profile->cdr_profile_id == 0) {
		profile->cdr_profile_id = cdr_cfg_insert_profile_pgsql(conn,profile);
		profile->cdr_server_id  = cdr_cfg_insert_server_pgsql(conn,profile);
	} else {
		profile->cdr_server_id = cdr_cfg_get_cdr_server_id(conn,profile->cdr_profile_id);
							
		if(profile->cdr_server_id == 0) {
			profile->cdr_server_id = cdr_cfg_insert_server_pgsql(conn,profile);
		}
	}
}

cdr_cfg_t *cdr_cfg_main(char *xmlFileName)
{
	int i;
	cdr_cfg_t *cfg;
	cdr_profile_cfg_t *profile;
	
	cfg = cdr_cfg_init();

	if(cfg != NULL) {
		cfg->doc = xml_cfg_doc(xmlFileName);
	
		if(cfg->doc != NULL) {
			cfg->root = xml_cfg_root(cfg->doc);
	
			if(cfg->root != NULL) {
				cdr_cfg_get(cfg);
				
				if(strcmp(cfg->cdr_profiles_dir,"") == 0) {
					re_write_syslog_2(config.log,"cdr_cfg_main()","A 'CDRProfilesDIR' is empty");
					mem_free(cfg);
					cfg = NULL;
				} else { 
					cdr_profiles_list_get(cfg);
				
					if(cfg->list != NULL) {
						cfg->profiles = cdr_profile_cfg_init(cfg->profiles_number);
					
						for(i=0;i<(cfg->profiles_number);i++) {						
							profile = &cfg->profiles[i];
							
							if(profile != NULL) {
								profile->doc = xml_cfg_doc(cfg->list[i].filename);
								
								if(profile->doc != NULL) {
									profile->root = xml_cfg_root(profile->doc);
									
									if(profile->root != NULL) {
										cdr_profile_cfg_get(profile);
										cdr_cfg_profile_insert(config.conn,profile);
									}
								}
							
								xml_cfg_free_doc(profile->doc);
							} else {
								re_write_syslog_2(config.log,"cdr_cfg_main()","A 'profile' pointer is null!");
								xml_cfg_free_doc(cfg->doc);
								mem_free(cfg);
								cfg = NULL;
							}
						}
					} else {
						re_write_syslog_2(config.log,"cdr_cfg_main()","A 'list' pointer is null!");
						xml_cfg_free_doc(cfg->doc);
						mem_free(cfg);
						cfg = NULL;
					}
				}
			} else {
				re_write_syslog_2(config.log,"cdr_cfg_main()","A main 'root' pointer is null!");
				xml_cfg_free_doc(cfg->doc);
				mem_free(cfg);
				cfg = NULL;
			}
		
			if(cfg != NULL) {
				xml_cfg_free_doc(cfg->doc);
				mem_free(cfg->node);
			}
		}
	}
	
	return cfg;
}
