#include "../misc/globals.h"
#include "../mem/mem.h"

#include "xml_cfg.h"

xml_node_t *xml_cfg_node_init(void)
{
	return (xml_node_t *)mem_alloc(sizeof(xml_node_t));
}

xml_param_t *xml_cfg_param_init(void)
{
	return (xml_param_t *)mem_alloc(sizeof(xml_param_t));		
}

/* http://xmlsoft.org/html/libxml-parser.html#xmlReadFile */
xmlDoc *xml_cfg_doc(char *xml_filename)
{
	char *fpt;
	xmlDoc *doc;
	
	fpt = realpath(xml_filename,NULL);
	
	if(fpt == NULL) return NULL;
	else free(fpt);
	
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
					
//					printf("node=%s name=%s value=%s\n",node->node_name,param->name,param->value);
				}
			}
		
			break;
		}
	}
	
	return 0;
}
int xml_cfg_params_get_v2(xmlNode *curr,xml_node_t *node)
{
	xml_param_t *param;
	xml_param_t *params;
	
	params = NULL;
		
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
					
//					printf("node=%s name=%s value=%s\n",node->node_name,param->name,param->value);
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

int xml_cfg_child_get(xmlNode *root,xml_node_t *node)
{	
	xml_node_t *tmp;
	xml_node_t *list;
	
	list = NULL;
	
	xmlNode *curr = NULL;	
	
	for(curr = root->children;curr;curr = curr->next) {
		if(!xmlStrcmp(curr->name,(xmlChar *) node->node_name)) {

			xmlNode *child = NULL;
			
			node->child_num = 0;
			
			for(child = curr->children; child; child = child->next) {
				if(!xmlStrcmp(child->name,(xmlChar *) node->child_node_name)) {			
					tmp = xml_cfg_node_init();
							
					if(list == NULL) {
						list = tmp;
						node->child_node = list;
					} else {
						list->next_node = tmp;
						list = list->next_node;
					}

					strcpy(tmp->node_name,node->child_node_name);
				
					xml_cfg_params_get_v2(child,tmp);

					node->child_num ++;
				}
			}
		}
	}
	
	return 0;
}

void xml_cfg_child_free(xml_node_t *child)
{
	xml_node_t *tmp;
	
	while(child != NULL) {
		tmp = child;
		child = child->next_node;
			
		xml_cfg_params_free(tmp->params);
			
		mem_free(tmp);
	}
}

