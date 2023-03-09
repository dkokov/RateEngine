/*
 * http://tldp.org/HOWTO/Program-Library-HOWTO/dl-libraries.html
 * https://stackoverflow.com/questions/384121/creating-a-module-system-dynamic-loading-in-c
 * https://pubs.opengroup.org/onlinepubs/009695399/functions/dlsym.html
 * 
 * */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <dlfcn.h> 

#include "../mem/mem.h"
#include "../misc/globals.h"

#include "mod.h"

mod_t *mod_init(void)
{	
	return (mod_t *)mem_alloc(sizeof(mod_t));
}

void mod_free(void)
{
	mod_t *curr;

	while(mod_lst != NULL) {
		curr = mod_lst;

		LOG("mod_free()","module '%s',free memory from the HEAP",curr->mod_name);
		mem_free(curr);
		
		mod_lst = mod_lst->next;
	}
}

mod_cfg_t *mod_cfg_init(void)
{
	return (mod_cfg_t *)mem_alloc(sizeof(mod_cfg_t));
}

void mod_cfg_free(mod_cfg_t *cfg)
{
	if(cfg != NULL) mem_free(cfg);
}

int mod_dep_module(mod_t *mod_ptr)
{
	int i;
	mod_t *tmp;
	
	mod_dep_t *dep = mod_ptr->depends;
	
	i = 0;
	while(strlen(dep[i].dep_mod_name) > 0) {
		tmp = mod_find_module(dep[i].dep_mod_name);
		if(tmp == NULL) {
			LOG("mod_dep_module()","ERROR!The dep module '%s' is not loaded!",dep[i].dep_mod_name);

			return RE_ERROR;
		} else {
			LOG("mod_dep_module()","The dep module '%s' is loaded!",dep[i].dep_mod_name);
		}
		
		i++;
	}
	
	return RE_SUCCESS;
}

int mod_init_module(mod_t *mod_ptr)
{
	int (*fptr)(void);
	
	if(mod_ptr == NULL) return RE_ERROR;
	
	if(mod_ptr->init == NULL) return RE_ERROR;
	
	fptr = mod_ptr->init;
	
	return fptr();
}
/*
void mod_free_module(mod_t *mod_ptr)
{

}*/

void *mod_load_module(char *path)
{
	void *handle;
	
	handle = NULL;
	
	handle = dlopen(path,DL_OPT);
	
    if(!handle) {
		LOG("mod_load_module()","dlopen() %s\n", dlerror());
		return NULL;
	}
	
	dlerror();
	
	return handle;
}

//void mod_destroy_module(void *handle)
void mod_destroy_module(mod_t *mod_ptr)
{
//	int (*fptr)(void);
	/*
	if(mod_ptr->destroy != NULL) {
		fptr = mod_ptr->destroy;
		
		LOG("mod_destroy_module()","call destroy function (ret=%d), module %s",fptr(),mod_ptr->mod_name);		
	} */
	
	if(mod_ptr->handle != NULL) dlclose(mod_ptr->handle);
}
/*
void *mod_find_func(void *handle,char *funcname)
{	
	if(handle == NULL) return NULL;
		
	return dlsym(handle,funcname);
}*/

void *mod_find_sim(void *handle,char *sim)
{
	if(handle == NULL) return NULL;
		
	return dlsym(handle,sim);	
}

void mod_destroy_modules(void)
{	
	mod_t *tmp = mod_lst;
	
	while(tmp != NULL) {
		LOG("mod_destroy_modules()","destroy mod_name: %s",tmp->mod_name);
		
		mod_destroy_module(tmp);
		tmp->handle = NULL;
		
		tmp = tmp->next;
	}
}

mod_t *mod_find_module(char *mod_name)
{
	mod_t *find,*chk;
	
	find = NULL;
	chk  = NULL;

	if(mod_lst == NULL) return NULL;
	
	chk = mod_lst;
	while(chk != NULL) {
		if(strcmp(chk->mod_name,mod_name) == 0) {
			find = chk;
			break;
		}

		chk = chk->next;
	}

	return find;
}

/* From a module name,compose module's mod_t struct name - content setup per module */
void mod_struct_name(char *mod_struct_n,char *mod_name)
{			
	char *buf = strdup(mod_name);
			
	for(int p=0;p<strlen(buf);p++) {
		if(buf[p] == '.') buf[p] = '\0';
	}
			
	sprintf(mod_struct_n,"%s_mod_t",buf);
			
	mem_free(buf);
}

/* Make modules list from the XML config file,tags <LoadModules> */
int mod_load_modules(char *dirname)
{
	int ret;
	char path[512];	
	char mod_struct_n[128];
	void *handle;

	mod_cfg_t *cfg;
	mod_t *ptr,*last,*rem;	
		
	xml_param_t *params = NULL;
	
	last = NULL;	
	
	if(mcfg == NULL) return RE_ERROR_N;

	cfg = mod_cfg_init();

	if(cfg != NULL) {
		cfg->doc = xml_cfg_doc(mcfg->cfg_filename);
	
		if(cfg->doc != NULL) {
			cfg->root = xml_cfg_root(cfg->doc);
			if(cfg->root != NULL) {
				cfg->node = xml_cfg_node_init();

				strcpy(cfg->node->node_name,"LoadModules");
				xml_cfg_params_get(cfg->root,cfg->node);
				
				params = cfg->node->params;

				while(params != NULL) {
					if(strcmp(params->name,"module") == 0) {							
						memset(path,0,512);
						sprintf(path,"%s%s",dirname,params->value);
						handle = mod_load_module(path);
						if(handle) {
							ptr = mod_init();
			
							if(ptr == NULL) return RE_ERROR_N;

							ptr->handle = handle;
							strcpy(ptr->mod_name,params->value);
					
							LOG("mod_load_modules()","load module '%s' ...",ptr->mod_name);
	
							memset(mod_struct_n,0,sizeof(mod_struct_n));
							mod_struct_name(mod_struct_n,ptr->mod_name);
							
							rem = (mod_t *)mod_find_sim(handle,mod_struct_n);
							if(rem != NULL) {
								if(rem->depends != NULL) mod_dep_module(rem);
								
								if(rem->init != NULL) { 
									ret = mod_init_module(rem);
									
									if(ret == RE_SUCCESS) {
										LOG("mod_load_modules()","init module '%s' ... success",ptr->mod_name);
									} else {
										LOG("mod_load_modules()","init module '%s' ... unsuccess",ptr->mod_name);
									}
								}
								
								if(rem->destroy != NULL) ptr->destroy = rem->destroy;
							}
							
							if(mod_lst == NULL) {
								mod_lst = ptr;
								last = mod_lst;
							} else {
								last->next = ptr;
								last = last->next;
							}
						} else {
							LOG("mod_load_modules()","ERROR! Un-load module '%s' ...",path);
						}
					}
		
					params = params->next_param;
				}

				xml_cfg_params_free(cfg->node->params);
				
				mem_free(cfg->node);
			}
			
			xml_cfg_free_doc(cfg->doc);
		}

		mod_cfg_free(cfg);
	}
		
	return RE_SUCCESS;
}
