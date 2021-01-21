#include "../misc/globals.h"
#include "../misc/mem/mem.h"

#include "xml_cfg.h"

xml_node_t *xml_cfg_node_init(void)
{
	xml_node_t *node = NULL;
	
	node = (xml_node_t *)mem_alloc(sizeof(xml_node_t));
		
	return node;
}

xml_param_t *xml_cfg_param_init(void)
{
	xml_param_t *param = NULL;
	
	param = (xml_param_t *)mem_alloc(sizeof(xml_param_t));
		
	return param;
}

xmlDoc *xml_cfg_doc(char *xml_filename)
{
	xmlDoc *doc;
	
	doc = xmlReadFile(xml_filename,NULL,0);
	
	return doc;
}

xmlNode *xml_cfg_root(xmlDoc *doc)
{
	xmlNode *root = NULL;
	
    if(doc == NULL) return NULL;
    else {
		root = xmlDocGetRootElement(doc);
		if(root == NULL) return NULL;
	}
	
	return root;
}

int xml_cfg_params_get(xmlNode *root,xml_node_t *node)
{
	xml_param_t *param;
	xml_param_t *params;
	
	params = NULL;
	xmlNode *curr = NULL;	
	
	for(curr = root->children;curr;curr = curr->next) {
		if(!xmlStrcmp(curr->name,(xmlChar *) node->node_name)) {
			
			xmlNode *msg = NULL;
			
			for(msg = curr->children; msg; msg = msg->next) {
				if(!xmlStrcmp(msg->name,(xmlChar *) "param")) {
										
					param = xml_cfg_param_init();
							
					if(params == NULL) {
						params = param;
						node->params = params;
					} else {
						params->next_param = param;
						params = params->next_param;
					}		
					
					char *name = (char *)xmlGetProp(msg,(xmlChar *) "name");						
					if(name != NULL) strcpy(param->name,name);
					xmlFree(name);
					
					char *value = (char *)xmlGetProp(msg,(xmlChar *) "value");
					if(value != NULL) strcpy(param->value,value);
					xmlFree(value);
				}
			}
		}
	}
	
	return 0;
}

void xml_cfg_params_free(xml_param_t *params)
{
	xml_param_t *tmp;
	
	while(params != NULL) {
		tmp = params;
		params = params->next_param;
		mem_free(tmp);
	}
}

xml_param_t *xml_cfg_param_get(char *search)
{
	xml_param_t *param = NULL;

	while(param != NULL) {
		if(strcmp(param->name,search) == 0) {
			return param;
		}
		
		param = param->next_param;
	}
	
	return param;
}

void xml_cfg_free_doc(xmlDoc *doc)
{
	xmlFreeDoc(doc);
	
	xmlCleanupParser();
}

/*
void main(void)
{
	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
	xml_param_t *params;
	
	doc = xml_cfg_doc("samples/cdr_db_profile_example.xml");
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
}
*/
