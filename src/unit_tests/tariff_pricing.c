/*
 * tariff_pricing.c — automated parity check for the DuckDB multi-tier pricing.
 *
 * The RatingDuckDB module prices calls with a recursive SQL CTE that must
 * reproduce the Rating module's calc_cprice_2() exactly (so DuckDB rating ==
 * /Rating rating). This test pins that:
 *   ref()  = a verbatim copy of calc_cprice_2 (mod/Rating/calc_functions.c)
 *   cte()  = the per-step formula encoded by the recursive CTE in rt_duckdb.c
 * and fuzzes representative tariffs + explicit edge cases, asserting both agree
 * on (cprice, billed_sec). Exit 0 = parity, 1 = mismatch.
 *
 * Build & run:
 *   cc -O2 -lm src/unit_tests/tariff_pricing.c -o /tmp/tariff_pricing && /tmp/tariff_pricing
 *
 * Tariff model (calc_function rows by pos): each step has delta (block size,s),
 * fee (per block), iterations (#blocks this step covers; 0 = final/unbounded).
 * delta=0 is the SMS flat-fee case (charge one fee, no seconds).
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define MAXP 5

/* ---- reference: verbatim calc_cprice_2 from mod/Rating/calc_functions.c ---- */
static void ref(int billsec,const int *delta,const double *fee,const int *iter,
                int npos,double *out_cprice,int *out_bsec)
{
	int p = 0,units,checksec = billsec,billed = 0;
	double cprice = 0;

	while(p < npos) {                       /* while(tr[p].pos) */
		units = ceil((float)checksec / (float)delta[p]);

		if(iter[p] == 0) {
			cprice  += units * fee[p];
			billed  += units * delta[p];
			checksec -= units * delta[p];
			break;
		} else {
			if(units > iter[p]) {
				cprice  += iter[p] * fee[p];
				billed  += iter[p] * delta[p];
				checksec -= iter[p] * delta[p];
			}
			if(units <= iter[p]) {
				cprice  += units * fee[p];
				billed  += units * delta[p];
				checksec -= units * delta[p];
			}
			if(units == 1) break;
		}
		p++;
	}

	*out_cprice = cprice;
	*out_bsec   = billed;
}

/* ---- the recursive-CTE per-step formula encoded in rt_duckdb.c ---- */
static void cte(int billsec,const int *delta,const double *fee,const int *iter,
                int npos,double *out_cprice,int *out_bsec)
{
	int rem = billsec,bsec = 0,done = 0,p = 0;
	double price = 0;

	while(p < npos && !done) {
		int    d  = delta[p];
		double f  = fee[p];
		int    it = iter[p];
		int    u  = (d == 0) ? 1 : (int)ceil((double)rem / (double)d);   /* delta=0 -> SMS flat */
		int    charge = (it == 0 || u <= it) ? u : it;

		price += charge * f;
		bsec  += charge * d;
		rem   -= charge * d;
		done   = (it == 0 || d == 0 || u == 1);
		p++;
	}

	*out_cprice = price;
	*out_bsec   = bsec;
}

/* Phase D free-pool drawdown: the clamp formula (free_sec = clamp(0, billsec,
 * limit - (history + running SUM of prior billsec))) used by the DuckDB window
 * must equal the sequential drawdown (consume free per call until the pool is
 * empty). Compares per-call free_sec over fuzzed call sequences. */
static int test_free_split(void)
{
	int fails = 0,seed;

	for(seed = 1; seed < 80000; seed++) {
		unsigned int st = (unsigned int)seed;
		int n     = 1 + rand_r(&st) % 8;
		int limit = rand_r(&st) % 600;
		int hist  = rand_r(&st) % 200;
		int bs[8],i;
		int consumed = hist;     /* sequential: history + free consumed so far */
		long cum = 0;            /* clamp: history + cumulative full billsec    */

		for(i = 0; i < n; i++) bs[i] = rand_r(&st) % 300;

		for(i = 0; i < n; i++) {
			int rem_seq  = limit - consumed;
			int free_seq = bs[i];
			if(free_seq > (rem_seq > 0 ? rem_seq : 0)) free_seq = (rem_seq > 0 ? rem_seq : 0);
			consumed += free_seq;

			long rem_my  = (long)limit - (hist + cum);
			int  free_my = bs[i];
			if((long)free_my > (rem_my > 0 ? rem_my : 0)) free_my = (int)(rem_my > 0 ? rem_my : 0);
			cum += bs[i];

			if(free_seq != free_my) {
				if(fails < 5)
					printf("FREE MISMATCH seed=%d limit=%d hist=%d call=%d bs=%d: seq=%d clamp=%d\n",
					       seed,limit,hist,i,bs[i],free_seq,free_my);
				fails++;
			}
		}
	}

	printf("free_split: %s\n",fails ? "FAIL" : "OK");
	return fails;
}

static int check(int bs,const int *d,const double *f,const int *it,int n,const char *tag)
{
	double c1,c2; int b1,b2;
	ref(bs,d,f,it,n,&c1,&b1);
	cte(bs,d,f,it,n,&c2,&b2);
	if(fabs(c1 - c2) > 1e-9 || b1 != b2) {
		printf("FAIL [%s] bs=%d: ref(price=%.4f,bsec=%d) cte(price=%.4f,bsec=%d)\n",
		       tag,bs,c1,b1,c2,b2);
		return 1;
	}
	return 0;
}

int main(void)
{
	int fails = 0,tested = 0,seed,bs;

	/* explicit, real-world tariffs */
	{
		/* 30s setup free, then per-second */
		int d[]  = {30,1};   double f[]  = {0.00,0.02}; int it[]  = {1,0};
		/* 30s @0.05 (min charge), then per-second */
		int d2[] = {30,1};   double f2[] = {0.05,0.02}; int it2[] = {1,0};
		/* flat per-minute */
		int d3[] = {60};     double f3[] = {0.10};      int it3[] = {0};
		/* SMS flat fee (delta=0) */
		int d4[] = {0};      double f4[] = {0.06};      int it4[] = {0};
		for(bs = 0; bs <= 600; bs++) {
			fails += check(bs,d, f, it, 2,"setup-free"); tested++;
			fails += check(bs,d2,f2,it2,2,"setup-min");  tested++;
			fails += check(bs,d3,f3,it3,1,"per-min");    tested++;
		}
		/* SMS flat fee (delta=0): calc_cprice_2 can't price this (ceil(x/0) ->
		 * divide-by-zero garbage), so the CTE intentionally diverges to a flat
		 * fee. Validate that directly instead of against the broken reference. */
		{
			double c; int b;
			cte(0,d4,f4,it4,1,&c,&b);
			if(fabs(c - 0.06) > 1e-9 || b != 0) {
				printf("FAIL [sms-flat] cte(price=%.4f,bsec=%d) expected 0.0600,0\n",c,b);
				fails++;
			}
			tested++;
		}
	}

	/* fuzz: representative tariffs (final step iterations=0) */
	{
		int dch[3] = {1,30,60};
		for(seed = 1; seed < 400000; seed++) {
			int delta[MAXP],iter[MAXP],i,n; double fee[MAXP];
			unsigned int st = (unsigned int)seed;
			n = 1 + rand_r(&st) % MAXP;
			for(i = 0; i < n; i++) {
				delta[i] = dch[rand_r(&st) % 3];
				fee[i]   = (rand_r(&st) % 500) / 100.0;
				iter[i]  = (i == n - 1) ? 0 : (1 + rand_r(&st) % 4);
			}
			bs = rand_r(&st) % 3601;
			fails += check(bs,delta,fee,iter,n,"fuzz");
			tested++;
		}
	}

	fails += test_free_split();

	printf("tariff_pricing: tested=%d  failures=%d  -> %s\n",
	       tested,fails,fails ? "FAIL" : "OK");
	return fails ? 1 : 0;
}
