#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "prefix_filter.h"

filter *prefix_filter_init(int num)
{
	return (filter *)mem_alloc_arr((num+1),sizeof(filter));
}

filter *prefix_filter_get(db_t *dbp,int cdr_profile_id)
{
    int  i;
    char str[SQL_BUF_LEN];
    
	filter *filters;
	db_sql_result_t *result;

	if(dbp == NULL) return NULL;

	filters = NULL;

	if(dbp->t == sql) {
		bzero(str,sizeof(str));
		
		sprintf(str,"SELECT id,filtering_prefix,filtering_number,replace_str,cdr_server_id,len "
					"FROM prefix_filter where cdr_profiles_id = %d",cdr_profile_id);
	
		db_select(dbp,str);
		db_fetch(dbp);
		
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
		
			if(result->rows > 0) {
				filters = prefix_filter_init(result->rows);
			
				if(filters != NULL) {
					for(i = 0; i < result->rows; i++) {
						filters[i].id = atoi(result->cols_list[0].rows_list[i].row);
						strcpy(filters[i].filtering_prefix,result->cols_list[1].rows_list[i].row);
						filters[i].filtering_number = atoi(result->cols_list[2].rows_list[i].row);
						strcpy(filters[i].replace_str,result->cols_list[3].rows_list[i].row);
						filters[i].cdr_server_id = atoi(result->cols_list[4].rows_list[i].row);
						filters[i].len = atoi(result->cols_list[5].rows_list[i].row);
					}
				
					filters[i].id = 0;
				}
			}
		
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
	}
	
	return filters;
}

void prefix_filter_cuti_replace(char *prefix,char *cut_string,int i,char *replace_string,char *ret,int prefix_len)
{
    int len,p,c;
    char buf[32];
    char replace_str[80];
    
    bzero(replace_str,80);
    
    len = 0;
    
    len = strlen(cut_string);
    
    strncpy(buf,prefix,len);
    
    buf[len] = '\0';
    
    if(prefix_len > 0)
    {
		if(strlen(prefix) == prefix_len) goto cont;
		else goto end_rep;
	}
    
    cont:
    if(!strcmp(buf,cut_string))
    {
		c=0;
		for(p=i;p<(strlen(prefix));p++)
		{
			ret[c] = prefix[p];
			c++;
		}
		ret[c]='\0';
	
		if(strcmp(replace_string,"")) 
		{
			strcpy(replace_str,replace_string);
			strcat(replace_str,ret);
			strcpy(ret,replace_str);
		}
    }
    else
    {
		end_rep:
		ret[0]='\0';
    }
} 
