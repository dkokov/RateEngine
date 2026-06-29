/*
 * gcc -Wall -g -o xml_cfg_test xml_cfg_test.c -L../ -lre7core
 * 
 * ./xml_cfg_test /usr/local/RateEngine/config/cdr_profiles/cdr_db_profile.xml
 * 
 * valgrind ./xml_cfg_test /usr/local/RateEngine/config/cdr_profiles/cdr_db_profile.xml 
 * 
 * */

#include "../config/xml_cfg.h"

int main(int argc, char *argv[])
{
	char *filename;

	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
	xml_param_t *params;
		
	if(argc == 1) return -1;
	
	if(argv[1] == NULL) return -1;
	
	filename = argv[1];
	
	if(strlen(filename) == 0) return -1;
	
	doc = xml_cfg_doc(filename);
	root = xml_cfg_root(doc);

	node = xml_cfg_node_init();
	strcpy(node->node_name,"General");
	
	xml_cfg_params_get(root,node);

	params = node->params;
	
	while(params != NULL) {
		printf("%s %s\n",params->name,params->value);
		params = params->next_param;
	}
	
	xml_cfg_params_free(node->params);
	
	strcpy(node->node_name,"CDRFormat");

	xml_cfg_params_get(root,node);

	params = node->params;
	
	while(params != NULL) {
		printf("%s %s\n",params->name,params->value);
		params = params->next_param;
	}
	
	xml_cfg_params_free(node->params);
	
	free(node);
	
	xml_cfg_free_doc(doc);	
	
	return 0;
}

