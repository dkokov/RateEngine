#ifndef FILE_LIST_H
#define FILE_LIST_H

typedef struct filename_list {
	
	char filename[256];
	
}filename_list_t; 

typedef struct file_list {
	
	char dirname[256];
	
	unsigned int files_number;
	
	filename_list_t *list;
	
}file_list_t;

void file_list_get(file_list_t *lst);

#endif
