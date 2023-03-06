#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 

#include "../../mem/mem.h"
#include "file_list.h"

filename_list_t *file_list_init(int n)
{
	filename_list_t *list;
	
	list = (filename_list_t *)mem_alloc_arr(n,sizeof(filename_list_t));

	return list;
}

void file_list_get(file_list_t *lst)
{
	int i;
	DIR  *d;
	struct dirent *dir;

	i = 0;

	d = opendir(lst->dirname);
	
	if(d) {
		while ((dir = readdir(d)) != NULL) {
			i++;
		}

		if(i > 2) {
			seekdir(d,0);
		
			/* without '.' and '..' */
			lst->files_number = (i - 2);

			lst->list = file_list_init(lst->files_number);
		
			i=0;
			while ((dir = readdir(d)) != NULL) {
				if((strcmp(dir->d_name,".") == 0)||(strcmp(dir->d_name,"..") == 0)) {
				
				} else {
					strcpy(lst->list[i].filename,dir->d_name);
					i++;
				}
			}
		}
		
		closedir(d);
	}
}


