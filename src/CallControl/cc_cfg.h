#ifndef CC_CFG_H
#define CC_CFG_H

#include "../config/xml_cfg.h"

#define CC_PROTO_MY_CC "MyCC"
#define CC_PROTO_DIAM  "Diameter"
#define CC_PROTO_MY_CC_v2 "MyCCv2"

typedef enum cc_cfg_proto {
	unkn_proto,
	my_cc,
	diam,
	my_cc_v2
}cc_cfg_proto_t;

typedef struct cc_cfg_int {
	
	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
	
	cc_cfg_proto_t proto;
	
	char ip[128];
	unsigned short port;
		
}cc_cfg_int_t;

typedef struct cc_cfg {
    
	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
    
	char cc_active_flag;
    
    unsigned int call_maxsec_limit;
    unsigned short sim_calls;
    unsigned int cc_server_usleep;
        
    char cc_int_cfg_dir[255];
    
    char cc_logfile[255];
    unsigned int cc_log_max_file_size;
    
    cc_cfg_int_t *interfaces;
    unsigned short int_number;
     
}cc_cfg_t;

cc_cfg_t *cc_cfg_main(char *xmlFileName);

#endif
