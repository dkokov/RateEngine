#include <sys/stat.h>

unsigned int re5_fstat(char *filename)
{	    
	struct stat buffer;

    stat(filename,&buffer);
    
	return (unsigned int)buffer.st_size;
} 
