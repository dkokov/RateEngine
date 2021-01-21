//#include "../RateEngine.h"

#include <stdlib.h>
#include <string.h>

#include "../misc/globals.h"
#include "../misc/mem/mem.h"
#include "../config/xml_cfg.h"
#include "../syslog/rt_log.h"

#include "rt_cfg.h"

rt_cfg_t *rt_cfg_init(void)
{
	rt_cfg_t *cfg = NULL;
	
	cfg = (rt_cfg_t *)mem_alloc(sizeof(rt_cfg_t));
		
	return cfg;
} 

void rt_cfg_get(rt_cfg_t *cfg)
{
	xml_param_t *params = NULL;
	
	if(cfg != NULL) {
		cfg->node = xml_cfg_node_init();
		strcpy(cfg->node->node_name,"Rating");
	
		xml_cfg_params_get(cfg->root,cfg->node);

		params = cfg->node->params;
	
		while(params != NULL) {
			if(strcmp(params->name,"active") == 0) {
				if(strcmp(params->value,"yes") == 0) cfg->rating_active_flag = 't';
				else cfg->rating_active_flag = 'f';
			}
			
			if(strcmp(params->name,"UsePCard") == 0) {
				if(strcmp(params->value,"yes") == 0) cfg->use_pcard = 't';
				else cfg->use_pcard = 'f';
			}
			
			if(strcmp(params->name,"PCardSortKey") == 0) {
				strcpy(cfg->pcard_sort_key,params->value);
			}
			
			if(strcmp(params->name,"PCardSortMode") == 0) {
				strcpy(cfg->pcard_sort_mode,params->value);
			}
			
			if(strcmp(params->name,"NoPrefixRating") == 0) {
				cfg->no_prefix_rating = *params->value;
			}
			
			if(strcmp(params->name,"BillingDay") == 0) {
				strcpy(cfg->billing_day,params->value);
			}
			
			if(strcmp(params->name,"RatingInterval") == 0) {
				cfg->rating_interval = atoi(params->value);
			}
			
			if(strcmp(params->name,"DayOfPayment") == 0) {
				cfg->day_of_payment = atoi(params->value);
			}
			
			if(strcmp(params->name,"KLimitMin") == 0) {
				cfg->k_limit_min = atof(params->value);
			}
			
			if(strcmp(params->name,"leg") == 0) {
				if((*params->value == 'a')||(*params->value == 'b')) cfg->leg = *params->value;
			}
			
			if(strcmp(params->name,"BalActiveNum") == 0) {
				cfg->bal_num = atoi(params->value);
			}			
						
			params = params->next_param;
		}
	
		xml_cfg_params_free(cfg->node->params);
	}
}

void rt_cfg_params_init(rt_cfg_t *cfg)
{
	if(strcmp(cfg->billing_day,"") == 0) strcpy(cfg->billing_day,BILLING_DAY);
		
	if(cfg->day_of_payment == 0) cfg->day_of_payment = DAY_OF_PAYMENT;
		
	if(cfg->k_limit_min == 0) cfg->k_limit_min = K_LIMIT_MIN;
	
	if(cfg->no_prefix_rating == '\0') cfg->no_prefix_rating = NO_PREFIX_RATING;
}

rt_cfg_t *rt_cfg_main(char *xmlFileName)
{
	rt_cfg_t *cfg;
	
	cfg = rt_cfg_init();

	if(cfg != NULL) {
		cfg->doc = xml_cfg_doc(xmlFileName);
	
		if(cfg->doc != NULL) {
			cfg->root = xml_cfg_root(cfg->doc);
	
			if(cfg->root != NULL) {
				rt_cfg_get(cfg); 
			} else {
				LOG("rt_cfg_main()","A main 'root' pointer is null!");
				free(cfg);
				cfg = NULL;
			}
		
			if(cfg != NULL) {
				mem_free(cfg->node);
				xml_cfg_free_doc(cfg->doc);
			}
		}
	
		rt_cfg_params_init(cfg);
	}
	
	return cfg;
}
