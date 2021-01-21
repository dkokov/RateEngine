#include <string.h>

#include "globals.h"
#include "../DB/db_pgsql.h"

int chk_db_version(char *version)
{
	PGresult *res;
	char chk_version[255];
	char *col[] = {"release",""};
	int *fnum;
	
	bzero(chk_version,sizeof(chk_version));
	
	res = db_pgsql_exec(config.conn,"select release from version");
	
	fnum = db_pgsql_fnum(res,col);
	if(fnum == 0) 
	{
		PQclear(res);
		return 1;
	}
	
	strcpy(chk_version,(PQgetvalue(res,0, fnum[0])));
	
	db_pgsql_fnum_free(fnum);
	PQclear(res);
	
	if(!strcmp(chk_version,version)) return 0;
	else return 1;
} 
