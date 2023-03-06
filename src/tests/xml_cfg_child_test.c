/*
 * gcc -Wall -g -o xml_cfg_child_test xml_cfg_child_test.c -L../ -lre7core
 * 
 * ./xml_cfg_child_test /usr/local/RateEngine/config/cdr_profiles/cdr_db_profile.xml
 * 
 * valgrind --leak-check=full ./xml_cfg_child_test /usr/local/RateEngine/config/cdr_profiles/cdr_file_profile.xml 
 * 
 * */

#include "../config/xml_cfg.h"
#include "../mem/mem.h"

int main(int argc, char *argv[])
{
	char *filename;

	/* XML parser params */
	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node,*child;
	xml_param_t *params,*child_params;
		
	if(argc == 1) return -1;
	
	if(argv[1] == NULL) return -1;
	
	filename = argv[1];
	
	if(strlen(filename) == 0) return -1;
	
	/* Open XML Document */
	doc = xml_cfg_doc(filename);
	
	if(doc == NULL) return -1;
	
	/* XML Root Element  */
	root = xml_cfg_root(doc);
	if(root == NULL) {
		xml_cfg_free_doc(doc);
		return -1;
	}

	/* XML Node/Element 'General' */
	node = xml_cfg_node_init();
	strcpy(node->node_name,"General");
	
	xml_cfg_params_get(root,node);

	params = node->params;
	
	while(params != NULL) {
		printf("%s %s\n",params->name,params->value);
		params = params->next_param;
	}
	
	xml_cfg_params_free(node->params);
	
	/* XML Node/Element 'CDRFormat' */
	strcpy(node->node_name,"CDRFormat");

	xml_cfg_params_get(root,node);

	params = node->params;
	
	while(params != NULL) {
		printf("%s %s\n",params->name,params->value);
		params = params->next_param;
	}
	
	xml_cfg_params_free(node->params);
	
	/* XML Node/Element 'PrefixFiltering' */
	strcpy(node->node_name,"PrefixFiltering");
	
	/* XML Nodes/Childs 'filter' */
	strcpy(node->child_node_name,"filter");
	
	xml_cfg_child_get(root,node);
	
	child = node->child_node;
	while(child != NULL) {
		printf("%s => ",child->node_name);
		
		child_params = child->params;
		while(child_params != NULL) {
			printf(" %s : %s ;",child_params->name,child_params->value);
			
			child_params = child_params->next_param;
		}
		
		printf("\n");
		
		child = child->next_node;
	}
	
	/* XML Nodes/Childs free memory */
	xml_cfg_child_free(node->child_node);
	
	/* XML Node free memory */
	mem_free(node);
	
	/* XML Document/Parser free memory */
	xml_cfg_free_doc(doc);	
	
	return 0;
}

