#include "../misc/globals.h"
#include "../misc/mem/mem.h"
#include "../DB/db_pgsql.h"

#include "prefix_filter.h"

filter *prefix_filter_init(int num)
{
	filter *pt = NULL;
	
	pt = (filter *)mem_alloc((sizeof(filter)*(num + 1)));
		
	return pt;
}

filter *prefix_filter_get(PGconn *conn,int cdr_profile_id)
{
    int  i,num;
    int fnum[7];
    char str[512];

    PGresult *res;

	filter *filters = NULL;

	bzero(str,sizeof(str));
		
	sprintf(str,"SELECT id,filtering_prefix,filtering_number,replace_str,cdr_server_id,len "
				"FROM prefix_filter where cdr_profiles_id = %d",cdr_profile_id);
	res = db_pgsql_exec(conn,str);
		
	if(res) {
		num = PQntuples(res);
		
		if(num > 0) {
			filters = prefix_filter_init(num);
			
			if(filters != NULL) {
				fnum[0] = PQfnumber(res, "id");
				fnum[1] = PQfnumber(res, "filtering_prefix");
				fnum[2] = PQfnumber(res, "filtering_number");
				fnum[3] = PQfnumber(res, "replace_str");
				fnum[4] = PQfnumber(res, "cdr_server_id");
				fnum[5] = PQfnumber(res, "len");

				for(i = 0; i < num; i++) {
					filters[i].id = atoi(PQgetvalue(res, i, fnum[0]));
					strcpy(filters[i].filtering_prefix,PQgetvalue(res, i, fnum[1]));
					filters[i].filtering_number = atoi(PQgetvalue(res, i, fnum[2]));
					strcpy(filters[i].replace_str,PQgetvalue(res, i, fnum[3]));
					filters[i].cdr_server_id = atoi(PQgetvalue(res, i, fnum[4]));
					filters[i].len = atoi(PQgetvalue(res, i, fnum[5]));
				}
		
				filters[i].id = 0;
			}
		}
		
		PQclear(res);
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
