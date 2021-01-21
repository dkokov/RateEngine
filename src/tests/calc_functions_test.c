/*
 * gcc -c calc_functions_test.c -I/usr/include/libxml2
 * 
 * 
 * */
 
#include <assert.h>

#include "../misc/globals.h"
#include "../Rating/calc_functions.h"

int calc_cprice_group_test_1(int billsec)
{
	tariff tr[1]  = {0};
	rating pre    = {0};
	
	tr[0].pos   = 1;
	tr[0].delta = 1;
	tr[0].fee   = 0.0012;
	
	pre.billsec = billsec;
	
	return calc_cprice_group(tr,&pre);
}

int calc_cprice_group_test_2(int billsec)
{
	tariff tr[2] = {0};
	rating pre   = {0};
	
	tr[0].pos   = 1;
	tr[0].delta = 30;
	tr[0].fee   = 0.0012;
	tr[0].iterations = 1;
	
	tr[1].pos   = 2;
	tr[1].delta = 1;
	tr[1].fee   = 0.00001;
	tr[1].iterations = 0;
	
	pre.billsec = billsec;
	
	return calc_cprice_group(tr,&pre);
}

int calc_cprice_group_test_3(int billsec)
{
	tariff tr[2] = {0};
	rating pre   = {0};
	
	tr[0].pos   = 1;
	tr[0].delta = 30;
	tr[0].fee   = 0.0012;
	tr[0].iterations = 1;
	
	tr[1].pos   = 2;
	tr[1].delta = 1;
	tr[1].fee   = 0.00001;
	tr[1].iterations = 0;
	
	pre.billsec = billsec;
	
	return calc_cprice_group(tr,&pre);
}

int calc_cprice_sms_test(void)
{
	tariff tr[1] = {0};
	rating pre = {0};
	
	return calc_cprice_sms(tr,&pre);
}

int main(void)
{
	calc_cprice_group_test_1(64);
	
	return 0;
}
