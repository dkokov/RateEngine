#include <math.h>

#include "../misc/globals.h"

int append_free_billsec(rating *pre)
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

int old_calc_maxsec(PGconn *conn,rating *pre,tariff *tr)
{
  int p,checksec;
  double cprice;
  //int freesec;

  cprice = 0; 
  pre->maxsec = 0;
  checksec = 0;
  //freesec = 0;
    
 /* if(pre->free_billsec_limit) 
  {
    if(pre->free_billsec_limit > pre->free_billsec) freesec = (pre->free_billsec_limit)-(pre->free_billsec);
    else pre->free_billsec = 0;
  }
  else
  {
    pre->free_billsec = 0;
  }
*/  
  // Pri tozi na4in na kalkulirane , Problem s rate->fee = 0.00 ... !!!
  p=0;
  while(tr[p].pos) 
  {
    //printf("p=[%d] fee=%f pos[%d] \n",p,tr[p].fee,tr[p].pos);
    if(tr[p].iterations) 
    {
		// kakvo shte stane ako pyrvata iteraciq e s fee = 0.00 ???
		cprice = (cprice + (tr[p].fee * tr[p].iterations));
		checksec = (checksec + (tr[p].delta * tr[p].iterations));
    }
    else 
    {
		// edna iteraciq ili posledna iteraciq !!!
		if((tr[p].fee) == 0 ) pre->maxsec = -5; //  free tariff ???
		else pre->maxsec = (pre->limit - cprice) / tr[p].fee ;
    }
    p++;
  }

  if((pre->maxsec) == -5) pre->maxsec = call_maxsec_limit;

  if((pre->maxsec) > call_maxsec_limit) pre->maxsec = call_maxsec_limit;
     
  if((pre->maxsec) <= checksec) return 0;
  else return pre->maxsec;
}

void calc_maxsec(PGconn *conn,rating *pre,tariff *tr)
{
	int p;
	int units;
	int maxsec;
	int free_billsec;
	double limit;
	
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

int calc_cprice(tariff *tr,rating *pre)
{
    int p;
    float units;
    int checksec,billsec;
    double cprice;

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

int calc_cprice_2(tariff *tr,rating *pre)
{
	int p;
	int units;
	int billsec;
	int checksec;
	double cprice;
		
	units = 0;
	cprice = 0;
	billsec = 0;	
	checksec = pre->billsec;
	
	p=0;
	while(tr[p].pos) {
		units = ceil(((float)checksec)/((float)tr[p].delta));
		
		if(log_debug_level == LOG_LEVEL_DEBUG) {							
			LOG("calc_cprice_2()","phase 1,call_uid %s,p %d,units %d",pre->call_uid,p,units);
		}
		
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
	
	if(log_debug_level == LOG_LEVEL_DEBUG) {							
		LOG("calc_cprice_2()","phase 2,call_uid %s,p %d,checksec %d,billsec %d",pre->call_uid,p,checksec,pre->billsec);
	}
	
    if(checksec <= 0) return 0;
    else return -1;
}

int calc_cprice_sms(tariff *tr,rating *pre)
{	
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
		LOG("calc_cprice_sms()","call_uid %s,cprice %f,bplan %d,tariff %d",pre->call_uid,pre->cprice,pre->bplan,pre->tariff);						
	}
	
	return 0;
}

int calculate2(tariff *tr,rating *pre)
{
    int p;
    float units;
    int checksec,billsec;
    int free_billsec_limit;
    int free_billsec;
    double cprice;

    checksec = 0 ;
    cprice = 0;
    free_billsec_limit = 0;
    free_billsec = 0;

    checksec = pre->billsec;
    billsec = checksec;
    free_billsec_limit = pre->free_billsec_limit;

    units = 0;

    p=0;
    while(tr[p].pos) {
		units = (((float)checksec / tr[p].delta));

		if((tr[p].iterations) == 0) {
			cprice = (cprice + (tr[p].fee * units));
			checksec = (checksec - (tr[p].delta * units));
			break;
		} else {
			if(units >= 1) {
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

    if(free_billsec_limit) {
		free_billsec = ((free_billsec_limit) - (pre->free_billsec));
		if(checksec <= free_billsec) cprice = cprice * (-1);
		else {
            /* Pri tazi situaciq trqbva 'billsec'-a da se razdeli na dve:
                => check_billsec_1 = free_billsec
                => check_billsec_2 = (check_billsec - free_billsec)
                Razgovoryt trqbva da se preiz4isli s novite vremena!!!
                ??? 
            */
        }
	/*
	    Pri taka napravenata shema,kogato bezplatnite sekundi sa po-malko ot izgovorenoto v reitvaniq
	    razgovor,v nego nqma da bydat ot4eteni bezplatni sekundi!
	*/
    }
	
    pre->cprice = cprice;
    pre->billsec = billsec;

    if(checksec == 0) return 0;
    else return 1;
}

int calc_cprice_group(tariff *tr,rating *pre)
{
	if((calc_cprice_2(tr,pre)) == (-1)) return -1;
		
	return append_free_billsec(pre);
}

