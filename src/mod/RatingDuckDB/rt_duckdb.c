#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "rt_duckdb.h"

/* run a statement on the DuckDB engine; clear the native result afterwards */
static int rt_duckdb_exec(rt_duckdb_t *ctx,const char *sql)
{
	if(db_query(ctx->duck,(char *)sql,1) < 0) {
		LOG("rt_duckdb_exec()","error on: %s",sql);
		return -1;
	}

	return 0;
}

int rt_duckdb_init(rt_duckdb_t *ctx,const char *pg_host,const char *pg_dbname,
                   const char *pg_user,const char *pg_pass,int pg_port)
{
	char sql[1024];

	if(ctx == NULL) return -1;

	memset(ctx,0,sizeof(rt_duckdb_t));

	/* open an in-memory DuckDB through the db_duckdb engine (duckdb.so) */
	ctx->duck = db_init();
	if(ctx->duck == NULL) return -1;

	ctx->duck->conn = db_conn_init("duckdb",(char *)pg_host,":memory:",
	                               (unsigned short)pg_port,(char *)pg_user,(char *)pg_pass,0);
	if(ctx->duck->conn == NULL) return -1;

	if(db_engine_bind(ctx->duck) < 0) {
		LOG("rt_duckdb_init()","cannot bind duckdb engine (is duckdb.so loaded?)");
		return -1;
	}

	if(db_connect(ctx->duck) < 0) {
		LOG("rt_duckdb_init()","cannot open DuckDB");
		return -1;
	}

	/* build PostgreSQL connection string */
	snprintf(ctx->pg_connstr,sizeof(ctx->pg_connstr),
		"host=%s dbname=%s user=%s password=%s port=%d",
		pg_host,pg_dbname,pg_user,pg_pass,pg_port);

	/* install and load postgres_scanner */
	rt_duckdb_exec(ctx,"INSTALL postgres;");
	rt_duckdb_exec(ctx,"LOAD postgres;");

	/* attach PostgreSQL database */
	snprintf(sql,sizeof(sql),
		"ATTACH '%s' AS pg (TYPE POSTGRES, READ_ONLY);",ctx->pg_connstr);

	if(rt_duckdb_exec(ctx,sql) < 0) {
		LOG("rt_duckdb_init()","cannot attach PostgreSQL");
		return -2;
	}

	LOG("rt_duckdb_init()","DuckDB initialized, PostgreSQL attached");

	return 0;
}

void rt_duckdb_close(rt_duckdb_t *ctx)
{
	if(ctx == NULL || ctx->duck == NULL) return;

	rt_duckdb_exec(ctx,"DETACH pg;");

	db_close(ctx->duck);
	db_free(ctx->duck);
	ctx->duck = NULL;

	LOG("rt_duckdb_close()","DuckDB closed");
}

/* run a SELECT on DuckDB and hand back the marshaled result (rows of strings).
 * Caller must db_sql_result_free() it and null conn->result when done. */
static db_sql_result_t *rt_duckdb_select(rt_duckdb_t *ctx,const char *sql)
{
	if(db_query(ctx->duck,(char *)sql,0) < 0) return NULL;
	if(db_fetch(ctx->duck) < 0) return NULL;

	return (db_sql_result_t *)ctx->duck->conn->result;
}

/*
 * Account-resolution modes — mirrors rt_main -> rt_racc_* -> rt_data_q_racc_sql
 * in the Rating module. For each cdr_rec_type_id and leg, the engine tries a
 * sequence of lookups (calling_number, account_code, src/dst_context,
 * src/dst_tgroup, sms) in fallback order; the first that resolves an account
 * wins. We model that set-based: build every applicable lookup, tag it with the
 * fallback priority, and pick the lowest-priority match per CDR.
 *
 * rec types (cdr.h): unkn=0 isup=1 sms=2 voip_a=3 voip_v=4(not rated) voip_t=5.
 * For each mode the lookup table and the CDR match column share a name, and the
 * bill-plan column is bill_plan_id (or sm_bill_plan_id for SMS). tgroup modes
 * additionally match clg_nadi/cld_nadi.
 */
typedef struct rt_acct_mode {
	char        leg;       /* 'a' or 'b' */
	int         rec_type;  /* cdr_rec_type_id */
	int         prio;      /* fallback order (1 = tried first) */
	const char *tbl;       /* lookup table; CDR match column has the same name */
	const char *bpcol;     /* bill_plan_id | sm_bill_plan_id */
	int         nadi;      /* 1 -> also match clg_nadi/cld_nadi (tgroup modes) */
} rt_acct_mode_t;

static const rt_acct_mode_t rt_acct_modes[] = {
	/* leg a */
	{'a',0,1,"calling_number","bill_plan_id",   0},  /* unkn:  clg  */
	{'a',0,2,"account_code",  "bill_plan_id",   0},  /* unkn:  acode */
	{'a',0,3,"src_context",   "bill_plan_id",   0},  /* unkn:  srcc */
	{'a',0,4,"src_tgroup",    "bill_plan_id",   1},  /* unkn:  srctg */
	{'a',1,1,"src_tgroup",    "bill_plan_id",   1},  /* isup:  srctg */
	{'a',2,1,"calling_number","sm_bill_plan_id",0},  /* sms */
	{'a',3,1,"calling_number","bill_plan_id",   0},  /* voip_a: clg */
	{'a',3,2,"account_code",  "bill_plan_id",   0},  /* voip_a: acode */
	{'a',5,1,"account_code",  "bill_plan_id",   0},  /* voip_t: acode */
	{'a',5,2,"src_context",   "bill_plan_id",   0},  /* voip_t: srcc */
	/* leg b */
	{'b',0,1,"dst_context",   "bill_plan_id",   0},  /* unkn:  dstc */
	{'b',0,2,"dst_tgroup",    "bill_plan_id",   1},  /* unkn:  dsttg */
	{'b',1,1,"dst_tgroup",    "bill_plan_id",   1},  /* isup:  dsttg */
	{'b',2,1,"calling_number","sm_bill_plan_id",0},  /* sms */
	{'b',3,1,"calling_number","bill_plan_id",   0},  /* voip_a: clg */
	{'b',3,2,"account_code",  "bill_plan_id",   0},  /* voip_a: acode */
	{'b',5,1,"dst_context",   "bill_plan_id",   0},  /* voip_t: dstc */
	{0,0,0,NULL,NULL,0}
};

/* Build the UNION ALL of per-mode account lookups for the given leg into buf.
 * Returns the number of modes emitted (0 = none for this leg). */
static int rt_build_acct_union(char leg,char *buf,int buflen)
{
	int i,n = 0,len = 0;

	buf[0] = '\0';

	for(i = 0; rt_acct_modes[i].tbl != NULL; i++) {
		const rt_acct_mode_t *m = &rt_acct_modes[i];

		if(m->leg != leg) continue;

		len += snprintf(buf+len,buflen-len,
			"%s SELECT c.id AS cdr_id, %d AS prio, ba.id AS bacc_id, "
			"ba.round_mode_id, bp.id AS bplan_id "
			"FROM batch_window c "
			"JOIN pg.%s t ON t.%s = c.%s "
			"JOIN pg.%s_deff df ON df.%s_id = t.id "
			"JOIN pg.billing_account ba ON ba.id = t.billing_account_id "
			"  AND ba.cdr_server_id = c.cdr_server_id "
			"JOIN pg.bill_plan bp ON bp.id = df.%s "
			"WHERE c.cdr_rec_type_id = %d%s ",
			(n ? "UNION ALL" : ""),
			m->prio,
			m->tbl,m->tbl,m->tbl,
			m->tbl,m->tbl,
			m->bpcol,
			m->rec_type,
			(m->nadi ? " AND df.clg_nadi = c.clg_nadi AND df.cld_nadi = c.cld_nadi" : ""));

		if(len >= buflen) return -1;   /* truncated */
		n++;
	}

	return n;
}

/*
 * STEP 2.5 — balance accumulation (the period bill), mirrors rt_balance_exec.
 *
 * Balance is keyed by (billing_account_id, start_date, end_date, active='t')
 * and accumulates cprice. The period comes from the account's active pcard
 * (pcard_status_id=1):
 *   credit card (type 2) -> the billing_day monthly window (start = the most
 *     recent billing_day boundary <= call date, end = +1 month);
 *   debit card  (type 1) -> the card's own start_date/end_date, when it covers
 *     the call date.
 * Accounts with no active pcard get no balance (as in /Rating). We aggregate
 * SUM(cprice) per (account, period) and UPDATE-or-INSERT balance — incremental,
 * so it stays correct across the per-window batches.
 */
static void rt_duckdb_balance(rt_duckdb_t *ctx,db_t *pg_dbp)
{
	db_sql_result_t *bd;
	int i;

	if(rt_duckdb_exec(ctx,
		"CREATE OR REPLACE TEMP TABLE bal_delta AS "
		"WITH pc AS ( "
		"  SELECT rb.cdr_id, rb.bacc_id, rb.cprice, "
		"         CAST(rb.start_ts_str AS TIMESTAMP) AS cts, "
		"         COALESCE(NULLIF(TRY_CAST(ba.billing_day AS INTEGER),0),1) AS bday, "
		"         p.pcard_type_id, p.start_date AS cstart, p.end_date AS cend, "
		"         ROW_NUMBER() OVER (PARTITION BY rb.cdr_id ORDER BY p.start_date DESC) AS prn "
		"  FROM rated_batch rb "
		"  JOIN pg.billing_account ba ON ba.id = rb.bacc_id "
		"  JOIN pg.pcard p ON p.billing_account_id = rb.bacc_id AND p.pcard_status_id = 1 "
		"    AND ( p.pcard_type_id = 2 "
		"       OR (p.pcard_type_id = 1 "
		"           AND TRY_CAST(p.start_date AS DATE) <= CAST(rb.start_ts_str AS DATE) "
		"           AND TRY_CAST(p.end_date AS DATE) >  CAST(rb.start_ts_str AS DATE)) ) "
		"  WHERE rb.is_free = FALSE "   /* free portions are not charged to balance */
		"), "
		"per AS ( "
		"  SELECT bacc_id, cprice, pcard_type_id, cstart, cend, "
		"         CASE WHEN EXTRACT(DAY FROM cts) >= bday "
		"              THEN (date_trunc('month',cts) + to_days(bday-1))::DATE "
		"              ELSE (date_trunc('month',cts) - INTERVAL 1 MONTH + to_days(bday-1))::DATE END AS win_start "
		"  FROM pc WHERE prn = 1 "
		") "
		"SELECT bacc_id, "
		"  CAST(CASE WHEN pcard_type_id = 2 THEN win_start ELSE TRY_CAST(cstart AS DATE) END AS VARCHAR) AS start_date, "
		"  CAST(CASE WHEN pcard_type_id = 2 THEN (win_start + INTERVAL 1 MONTH)::DATE ELSE TRY_CAST(cend AS DATE) END AS VARCHAR) AS end_date, "
		"  SUM(cprice) AS amount "
		"FROM per GROUP BY ALL") < 0) {
		LOG("rt_duckdb_balance()","build bal_delta failed");
		return;
	}

	bd = rt_duckdb_select(ctx,"SELECT bacc_id, start_date, end_date, amount FROM bal_delta");
	if(bd == NULL) return;

	for(i = 0; i < bd->rows; i++) {
		char up[2048];
		long long bacc = atoll(bd->cols_list[0].rows_list[i].row);
		char *s        = bd->cols_list[1].rows_list[i].row;
		char *e        = bd->cols_list[2].rows_list[i].row;
		double amt     = atof(bd->cols_list[3].rows_list[i].row);

		/* update the period's balance, or create it if absent (one statement) */
		snprintf(up,sizeof(up),
			"WITH upd AS ( "
			"  UPDATE balance SET amount = amount + %f, last_update = now() "
			"  WHERE billing_account_id = %lld AND start_date = '%s' AND end_date = '%s' AND active = 't' "
			"  RETURNING id "
			") "
			"INSERT INTO balance (billing_account_id,start_date,end_date,active,amount,last_update) "
			"SELECT %lld,'%s','%s','t',%f,now() WHERE NOT EXISTS (SELECT 1 FROM upd)",
			amt,bacc,s,e,bacc,s,e,amt);

		db_query(pg_dbp,up,1);
	}

	LOG("rt_duckdb_balance()","balance updated for %d account-periods",bd->rows);

	db_sql_result_free(bd);
	ctx->duck->conn->result = NULL;
}

/*
 * The big query:
 * 1. Pin a deterministic window of unrated CDRs from PostgreSQL (postgres_scanner)
 * 2. JOIN with subscriber, billing plan (via bill_plan_tree), rates, tariff
 * 3. Calculate prices in DuckDB SQL
 * 4. Write rated rows back to PostgreSQL, mark the rest of the window as -1
 */
int rt_duckdb_rate_batch(rt_duckdb_t *ctx,db_t *pg_dbp,char leg,int limit)
{
	char sql[16384];
	char acct_union[6144];
	int row_count = 0;
	int rated = 0;
	long long max_window_id = 0;

	db_sql_result_t *rb;

	if(ctx == NULL || pg_dbp == NULL) return 0;

	/*
	 * STEP 0: pin a deterministic batch window (the lowest `limit` unrated CDR
	 * ids). Both the rating JOIN and the unmatched (-1) marking work on exactly
	 * this set, so we never mark a CDR we did not actually evaluate.
	 */
	snprintf(sql,sizeof(sql),
		"CREATE OR REPLACE TEMP TABLE batch_window AS "
		"SELECT id, calling_number, called_number, billsec, billusec, start_ts, cdr_server_id, "
		"cdr_rec_type_id, account_code, src_context, dst_context, src_tgroup, dst_tgroup, "
		"clg_nadi, cld_nadi "
		"FROM pg.cdrs WHERE leg_%c = 0 ORDER BY id LIMIT %d",
		leg,limit);

	if(rt_duckdb_exec(ctx,sql) < 0) {
		LOG("rt_duckdb_rate_batch()","build batch_window failed");
		return -1;
	}

	/* window bounds: is there work left, and the max id to scope the -1 mark */
	{
		db_sql_result_t *wr =
			rt_duckdb_select(ctx,"SELECT COUNT(*), COALESCE(MAX(id),0) FROM batch_window");
		long long win_count;

		if(wr == NULL || wr->rows < 1) {
			if(wr != NULL) { db_sql_result_free(wr); ctx->duck->conn->result = NULL; }
			return -1;
		}

		win_count     = atoll(wr->cols_list[0].rows_list[0].row);
		max_window_id = atoll(wr->cols_list[1].rows_list[0].row);

		db_sql_result_free(wr);
		ctx->duck->conn->result = NULL;

		if(win_count == 0) return 0;   /* backlog drained */
	}

	/*
	 * STEP 1: resolve each CDR's account via the applicable lookup modes
	 * (calling_number / account_code / contexts / tgroups / sms) in fallback
	 * priority, then match rates through bill_plan_tree (COALESCE falls back to
	 * the plan itself for flat plans) and the longest matching prefix.
	 */
	if(rt_build_acct_union(leg,acct_union,sizeof(acct_union)) <= 0) {
		LOG("rt_duckdb_rate_batch()","no account modes for leg %c",leg);
		return -1;
	}

	snprintf(sql,sizeof(sql),
		"CREATE OR REPLACE TEMP TABLE rated_batch AS "
		"WITH RECURSIVE acct_all AS ( %s ), "
		"acct AS ( "
		"  SELECT cdr_id, bacc_id, round_mode_id, bplan_id FROM ( "
		"    SELECT *, ROW_NUMBER() OVER (PARTITION BY cdr_id ORDER BY prio) AS arn FROM acct_all "
		"  ) WHERE arn = 1 "
		"), "
		"matched AS ( "
		"  SELECT "
		"    c.id AS cdr_id, c.calling_number, c.called_number, "
		/* round_billsec: round_mode 1=ceil, 2=floor of billusec; only override
		 * billsec when the rounded value is nonzero (matches round_billsec.c). */
		"    CASE "
		"      WHEN a.round_mode_id = 1 AND CAST(CEIL(c.billusec / 1000000.0) AS BIGINT) <> 0 "
		"        THEN CAST(CEIL(c.billusec / 1000000.0) AS BIGINT) "
		"      WHEN a.round_mode_id = 2 AND CAST(FLOOR(c.billusec / 1000000.0) AS BIGINT) <> 0 "
		"        THEN CAST(FLOOR(c.billusec / 1000000.0) AS BIGINT) "
		"      ELSE c.billsec END AS billsec, "
		"    c.start_ts, "
		"    a.bacc_id, a.round_mode_id, "
		"    rt.id AS rate_id, rt.tariff_id, pr.prefix, pr.id AS prefix_id, "
		"    ROW_NUMBER() OVER (PARTITION BY c.id ORDER BY LENGTH(pr.prefix) DESC) AS rn "
		"  FROM batch_window c "
		"  JOIN acct a ON a.cdr_id = c.id "
		"  LEFT JOIN pg.bill_plan_tree tree ON tree.root_bplan_id = a.bplan_id "
		"  JOIN pg.rate rt ON rt.bill_plan_id = COALESCE(tree.bill_plan_id, a.bplan_id) "
		"  JOIN pg.prefix pr ON pr.id = rt.prefix_id "
		"    AND c.called_number LIKE pr.prefix || '%%' "
		"  JOIN pg.tariff tr ON tr.id = rt.tariff_id "
		"), "
		/*
		 * Time conditions (tc_ts_cmp): a tariff that has time_condition rows only
		 * applies if start_ts matches one — by tc_date, or day-of-week range, each
		 * optionally gated by an hours range (with midnight wrap). A tariff with no
		 * conditions always applies (so this is a no-op when unused). Match factors
		 * to: hours_match AND (tc_date matches OR isodow in [d1,d2]).
		 */
		"best_match AS ( "
		"  SELECT m.* FROM matched m WHERE m.rn = 1 AND ( "
		"    NOT EXISTS (SELECT 1 FROM pg.time_condition tc WHERE tc.tariff_id = m.tariff_id) "
		"    OR EXISTS ( "
		"      SELECT 1 FROM pg.time_condition tc "
		"      JOIN pg.time_condition_deff df ON df.id = tc.time_condition_id "
		"      WHERE tc.tariff_id = m.tariff_id "
		/* hours compared lexically vs the call time string, like tc_ts_cmp's
		 * strcmp (df.hours may be "HH-HH" or "HH:MM:SS-HH:MM:SS"). */
		"      AND ( COALESCE(df.hours,'') = '' "
		"         OR (split_part(df.hours,'-',1) <  split_part(df.hours,'-',2) "
		"             AND split_part(df.hours,'-',1) <  CAST(CAST(m.start_ts AS TIME) AS VARCHAR) "
		"             AND split_part(df.hours,'-',2) >= CAST(CAST(m.start_ts AS TIME) AS VARCHAR)) "
		"         OR (split_part(df.hours,'-',1) >= split_part(df.hours,'-',2) "
		"             AND (split_part(df.hours,'-',1) <  CAST(CAST(m.start_ts AS TIME) AS VARCHAR) "
		"               OR split_part(df.hours,'-',2) >= CAST(CAST(m.start_ts AS TIME) AS VARCHAR))) ) "
		"      AND ( (COALESCE(df.tc_date,'') <> '' AND TRY_CAST(df.tc_date AS DATE) = CAST(m.start_ts AS DATE)) "
		"         OR isodow(m.start_ts) BETWEEN "
		"            CASE lower(split_part(df.days_week,'-',1)) WHEN 'mon' THEN 1 WHEN 'tue' THEN 2 WHEN 'wed' THEN 3 WHEN 'thu' THEN 4 WHEN 'fri' THEN 5 WHEN 'sat' THEN 6 WHEN 'sun' THEN 7 END "
		"        AND CASE lower(split_part(df.days_week,'-',2)) WHEN 'mon' THEN 1 WHEN 'tue' THEN 2 WHEN 'wed' THEN 3 WHEN 'thu' THEN 4 WHEN 'fri' THEN 5 WHEN 'sat' THEN 6 WHEN 'sun' THEN 7 END ) "
		"    ) "
		"  ) "
		"), "
		/*
		 * Multi-tier pricing — faithful set-based replica of calc_cprice_2:
		 * walk the tariff steps (calc_function by pos) carrying remaining seconds
		 * (rem), accumulating price and billed seconds (bsec). Per step, with
		 * u = ceil(rem/delta) blocks needed (delta=0 -> SMS flat: 1 block):
		 *   charge = (iterations=0 OR u<=iterations) ? u : iterations
		 *   done   = iterations=0 OR delta=0 OR u=1
		 * (the u=1-only break is calc_cprice_2's exact control flow). The free/
		 * paid split that yields negative prices is free_billsec (a later phase).
		 */
		/*
		 * Phase D — free_billsec split (double_rating). A tariff with a free
		 * allowance (free_billsec via tariff.free_billsec_id) gives each subscriber
		 * a pool of free seconds per balance period. Calls draw from it in id order
		 * (as /Rating fetches), seeded from history (prior free seconds in this
		 * period = rating rows with call_price < 0). Per call:
		 *   before = history_used + running SUM(billsec of earlier calls in period)
		 *   free_sec = clamp(0, billsec, limit - before)   [== sequential drawdown]
		 * A boundary call splits into a free portion (negative price) and a paid
		 * portion -> two rating rows. Each portion is priced by the tier walk.
		 */
		"fb AS ( "
		"  SELECT bm.cdr_id, bm.billsec, bm.start_ts, bm.bacc_id, bm.rate_id, bm.tariff_id, "
		"         t.free_billsec_id AS fbid, COALESCE(f.free_billsec,0) AS fb_limit, "
		"         CASE WHEN EXTRACT(DAY FROM bm.start_ts) >= (COALESCE(NULLIF(TRY_CAST(ba.billing_day AS INTEGER),0),1)) "
		"              THEN (date_trunc('month',bm.start_ts) + to_days((COALESCE(NULLIF(TRY_CAST(ba.billing_day AS INTEGER),0),1))-1))::DATE "
		"              ELSE (date_trunc('month',bm.start_ts) - INTERVAL 1 MONTH + to_days((COALESCE(NULLIF(TRY_CAST(ba.billing_day AS INTEGER),0),1))-1))::DATE END AS pstart "
		"  FROM best_match bm "
		"  JOIN pg.tariff t ON t.id = bm.tariff_id "
		"  JOIN pg.billing_account ba ON ba.id = bm.bacc_id "
		"  LEFT JOIN pg.free_billsec f ON f.id = t.free_billsec_id "
		"), "
		"hist AS ( "
		"  SELECT r.billing_account_id AS bacc_id, r.free_billsec_id AS fbid, "
		"         CASE WHEN EXTRACT(DAY FROM r.call_ts) >= (COALESCE(NULLIF(TRY_CAST(ba.billing_day AS INTEGER),0),1)) "
		"              THEN (date_trunc('month',r.call_ts) + to_days((COALESCE(NULLIF(TRY_CAST(ba.billing_day AS INTEGER),0),1))-1))::DATE "
		"              ELSE (date_trunc('month',r.call_ts) - INTERVAL 1 MONTH + to_days((COALESCE(NULLIF(TRY_CAST(ba.billing_day AS INTEGER),0),1))-1))::DATE END AS pstart, "
		"         SUM(r.call_billsec) AS used "
		"  FROM pg.rating r "
		"  JOIN pg.billing_account ba ON ba.id = r.billing_account_id "
		"  WHERE r.call_price < 0 AND r.free_billsec_id <> 0 "
		"    AND r.billing_account_id IN (SELECT DISTINCT bacc_id FROM fb WHERE fbid <> 0 AND fb_limit > 0) "
		"  GROUP BY 1,2,3 "
		"), "
		"split AS ( "
		"  SELECT fb.cdr_id, fb.billsec, fb.start_ts, fb.bacc_id, fb.rate_id, fb.tariff_id, fb.fbid, "
		"         GREATEST(0, LEAST(fb.billsec, fb.fb_limit - (COALESCE(h.used,0) "
		"           + COALESCE(SUM(fb.billsec) OVER (PARTITION BY fb.bacc_id, fb.fbid, fb.pstart "
		"               ORDER BY fb.cdr_id ROWS BETWEEN UNBOUNDED PRECEDING AND 1 PRECEDING),0)))) AS free_sec "
		"  FROM fb LEFT JOIN hist h ON h.bacc_id=fb.bacc_id AND h.fbid=fb.fbid AND h.pstart=fb.pstart "
		"  WHERE fb.fbid <> 0 AND fb.fb_limit > 0 "
		"), "
		"portions AS ( "
		"  SELECT cdr_id, 'P' AS kind, billsec AS sec, tariff_id, 1 AS sgn, CAST(0 AS BIGINT) AS fbid_out, start_ts, bacc_id, rate_id "
		"  FROM fb WHERE NOT (fbid <> 0 AND fb_limit > 0) "
		"  UNION ALL "
		"  SELECT cdr_id, 'P', (billsec - free_sec), tariff_id, 1, CAST(0 AS BIGINT), start_ts, bacc_id, rate_id "
		"  FROM split WHERE (billsec - free_sec) > 0 "
		"  UNION ALL "
		"  SELECT cdr_id, 'F', free_sec, tariff_id, -1, CAST(fbid AS BIGINT), start_ts, bacc_id, rate_id "
		"  FROM split WHERE free_sec > 0 "
		"), "
		"price_walk(cdr_id, kind, pos, rem, price, bsec, done) AS ( "
		"  SELECT CAST(cdr_id AS BIGINT), kind, CAST(pos AS INTEGER), "
		"         CAST(cs - charge*d AS BIGINT), CAST(charge*f AS DOUBLE), CAST(charge*d AS BIGINT), "
		"         CAST(CASE WHEN it=0 OR d=0 THEN 1 WHEN u=1 THEN 1 ELSE 0 END AS INTEGER) "
		"  FROM ( "
		"    SELECT cdr_id, kind, pos, d, f, it, cs, u, "
		"           CASE WHEN it=0 OR u<=it THEN u ELSE it END AS charge "
		"    FROM ( "
		"      SELECT pt.cdr_id, pt.kind, cf.pos, cf.delta_time AS d, cf.fee AS f, cf.iterations AS it, "
		"             pt.sec AS cs, "
		"             CASE WHEN cf.delta_time=0 THEN 1 ELSE CAST(CEIL(pt.sec::DOUBLE/cf.delta_time) AS BIGINT) END AS u "
		"      FROM portions pt "
		"      JOIN pg.calc_function cf ON cf.tariff_id = pt.tariff_id AND cf.pos = 1 "
		"    ) "
		"  ) "
		"  UNION ALL "
		"  SELECT CAST(cdr_id AS BIGINT), kind, CAST(pos AS INTEGER), "
		"         CAST(cs - charge*d AS BIGINT), CAST(pprice + charge*f AS DOUBLE), "
		"         CAST(pbsec + charge*d AS BIGINT), "
		"         CAST(CASE WHEN it=0 OR d=0 THEN 1 WHEN u=1 THEN 1 ELSE 0 END AS INTEGER) "
		"  FROM ( "
		"    SELECT cdr_id, kind, pos, d, f, it, cs, u, pprice, pbsec, "
		"           CASE WHEN it=0 OR u<=it THEN u ELSE it END AS charge "
		"    FROM ( "
		"      SELECT p.cdr_id, p.kind, cf.pos, cf.delta_time AS d, cf.fee AS f, cf.iterations AS it, "
		"             p.rem AS cs, p.price AS pprice, p.bsec AS pbsec, "
		"             CASE WHEN cf.delta_time=0 THEN 1 ELSE CAST(CEIL(p.rem::DOUBLE/cf.delta_time) AS BIGINT) END AS u "
		"      FROM price_walk p "
		"      JOIN portions pt ON pt.cdr_id = p.cdr_id AND pt.kind = p.kind "
		"      JOIN pg.calc_function cf ON cf.tariff_id = pt.tariff_id AND cf.pos = p.pos + 1 "
		"      WHERE p.done = 0 "
		"    ) "
		"  ) "
		"), "
		"final AS ( "
		"  SELECT cdr_id, kind, price AS cprice, bsec AS billsec_out, "
		"         ROW_NUMBER() OVER (PARTITION BY cdr_id, kind ORDER BY pos DESC) AS frn "
		"  FROM price_walk "
		") "
		"SELECT CAST(pt.cdr_id AS BIGINT) AS cdr_id, "
		"  CAST(fp.billsec_out AS BIGINT) AS billsec, "
		"  CAST(pt.start_ts AS VARCHAR) AS start_ts_str, "
		"  CAST(pt.bacc_id AS BIGINT) AS bacc_id, "
		"  CAST(pt.rate_id AS BIGINT) AS rate_id, "
		"  (pt.sgn * fp.cprice) AS cprice, "
		"  CAST(pt.fbid_out AS BIGINT) AS free_billsec_id, "
		"  (pt.kind = 'F') AS is_free "
		"FROM portions pt "
		"JOIN final fp ON fp.cdr_id = pt.cdr_id AND fp.kind = pt.kind AND fp.frn = 1",
		acct_union);

	if(rt_duckdb_exec(ctx,sql) < 0) {
		LOG("rt_duckdb_rate_batch()","build rated_batch failed");
		return -1;
	}

	rb = rt_duckdb_select(ctx,"SELECT * FROM rated_batch");
	row_count = (rb != NULL) ? rb->rows : 0;

	LOG("rt_duckdb_rate_batch()","matched: %d CDRs (window <= id %lld)",
		row_count,(long long)max_window_id);

	/*
	 * STEP 2: Bulk INSERT rated rows into PostgreSQL and set leg = rating_id.
	 * Skipped when nothing matched - the whole window then falls through to -1.
	 */
	if(row_count > 0) {
		char *insert_sql;
		char row_buf[512];
		char safe_ts[64];
		int first = 1;
		int i;

		insert_sql = (char *)mem_alloc(row_count * 300 + 1024);
		if(insert_sql == NULL) {
			db_sql_result_free(rb);
			ctx->duck->conn->result = NULL;
			return -1;
		}

		strcpy(insert_sql,
			"INSERT INTO rating"
			" (call_price,call_billsec,rate_id,billing_account_id,call_id,"
			"rating_mode_id,pcard_id,time_condition_id,call_ts,last_update,free_billsec_id)"
			" VALUES ");

		for(i = 0; i < row_count; i++) {
			/* rated_batch cols: 0 cdr_id,1 billsec,2 start_ts,3 bacc_id,
			 * 4 rate_id,5 cprice(signed),6 free_billsec_id,7 is_free */
			long long cdr_id  = atoll(rb->cols_list[0].rows_list[i].row);
			int billsec       = atoi(rb->cols_list[1].rows_list[i].row);
			char *ts          = rb->cols_list[2].rows_list[i].row;
			long long bacc_id = atoll(rb->cols_list[3].rows_list[i].row);
			long long rate_id = atoll(rb->cols_list[4].rows_list[i].row);
			double cprice     = atof(rb->cols_list[5].rows_list[i].row);
			long long fbid    = atoll(rb->cols_list[6].rows_list[i].row);

			memset(safe_ts,0,sizeof(safe_ts));
			if(ts != NULL && ts[0] != '\0') {
				db_sql_escape(ts,safe_ts,sizeof(safe_ts));
			} else {
				strcpy(safe_ts,"1970-01-01 00:00:00");
			}

			snprintf(row_buf,sizeof(row_buf),
				"%s(%f,%d,%lld,%lld,%lld,1,0,0,'%s','now()',%lld)",
				(first ? "" : ","),
				cprice,billsec,rate_id,bacc_id,cdr_id,safe_ts,fbid);

			strcat(insert_sql,row_buf);
			first = 0;
		}

		strcat(insert_sql," RETURNING id, call_id");

		/* done reading the DuckDB result */
		db_sql_result_free(rb);
		ctx->duck->conn->result = NULL;
		rb = NULL;

		/* execute on PostgreSQL */
		db_query(pg_dbp,insert_sql,0);
		db_fetch(pg_dbp);

		/* build CDR update from RETURNING result */
		if(pg_dbp->conn->result != NULL) {
			db_sql_result_t *pg_result = (db_sql_result_t *)pg_dbp->conn->result;

			if(pg_result->rows > 0) {
				char *update_sql = (char *)mem_alloc(pg_result->rows * 40 + 256);
				if(update_sql != NULL) {
					int j;

					sprintf(update_sql,
						"UPDATE cdrs SET leg_%c = v.rid FROM (VALUES ",leg);

					for(j = 0; j < pg_result->rows; j++) {
						char upd_row[64];
						long long rating_id = atoll(pg_result->cols_list[0].rows_list[j].row);
						long long cdr_id    = atoll(pg_result->cols_list[1].rows_list[j].row);

						snprintf(upd_row,sizeof(upd_row),"%s(%lld,%lld)",
							(j > 0 ? "," : ""),cdr_id,rating_id);
						strcat(update_sql,upd_row);
					}

					strcat(update_sql,") AS v(cid,rid) WHERE cdrs.id = v.cid");

					db_query(pg_dbp,update_sql,1);
					rated = pg_result->rows;

					mem_free(update_sql);
				}
			}

			db_sql_result_free(pg_result);
			pg_dbp->conn->result = NULL;
		}

		mem_free(insert_sql);
	} else if(rb != NULL) {
		db_sql_result_free(rb);
		ctx->duck->conn->result = NULL;
	}

	/* STEP 2.5: accumulate the rated cprice into the period balance (the bill). */
	if(rated > 0) rt_duckdb_balance(ctx,pg_dbp);

	/*
	 * STEP 3: Mark the evaluated-but-unrated CDRs in this window as leg = -1.
	 * Scoped to id <= max_window_id (the window we just rated against), so we
	 * never mark a CDR that was never evaluated. Matched CDRs are already
	 * leg > 0 and excluded by the leg = 0 predicate.
	 */
	{
		char mark_sql[256];
		snprintf(mark_sql,sizeof(mark_sql),
			"UPDATE cdrs SET leg_%c = -1 WHERE leg_%c = 0 AND id <= %lld",
			leg,leg,(long long)max_window_id);

		db_query(pg_dbp,mark_sql,1);

		LOG("rt_duckdb_rate_batch()","marked unmatched CDRs as -1 (id <= %lld)",
			(long long)max_window_id);
	}

	LOG("rt_duckdb_rate_batch()","batch complete: %d rated",rated);

	return rated;
}
