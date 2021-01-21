#include <math.h>
#include "round_billsec.h"

int round_billsec(int round_mode_id,double billusec)
{
	int billsec;
	
	switch(round_mode_id) {
		case 1:
				billsec = (int)ceil(billusec / 1000000);
				break;
		case 2:
				billsec = (int)floor(billusec / 1000000);
				break;
		default:
				billsec = 0;
	}
	return billsec;
}
