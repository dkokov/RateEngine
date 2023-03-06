#ifndef XML_CFG_H
#define XML_CFG_H

#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

#define XML_NAME_LEN  128
#define XML_VALUE_LEN 256
#define XML_NODE_NAME_LEN 128

typedef struct xml_param {
	
	char   name[XML_NAME_LEN];
	char value[XML_VALUE_LEN];

	struct xml_param *next_param;
	
} xml_param_t;

typedef struct xml_node {
	
	char node_name[XML_NODE_NAME_LEN];
	xml_param_t *params;
	
	char child_node_name[XML_NODE_NAME_LEN];
	struct xml_node *child_node;
	unsigned short child_num;
	
	struct xml_node *next_node;
	
} xml_node_t;

xml_node_t *xml_cfg_node_init(void);
xml_param_t *xml_cfg_param_init(void);
xmlDoc *xml_cfg_doc(char *xml_filename);
xmlNode *xml_cfg_root(xmlDoc *doc);
int xml_cfg_params_get(xmlNode *root,xml_node_t *node);
void xml_cfg_params_free(xml_param_t *params);
xml_param_t *xml_cfg_param_get(char *search);
void xml_cfg_free_doc(xmlDoc *doc);
int xml_cfg_child_get(xmlNode *root,xml_node_t *node);
void xml_cfg_child_free(xml_node_t *child);

#endif
