/* 
 * gcc -g -o monit smtp/send_email.c monit.c  -I/usr/include/libxml2 -lpq -lcurl -L ../../ -lre6core -lcdrmediator -lrating -lre6cc
 * valgrind --leak-check=full -s ./monit
 * */

#include "../globals.h"
#include "../mem/mem.h"
#include "../../config/xml_cfg.h"

#include "smtp/send_email.h"
#include "snmp/snmp_defs.h"
#include "monit.h"

int main(void)
{
	char msg[] = "Test message from DKokov ...";
	monit_cfg_t *cfg = NULL;
	monit_info_t info = {0};
	
	cfg = monit_cfg_main("../../config/samples/RateEngine6.xml");
	if(cfg) {
		info.t = notifications;
		info.ptr = (void *)msg;
		
		monit_main(cfg,&info);
		
		monit_cfg_free(cfg);
	}
	
	return 0;
}
