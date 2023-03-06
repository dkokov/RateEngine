#include <math.h>

#include "../../misc/globals.h"
#include "../../db/db.h"

#include "rt_data.h"
#include "calc_functions.h"

int append_free_billsec(rating_t *pre)
{
	int checksec;
    int free_billsec_limit;
    int free_billsec;
    double cprice;
			
	checksec = pre->billsec;
    free_billsec_limit = 0;
    free_billsec = 0;

    free_billsec_limit = pre->free_billsec_limit;
	cprice = pre->cprice;
	
	if(free_billsec_limit) {
		free_billsec = ((free_billsec_limit) - (pre->free_billsec_sum));
		
		if(free_billsec > 0) {
			if(checksec > free_billsec)	return 1 ;
		
			cprice = ((cprice) * (-1));
			pre->cprice = cprice;
		}
    }
    
    return 0;
}

void calc_maxsec(racc_t *rtp)
{
	int p;
	int units;
	int maxsec;
	int free_billsec;
	double limit;
	
	rating_t *pre;
	calc_function_t *tr;
	
	pre = rtp->pre;
	tr = rtp->bplan_ptr->rates_ptr->calc_funcs;
	
	units = 0;
	maxsec = 0;
	free_billsec = ((pre->free_billsec_limit)-(pre->free_billsec_sum));
	limit = pre->limit;
	
	if(free_billsec > 0) {
		maxsec = free_billsec;
		if(call_maxsec_limit <= maxsec) goto end_func;
	}
	
	p=0;
	while(tr[p].pos) {
		if((free_billsec > 0)&&(maxsec < tr[p].delta)) {
			maxsec = 0;
			//break;
		}
		
		if((limit) <= 0) break;
		
		if((tr[p].fee) > 0) units = floor((limit/tr[p].fee));
				
		if((tr[p].fee) == 0) units = 1;
		
		if(tr[p].iterations == 0) {
			maxsec = maxsec + (units * tr[p].delta);
			break;
		} else {
			if(units > tr[p].iterations) {
				maxsec = maxsec + (tr[p].iterations * tr[p].delta);
				limit = limit - (tr[p].iterations * tr[p].fee);
			}
			
			if(units <= tr[p].iterations) {
				maxsec = maxsec + (units * tr[p].delta);
				limit = limit - (units * tr[p].fee);
			}
		}	
		p++;
	}
	
	end_func:
	if(call_maxsec_limit <= maxsec) maxsec = call_maxsec_limit;
	
	pre->maxsec = maxsec;
}

int calc_cprice(racc_t *rtp)
{
    int p;
    float units;
    int checksec,billsec;
    double cprice;
    
	calc_function_t *tr;
	rating_t *pre;
    
    pre = rtp->pre;
    tr = rtp->bplan_ptr->rates_ptr->calc_funcs;
    
    checksec = pre->billsec;
    billsec  = pre->billsec;

    cprice = 0;
    units = 0;

    p=0;
    while(tr[p].pos) {
		units = (((float)checksec / tr[p].delta));

		if((tr[p].iterations) == 0) {
			if(units >= 1) {
				cprice = (cprice + (tr[p].fee * units));
				checksec = (checksec - (tr[p].delta * units));
			} else {
				cprice = (cprice + (tr[p].fee));
				checksec = tr[p].delta;
			}
			
			break;
		} else {
			if(units >= 1)  {
				cprice = (cprice + (tr[p].fee * tr[p].iterations));
				checksec = (checksec - (tr[p].delta * tr[p].iterations));
			} else {
				cprice = (cprice + (tr[p].fee));
				checksec = checksec - billsec;
				billsec = (tr[p].delta * tr[p].iterations);
				break;
			}
		}
	
		p++;
    }
	
    pre->cprice = cprice;
    pre->billsec = billsec;

    if(checksec == 0) return 0;
    else return -1;
}

int calc_cprice_2(racc_t *rtp)
{
	int p;
	int units;
	int billsec;
	int checksec;
	double cprice;
	
	calc_function_t *tr;
	rating_t *pre;
	
	units = 0;
	cprice = 0;
	billsec = 0;	
	
	pre = rtp->pre;
	tr = rtp->bplan_ptr->rates_ptr->calc_funcs;
	checksec = pre->billsec;

	p=0;
	while(tr[p].pos) {
		units = ceil(((float)checksec)/((float)tr[p].delta));
	
		DBG("calc_cprice_2()","phase 1,call_uid %s,p %d,units %d",pre->call_uid,p,units);
		
		if(tr[p].iterations == 0) {
			cprice = cprice + (units * tr[p].fee);
			billsec = billsec + (units * tr[p].delta);
			checksec = checksec - (units * tr[p].delta);
			break;
		} else {
			if(units > tr[p].iterations) {
				cprice = cprice + (tr[p].iterations * tr[p].fee);
				billsec = billsec + (tr[p].iterations * tr[p].delta);
				checksec = checksec - (tr[p].iterations * tr[p].delta);
			}
			
			if(units <= tr[p].iterations) {
				cprice = cprice + (units * tr[p].fee);
				billsec = billsec + (units * tr[p].delta);
				checksec = checksec - (units * tr[p].delta);
			}
			
			if(units == 1) break;		
		}
		p++;
	}
	
	pre->cprice = cprice;
    pre->billsec = billsec;
	
	DBG("calc_cprice_2()","phase 2,call_uid %s,p %d,checksec %d,billsec %d",pre->call_uid,p,checksec,pre->billsec);
	
    if(checksec <= 0) return 0;
    else return -1;
}

int calc_cprice_sms(racc_t *rtp)
{	
	rating_t *pre;
	calc_function_t *tr;
	
	pre = rtp->pre;
	tr = rtp->bplan_ptr->rates_ptr->calc_funcs;
	
	if((tr[0].pos == 1)&&(tr[0].delta == 0)&&(pre->billsec == 0)) {
		pre->cprice = tr[0].fee;
		
		/* free_billsec_limit is 'free_billsec.free_billsec' , free_billsec is 'sum from rated sms with cprice < 0' */
		if(pre->free_billsec_limit > 0) {
			if(pre->free_billsec_limit > pre->free_billsec) {
				pre->cprice = ((-1)*(pre->cprice));
			}
		}
	} else return -1;
	
	if(log_debug_level == LOG_LEVEL_DEBUG) {
		//LOG("calc_cprice_sms()","call_uid %s,cprice %f,bplan %d,tariff %d",pre->call_uid,pre->cprice,pre->bplan,pre->tariff);						
	}
	
	return 0;
}

int calc_cprice_group(racc_t *rtp)
{
	if((calc_cprice_2(rtp)) == (-1)) return -1;
		
	return append_free_billsec(rtp->pre);
}

