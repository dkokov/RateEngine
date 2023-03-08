#include <dirent.h> 

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "prefix_filter.h"
#include "cdr.h"
#include "cdr_storage.h"
#include "cdr_file.h"
#include "cdr_cfg.h"
#include "cdrm_json.h"

cdr_cfg_t *cdr_cfg_init(void)
{	
	return (cdr_cfg_t *)mem_alloc(sizeof(cdr_cfg_t));
}

void cdr_cfg_view(cdr_cfg_t *cfg)
{

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
				
				if(strcmp(params->name,"dbport") == 0) {
					cfg->profile_db_type->dbport = atoi(params->value);
				}				
				
				if(strcmp(params->name,"dbtype") == 0) {
					strcpy(cfg->profile_db_type->dbtype,params->value);
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
		
		/* PrefixFiltering */
		cdr_cfg_profile_filters_xml(cfg);
		
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

void cdr_cfg_profile_filters_xml(cdr_profile_cfg_t *cfg)
{
	int i;
	xml_node_t *child,*child2;
	xml_param_t *params;
	
	if(cfg != NULL) {
		strcpy(cfg->node->node_name,"PrefixFiltering");		
		strcpy(cfg->node->child_node_name,"filter");
		
		xml_cfg_child_get(cfg->root,cfg->node);

		if(cfg->node->child_num == 0) return;

		cfg->filters = prefix_filter_init(cfg->node->child_num);
		
		child2 = cfg->node->child_node;

		i = 0;
		child  = child2;
		while(child != NULL) {
			if(child->params != NULL) {
				params = child->params;
			
				while(params != NULL) {					
					if(strcmp(params->name,"prefix") == 0) {
						cfg->filters[i].id = i+1;
						strcpy(cfg->filters[i].filtering_prefix,params->value);
					}
					
					if(strcmp(params->name,"num") == 0) {
						cfg->filters[i].filtering_number = atoi(params->value);
					}					
					
					if(strcmp(params->name,"replace") == 0) {
						strcpy(cfg->filters[i].replace_str,params->value);
					}
					
					if(strcmp(params->name,"len") == 0) {
						cfg->filters[i].len = atoi(params->value);
					}
																
					params = params->next_param;
				}
				
				i++;
			}

			child = child->next_node;
		}

		xml_cfg_child_free(child2);
	}	
}

int cdr_cfg_get_cdr_profile_filters(db_t *dbp,int cdr_profile_id)
{
	int num;
	char str[512];
	
	db_sql_result_t *result;
	
	if(dbp == NULL) return -1;
	if(cdr_profile_id <= 0) return -2;
	
	if(dbp->t == sql) {
		num = 0;
		bzero(str,sizeof(str));
		sprintf(str,"select count(*) from prefix_filter where cdr_profiles_id = %d;",cdr_profile_id);

		if(db_select(dbp,str) < 0) return -3;
		
		db_fetch(dbp);
		
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			if(result->rows == 1) num = atoi(result->cols_list[0].rows_list[0].row);
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}		
	} else return -4;

	return num;
}

int cdr_cfg_get_cdr_profile_id(db_t *dbp,char *profile_name)
{
	int ret;
	char str[512];
	int profile_id;
	
	db_sql_result_t *result;
	db_nosql_result_t *result2;
	
	if(dbp == NULL) return -1;
	
	profile_id = 0;

	if(dbp->t == sql) {
		bzero(str,sizeof(str));
		sprintf(str,"select id from cdr_profiles where profile_name = '%s';",profile_name);
	
		db_select(dbp,str);
		db_fetch(dbp);
		
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			if(result->rows == 1) profile_id = atoi(result->cols_list[0].rows_list[0].row);
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;	
		}		
	} else if(dbp->t == nosql) {
		bzero(str,sizeof(str));
		sprintf(str,"get %s%s",CDR_JSON_CDR_PROFILE_HDR_PATTERN,profile_name);
	
		ret = db_get(dbp,str);
		if(ret < 0) {
			if(ret == DB_ERR_NOSQL_RES_NUL) return 0;
			
			db_error(ret);
			return -3;
		} else {
			result2 = (db_nosql_result_t *)dbp->conn->result;
				
//			printf("\ncfg%s %ld\n",result2->str,strlen(result2->str));
				
			db_nosql_result_free(result2);
			dbp->conn->result = NULL;
			
			db_free_result(dbp);

			return 1;
		}
	} else return -2;

	return profile_id;
}

int cdr_cfg_insert_profile(cdr_profile_cfg_t *cfg)
{
    int ret;
    char str[SQL_BUF_LEN];
    
	cdrm_json_t cdr_j;

	if(cfg->dbp == NULL) return -1;

	if(cfg->dbp->t == sql) {
		bzero(str,sizeof(str));

		sprintf(str,"insert into cdr_profiles (profile_name,profile_version) values ('%s',%d)",cfg->profile_name,cfg->profile_version);

		db_insert(cfg->dbp,str);
		
		return cdr_cfg_get_cdr_profile_id(cfg->dbp,cfg->profile_name);
	} else if(cfg->dbp->t == nosql) {
		bzero(str,sizeof(str));

		memset(&cdr_j,0,sizeof(cdrm_json_t));
		cdrm_profile_struct_json(cfg,&cdr_j);
	
		sprintf(str,"set %s %s",cdr_j.header,cdr_j.msg);
		mem_free(cdr_j.msg);
		
		ret = db_set(cfg->dbp,str,NULL);
		if(ret < 0) {
			db_error(ret);
		} else return 1;
	} else return -2;
    
    return 0;
}

int cdr_cfg_get_cdr_server_id(db_t *dbp,int cdr_profile_id)
{
	int cdr_server_id;
	char str[512];
	
	db_sql_result_t *result;
	
	if(dbp == NULL) return -1;
	
	cdr_server_id = 0;
	
	if(dbp->t == sql) {
		bzero(str,sizeof(str));
		sprintf(str,"select id from cdr_servers where cdr_profiles_id = %d;",cdr_profile_id);
	
		db_select(dbp,str);
		db_fetch(dbp);
		
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
			if(result->rows == 1) cdr_server_id = atoi(result->cols_list[0].rows_list[0].row);
			
			db_sql_result_free(result);
			dbp->conn->result = NULL;		
		}		
	}
	
	return cdr_server_id;
}

int cdr_cfg_insert_server(cdr_profile_cfg_t *profile)
{
	int ret;
    char str[SQL_BUF_LEN];

	if(profile->dbp == NULL) return DB_ERR_DBP_NUL;

	if(profile->dbp->t == sql) {
		bzero(str,SQL_BUF_LEN);
		
		sprintf(str,"insert into cdr_servers "
					"(cdr_profiles_id,get_mode_id,active) "
					"values (%d,%d,'t')",
					profile->cdr_profile_id,(profile->t + 1));

		ret = db_insert(profile->dbp,str);
		if(ret < 0) db_error(ret);
		
		ret = cdr_cfg_get_cdr_server_id(profile->dbp,profile->cdr_profile_id);
	} else if(profile->dbp->t == nosql) {
		bzero(str,SQL_BUF_LEN);
		
		sprintf(str,"SET cdr_server_%s 'true'",profile->profile_name);
		
		ret = db_set(profile->dbp,str,NULL);
		if(ret < 0) db_error(ret);
	} else return DB_ERR_TYPE_UNK;
    
    return ret;
}

int cdr_cfg_insert_filters(cdr_profile_cfg_t *profile)
{
	int i;
	char str[SQL_BUF_LEN];
		
	if(profile->dbp == NULL) return -1;
	if(profile->filters == NULL) return -2;
	
	if(profile->dbp->t == sql) {
		i=0;
		while(strcmp(profile->filters[i].filtering_prefix,"")) {
			bzero(str,sizeof(str));
		
			sprintf(str,"insert into prefix_filter (cdr_profiles_id,cdr_server_id,filtering_prefix,filtering_number,replace_str,len) values (%d,%d,'%s',%d,'%s',%d)",
					profile->cdr_profile_id,profile->cdr_server_id,profile->filters[i].filtering_prefix,profile->filters[i].filtering_number,
					profile->filters[i].replace_str,profile->filters[i].len);

			if(db_insert(profile->dbp,str) < 0) return -4;
						
			i++;
		}
	} else if(profile->dbp->t == nosql) {
		 
	}else return -3;	
	
	return 0;
}

void cdr_cfg_profile_insert(cdr_profile_cfg_t *profile)
{
	profile->cdr_profile_id = cdr_cfg_get_cdr_profile_id(profile->dbp,profile->profile_name);
						
	if(profile->cdr_profile_id == 0) {
		profile->cdr_profile_id = cdr_cfg_insert_profile(profile);
		profile->cdr_server_id  = cdr_cfg_insert_server(profile);		
	} else {
		profile->cdr_server_id = cdr_cfg_get_cdr_server_id(profile->dbp,profile->cdr_profile_id);
							
		if(profile->cdr_server_id == 0) {
			profile->cdr_server_id = cdr_cfg_insert_server(profile);
		}
	}
	
	if((profile->cdr_profile_id > 0)&&(profile->filters != NULL)) {
		if(cdr_cfg_get_cdr_profile_filters(profile->dbp,profile->cdr_profile_id) == 0) { 
			cdr_cfg_insert_filters(profile);
		}
	
		if(profile->dbp->t == sql) mem_free(profile->filters);
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
					LOG("cdr_cfg_main()","A 'CDRProfilesDIR' is empty");
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
								strcpy(profile->filename,cfg->list[i].filename); 

								if(profile->doc != NULL) {
									profile->root = xml_cfg_root(profile->doc);
									
									if(profile->root != NULL) {
										cdr_profile_cfg_get(profile);
									}
								}
							
								xml_cfg_free_doc(profile->doc);
							} else {
								LOG("cdr_cfg_main()","A 'profile' pointer is null!");
								xml_cfg_free_doc(cfg->doc);
								mem_free(cfg);
								cfg = NULL;
							}
						}
					} else {
						LOG("cdr_cfg_main()","A 'list' pointer is null!");
						xml_cfg_free_doc(cfg->doc);
						mem_free(cfg->node);
						mem_free(cfg);
						cfg = NULL;
					}
				}
			} else {
				LOG("cdr_cfg_main()","A main 'root' pointer is null!");
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
