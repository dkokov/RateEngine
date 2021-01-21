#include <time.h>

#include "globals.h"
#include "re5_fstat.h"

void reload_logfile(int log,char *log_filename,unsigned int log_max_file_size)
{
	unsigned int filesize;
	
	char new_fn[256];
	time_t res;
	struct tm *tt;
	
	filesize = re5_fstat(log_filename);
	
	if(filesize >= log_max_file_size) {
		res = time(NULL);
			
		tt = localtime(&res);
						
		sprintf(new_fn,"%s.%.4d%.2d%.2d%.2d%.2d%.2d",log_filename,
						                            (tt->tm_year+1900),(tt->tm_mon+1),(tt->tm_mday),
											        (tt->tm_hour),(tt->tm_min),(tt->tm_sec));
		re_reload_syslog(log,log_filename,new_fn);
	}
} 
