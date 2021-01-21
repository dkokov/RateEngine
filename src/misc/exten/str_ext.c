#include <string.h>

#include "../../mem/mem.h"

/* remove 'search' symbol from the source string */
char *str_ext_clear_str(char *src,char search)
{
	int i,p,len;
	char *buf;
	
	len = strlen(src);
	
	buf = (char *)mem_alloc(len+1);

	if(buf == NULL) return NULL;

	i=0;p=0;
	while(src[i] != '\0') {
		if(src[i] != search) {
			buf[p] = src[i];
			p++;
		}
			
		i++;
	}	

	return buf;
}

char *str_ext_replace_symbol(char *src,char search,char replace)
{
	int i,len;
	char *buf;
	
	len = strlen(src);
	
	buf = (char *)mem_alloc(len+1);

	if(buf == NULL) return NULL;

	i=0;
	while(src[i] != '\0') {
		if(src[i] == search) buf[i] = replace;
		else buf[i] = src[i];
		
		i++;
	}	

	return buf;
}
