#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "../../mem/mem.h"

#include "rt_data.h"

bacc_t *rt_data_bacc_init(void)
{
	return mem_alloc(sizeof(bacc_t));
}

bplan_t *rt_data_bplan_init(void)
{
	return mem_alloc(sizeof(bplan_t));
}

bal_t *rt_data_bal_init(void)
{
	return mem_alloc(sizeof(bal_t));
}

rating_t *rt_data_rating_init(void)
{
	return mem_alloc(sizeof(rating_t));
}

racc_t *rt_data_racc_init(void)
{
	racc_t *racc_pt;
	
	racc_pt = mem_alloc(sizeof(racc_t));
	if(racc_pt != NULL) {
		racc_pt->bacc_ptr = rt_data_bacc_init();
		racc_pt->bplan_ptr = rt_data_bplan_init();
		racc_pt->bal_ptr = rt_data_bal_init();
//		racc_pt->pre = rt_data_rating_init();
	}
	
	return racc_pt;
}
/*
void rt_data_racc_clean(racc_t *racc_pt)
{
	if(racc_pt != NULL) {
		if(racc_pt->bacc_ptr != NULL) memset(racc_pt->bacc_ptr,0,sizeof(bacc_t));
		if(racc_pt->bplan_ptr != NULL) memset(racc_pt->bplan_ptr,0,sizeof(bplan_t));
		if(racc_pt->bal_ptr != NULL) memset(racc_pt->bal_ptr,0,sizeof(bal_t));
		if(racc_pt->pre != NULL) memset(racc_pt->pre,0,sizeof(rating_t));
	}
}*/

void rt_data_racc_free(racc_t *racc_pt)
{
	if(racc_pt != NULL) {
		if(racc_pt->bacc_ptr != NULL) {
			if(racc_pt->bacc_ptr->pcard_ptr != NULL) mem_free(racc_pt->bacc_ptr->pcard_ptr);
			
			mem_free(racc_pt->bacc_ptr);
		}
		
		if(racc_pt->bplan_ptr != NULL) {
			if(racc_pt->bplan_ptr->rates_ptr->calc_funcs != NULL) mem_free(racc_pt->bplan_ptr->rates_ptr->calc_funcs);
			
			mem_free(racc_pt->bplan_ptr);
		}
		
		if(racc_pt->bal_ptr != NULL) mem_free(racc_pt->bal_ptr);
		
		//if(racc_pt->pre != NULL) mem_free(racc_pt->pre); ??? problem
		
		mem_free(racc_pt);
	}
}
