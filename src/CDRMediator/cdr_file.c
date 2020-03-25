#include "../misc/globals.h"
#include "../misc/mem/mem.h"
#include <dirent.h> 
#include "cdr_file.h"

/* checking whether a readbuf is overfilled */
int cdr_file_chk_readbuf(char *readbuf)
{
	int len;
	
	len = strlen(readbuf);
	
	if(len == READBUF_LEN) {
		if(readbuf[READBUF_LEN] != '\0') return 1;
	}
	
	return 0;
}

void cdr_file_getfield_cr_fix(char *row,char *end)
{
	char *tmp;
	char buf[256];
	
	tmp = strtok(row,"\n");
	if(tmp != NULL) {
		bzero(buf,256);
		strcpy(buf,tmp);
					
		bzero(row,strlen(row));
		strcpy(row,buf);
	}
	//printf("\ndebug: %s",row);
}

void cdr_file_getfield_delimiter_fix(cdr_file_field_t *arr,char *del)
{
	int c;
	char *tmp;
	char buf[256];
	char first,last,del2;
	
	c = 0;
	while((arr[c].id)>0) {
		tmp = arr[c].txt;
		
		first = tmp[0];
		last = tmp[(strlen(tmp))-1];
		
		if(strlen(del) == 1) del2 = *(del);
	
		/* case 1: */
		if((first == last)&&(first == del2)) {
			tmp = strtok(tmp,del);
			if(tmp != NULL) {
				bzero(buf,256);
				strcpy(buf,tmp);
					
				bzero(arr[c].txt,sizeof(arr[c].txt));
				strcpy(arr[c].txt,buf);
			}
		}
		//printf("\ndebug[%d]:%s",c,arr[c].txt);
			
		c++;
	}
}

/* Get fields from a line(row) and put in the struct array */
void cdr_file_getfield(cdr_file_field_t *arr,char *line,char *sep,char *del,char *end)
{
    int i;
    char *tok;
    
	i = 0;
    for(tok = strtok(line,sep);tok && *tok;tok = strtok(NULL,sep)) {
        arr[i].id = (i+1);
        strcpy(arr[i].txt,tok);
        
        i++;
    }
    
	arr[i].id = 0;
    strcpy(arr[i].txt,"");
    
    cdr_file_getfield_cr_fix(arr[(i-1)].txt,end);
    
	cdr_file_getfield_delimiter_fix(arr,del);
}

/* Init the struct array */
void cdr_file_field_init_arr(cdr_file_field_t *pt,size_t mem)
{
	memset(pt,0,mem);
}

/* Create the struct array */
cdr_file_field_t *cdr_file_field_create_arr(size_t mem)
{
	cdr_file_field_t *arr;
		
	arr = (cdr_file_field_t *)mem_alloc(mem);
	//if(arr != NULL) cdr_file_field_init_arr(arr,mem);
	
	return arr;
}

/* Clear memory from the struct array */
void cdr_file_field_free_arr(cdr_file_field_t *pt)
{
	if(pt != NULL) mem_free(pt);
}

/* CDR file profile init,put,get,delete */
cdr_file_header_t *cdr_file_header_init(int num)
{
	size_t mem;
	cdr_file_header_t *hdr = NULL;
	
	if(num > 0) mem = sizeof(cdr_file_header_t)*(num + 1);
	
	hdr = (cdr_file_header_t *)mem_alloc(mem);
	
	return hdr;
}

cdr_file_profile_t *cdr_file_profile_init(void)
{
	cdr_file_profile_t *profile = NULL;
	
	profile = (cdr_file_profile_t *)mem_alloc(sizeof(cdr_file_profile_t));
	
	return profile;
}

void cdr_file_header_put(cdr_file_header_t *hdr,cdr_table_t *tbl,char *hdr_name,int hdr_id) 
{
	if((tbl != NULL)&&(hdr != NULL)) {
		hdr->func = tbl->func;
		strcpy(hdr->hdr_name,hdr_name);
		hdr->hdr_id = hdr_id;
	}
}

void cdr_file_profile_free(cdr_file_profile_t *profile)
{
	if(profile != NULL) mem_free(profile);
}

cdr_file_list_t *cdr_file_list_init(int n)
{
	cdr_file_list_t *list;
	
	list = (cdr_file_list_t *)mem_alloc((sizeof(cdr_file_list_t)*n));

	return list;
}

void cdr_file_list_get(cdr_file_profile_t *cfg)
{
	int i;
	DIR  *d;
	struct dirent *dir;

	i = 0;

	d = opendir(cfg->src_dir);
	
	if(d) {
		while ((dir = readdir(d)) != NULL) {
			i++;
		}

		if(i > 2) {
			seekdir(d,0);
		
			/* without '.' and '..' */
			cfg->files_number = (i - 2);

			cfg->list = cdr_file_list_init(cfg->files_number);
		
			i=0;
			while ((dir = readdir(d)) != NULL) {
				if((strcmp(dir->d_name,".") == 0)||(strcmp(dir->d_name,"..") == 0)) {
				
				} else {
					strcpy(cfg->list[i].filename,dir->d_name);
					i++;
				}
			}
		}
		
		closedir(d);
	}
}

/* CDR file field analize */
int cdr_file_field_analize(char *line,char *sep)
{
	int i;
    char *tok,*tmp;
    
	i = 0;
	
	if(line != NULL) {
		tmp = strdup(line);

		if(tmp != NULL) {
			for(tok = strtok(tmp,sep);tok && *tok;tok = strtok(NULL,sep)) i++;
    
			mem_free(tmp);
		}
    }
    
    return i;
}

/* Add a filename in the table 'cdr_files' in db  */
void cdr_file_add_in_tbl(PGconn *conn,char *filename,int cdr_server_id)
{
	char str[512];
	
	bzero(str,sizeof(str));
	sprintf(str,"insert into cdr_files (cdr_server_id,filename,last_update) "
				"values (%d,'%s','now()')",cdr_server_id,filename);

	db_pgsql_exec(conn,str);
}

/* Get id from a 'cdr_files' table */
int cdr_file_get_from_tbl(PGconn *conn,char *filename,int cdr_server_id)
{
	int id,num,fnum ;
	char str[512];
	PGresult *res;
	
	id = 0; num = 0 ; 
	
	bzero(str,sizeof(str));
	sprintf(str,"select id from cdr_files where cdr_server_id = %d and filename = '%s'",
			cdr_server_id,filename);
	res = db_pgsql_exec(conn,str);
	
	if(res != NULL) {
		num = PQntuples(res);
    
		if(num) {
			fnum = PQfnumber(res, "id");	
			id = atoi(PQgetvalue(res,0, fnum));
		}		
		
		PQclear(res);
	}
	
	return id;
}

/* 
 * Open CDR file(csv format),
 * Get data row by row,
 * Parse this data,
 * Insert in the DB row by row 
 * ;
 * 'col_sep' is column separator in the row,
 * 'hdr_comm' is header comment character when has (';','//','#',etc),
 * 'hdr_sep' is header separator in the header row
 * 
 */
int cdr_file_parser(cdr_file_profile_t *profile,char *filename) 
{
	int all_rows,parse_rows,ins_rows;
	int file_field_num;
	size_t mem;
	FILE *fp;
	char readbuf[READBUF_LEN];
	
	cdr_file_field_t *arr;
	cdr_file_header_t *hdr;
		
	if(profile == NULL) {
		re_write_syslog_2(config.log,"cdr_file_parser()","A 'profile' pointer is null!");
		return 1;
	}
	
	arr = NULL;
	hdr = profile->hdr;
	
	/* Open a CDR file */
	fp = fopen(filename,"r");
	
	if(fp == NULL) {
		re_write_syslog_2(config.log,"cdr_file_parser()","A CDR file '%s' is not opened successful!",filename);
		return 2;
	}
	
	/* All get from a file rows */
	all_rows = 0; 
	
	/* Parse rows */
	parse_rows = 0;
	
	/* Inserted rows */
	ins_rows = 0;
	
	while(!feof(fp)) {
		/* Get a row from a file and put in the READBUF */
		bzero(readbuf,READBUF_LEN);
		fgets(readbuf,READBUF_LEN,fp);

		file_field_num = 0;

		if((readbuf[0]) == (profile->hdr_comm)) {
			/* READBUF is the headers line */
		} else {
			/* READBUF is not the headers line */
			file_field_num = cdr_file_field_analize(readbuf,profile->col_sep);

			if((file_field_num > 0)&&(profile->file_field_num == file_field_num)) {
				mem = ((sizeof(cdr_file_field_t)*(profile->file_field_num))+1);

				arr = cdr_file_field_create_arr(mem);
					
				if(arr == NULL) {
					re_write_syslog_2(config.log,"cdr_file_parser()","An 'arr' pointer is null!");
					return 3;			
				}
			} else {
				continue;
			}
			
			all_rows++;
			
			if(arr != NULL) {
				cdr_file_getfield(arr,readbuf,profile->col_sep,profile->col_delimiter,profile->line_end);			

				/* insert data from array in the cdr struct */
				int i,c;
				cdr_t the_cdr;
				
				memset(&the_cdr,0,sizeof(cdr_t));
			
				the_cdr.cdr_server_id = profile->cdr_server_id;
				the_cdr.cdr_rec_type_id = profile->cdr_rec_type_id;
			
				for(i=0;i<profile->file_field_num;i++) {
					for(c=0;c<profile->hdr_num;c++) {
						if(i == hdr[c].hdr_id) {
							if(hdr[c].func != NULL) {
								(*hdr[c].func)(&the_cdr,arr[i].txt);
							}
						}
					}
				}
						
				if((strcmp(the_cdr.call_uid,""))&&(strcmp(the_cdr.called_number,""))&&
					(the_cdr.cdr_server_id > 0)&&(the_cdr.billsec > 0)) {
					/* It's correct parse row */
					parse_rows++;
					
					/* Insert the CDR struct in the DB */
					if(cdr_add_in_db(profile->conn,&the_cdr,profile->filters)) ins_rows++;
				} else {
					/* It's not need minimum data for DB inserting */
				}
				
				file_field_num = 0;
				cdr_file_field_free_arr(arr);
			} else {
				re_write_syslog_2(config.log,
								  "cdr_file_parser()","Error!Cannot parse a line from a CDR file '%s'!arr is null!",
								  filename);
			}
		}
	}
	
	re_write_syslog_2(config.log,"cdr_file_parser()","all rows: %d,parse rows: %d(%.2f%),inserted rows: %d(%.2f%)",
						all_rows,parse_rows,(((float)parse_rows/(float)all_rows)*100),
						ins_rows,(((float)ins_rows/(float)parse_rows)*100));
	
	fclose(fp);
	
	cdr_file_add_in_tbl(profile->conn,filename,profile->cdr_server_id);
					
	re_write_syslog_2(config.log,
					  "cdr_file_parser()","The CDR file '%s' is inserted succesful!",
					  filename);
	return 0;
}

/* 
 * Get filenames from a dir,
 * Call parser,
 * Move files to other dir,
 * Mark files ???
 * 
 */
void cdr_file_reader(cdr_file_profile_t *profile)
{
	int i;
	char src[256];
	char dst[256];
	
	cdr_file_list_get(profile);
	
	re_write_syslog_2(config.log,"cdr_file_reader()","src-dir[%s] : %d files",
					  profile->src_dir,profile->files_number);
	
	if(profile->list != NULL) {
		for(i=0;i<profile->files_number;i++) {
			if(strcmp((profile->list[i].filename),"") == 0) {
				re_write_syslog_2(config.log,"cdr_file_reader()","filename is empty");
				continue;
			}
			
			bzero(src,256);				
			sprintf(src,"%s%s",profile->src_dir,profile->list[i].filename);
				
			bzero(dst,256);
			sprintf(dst,"%s%s",profile->dst_dir,profile->list[i].filename);

			if((cdr_file_get_from_tbl(profile->conn,src,profile->cdr_server_id)) == 0) {							
				re_write_syslog_2(config.log,"cdr_file_reader()","Read & parse a cdr file '%s'",src);

				if(cdr_file_parser(profile,src) == 0) {
					cdr_file_move(src,dst);
				}
			} else {
				re_write_syslog_2(config.log,"cdr_file_reader()","The file '%s' is already inserted in the DB!",src);
				
				cdr_file_move(src,dst);
			}
		}
		
		profile->files_number = 0;
		
		//free(profile->list);
	} else {
		re_write_syslog_2(config.log,"cdr_file_reader()","The src_dir '%s' is empty!",profile->src_dir);
	}
}

/* 
 * Remote access (ftp,sftp,ssh),
 * Get remote files and save in a dir,
 * 
 */
void cdr_file_remote(void)
{
	//http://www.libssh2.org/examples/
}

void cdr_file_move(char *src,char *dst)
{
	if(rename(src,dst) == 0) {
		re_write_syslog_2(config.log,"cdr_file_move()","%s => %s",src,dst);
	} else {
		re_write_syslog_2(config.log,"cdr_file_move()","A file '%s' is not moved!",src,dst);
	}
}
