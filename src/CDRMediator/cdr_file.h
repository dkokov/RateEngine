#ifndef CDR_FILE_H
#define CDR_FILE_H

#define CDR_FILE_HDR_LEN 32
#define READBUF_LEN 1024
#define CDR_FIELD_TXT_LEN 256

typedef struct cdr_file_list {
	
	char filename[256];
	
}cdr_file_list_t;

typedef struct cdr_file_field {
	
    int  id;
    char txt[CDR_FIELD_TXT_LEN];
    
}cdr_file_field_t;

typedef struct cdr_file_header {

	unsigned short hdr_id;
	char hdr_name[CDR_FILE_HDR_LEN];
	
	void (*func)(cdr_t *,char *);

}cdr_file_header_t;

typedef struct cdr_file_profile {
	
	char src_dir[256];
	unsigned short files_number;
	cdr_file_list_t *list;

	char dst_dir[256];

	cdr_file_header_t *hdr;
	unsigned short hdr_num;
	
	char  col_sep[2];
	char  hdr_comm;
	char  *hdr_sep;
	char  line_end[2];

	PGconn *conn;
	filter *filters;
	int cdr_server_id;
	int cdr_rec_type_id;

	char col_delimiter[2];
	
	int file_field_num;
	
}cdr_file_profile_t;

int cdr_file_chk_readbuf(char *readbuf);
void cdr_file_getfield(cdr_file_field_t *arr,char *line,char *sep,char *del,char *end);
void cdr_file_field_init_arr(cdr_file_field_t *pt,size_t mem);
cdr_file_field_t *cdr_file_field_create_arr(size_t mem);
void cdr_file_field_free_arr(cdr_file_field_t *pt);
cdr_file_header_t *cdr_file_header_init(int num);
cdr_file_profile_t *cdr_file_profile_init(void);
void cdr_file_header_put(cdr_file_header_t *hdr,cdr_table_t *tbl,char *hdr_name,int hdr_id);
void cdr_file_profile_free(cdr_file_profile_t *profile);
int cdr_file_parser(cdr_file_profile_t *profile,char *filename);
void cdr_file_reader(cdr_file_profile_t *profile);
void cdr_file_move(char *src,char *dst);

#endif
