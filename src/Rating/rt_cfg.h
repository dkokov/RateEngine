#ifndef RT_CFG_H
#define RT_CFG_H

#define BILLING_DAY "01"
#define DAY_OF_PAYMENT 14
#define K_LIMIT_MIN 0.05

#define NO_PREFIX_RATING '&'

typedef struct rt_cfg {
    
	xmlDoc *doc;
	xmlNode *root;
	xml_node_t *node;
    
    char use_pcard;
    
    char billing_day[3];
    
    int day_of_payment;
    
    char rating_active_flag;
    
    char no_prefix_rating;
    
    int rating_interval;
    unsigned int rt_interval;
    
    float k_limit_min;
    
    char pcard_sort_key[12];
    char pcard_sort_mode[5];
    
    char leg;
    
    unsigned short bal_num;
     
}rt_cfg_t;

rt_cfg_t *rt_cfg_init(void);
void rt_cfg_get(rt_cfg_t *cfg);
void rt_cfg_params_init(rt_cfg_t *cfg);
rt_cfg_t *rt_cfg_main(char *xmlFileName);

#endif
