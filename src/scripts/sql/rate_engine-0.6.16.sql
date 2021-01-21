--
-- rate_engine , 0.6.16
--

CREATE TABLE account_code (
    id serial PRIMARY KEY NOT NULL,
    account_code character varying(64) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);


CREATE TABLE account_code_deff (
    id serial PRIMARY KEY NOT NULL,
    account_code_id integer NOT NULL,
    bill_plan_id integer NOT NULL
);


--
-- 'balance' table
--
CREATE TABLE balance (
    id bigserial PRIMARY KEY NOT NULL,
    billing_account_id integer NOT NULL,
    amount numeric DEFAULT 0,
    last_update timestamp without time zone,
    start_date character varying(32),
    end_date character varying(32),
    active boolean DEFAULT false,
    last_update_flag timestamp without time zone
);

CREATE INDEX bal_bacc_id_ind ON balance USING btree (billing_account_id);
CREATE INDEX bal_startd_ind  ON balance USING btree (start_date);


--
-- 'bill_plan' table
--
CREATE TABLE bill_plan (
    id serial PRIMARY KEY NOT NULL,
    name character varying(128) UNIQUE NOT NULL,
    bill_plan_type_id integer DEFAULT 2 NOT NULL,
    start_period integer DEFAULT 0,
    end_period integer DEFAULT 0
);


--
-- 'bill_plan_tree' table
--
CREATE TABLE bill_plan_tree (
    id serial PRIMARY KEY NOT NULL,
    bill_plan_id integer NOT NULL,
    root_bplan_id integer NOT NULL
);

CREATE INDEX tree_root_id_ind ON bill_plan_tree USING btree (root_bplan_id);


-- ???
CREATE TABLE bill_plan_type (
    id serial PRIMARY KEY NOT NULL,
    name character varying(32) UNIQUE NOT NULL
);


CREATE TABLE billing_account (
    id bigserial PRIMARY KEY NOT NULL,
    username character varying(64) UNIQUE NOT NULL,
    currency_id integer DEFAULT 1 NOT NULL,
    leg character varying(3) DEFAULT 'a'::character varying NOT NULL,
    cdr_server_id integer DEFAULT 0 NOT NULL,
    billing_day character varying(2) DEFAULT '01'::character varying,
    round_mode_id integer DEFAULT 0,
    day_of_payment integer DEFAULT 0
);


--
-- 'calc_function' table
--
CREATE TABLE calc_function (
    id serial PRIMARY KEY NOT NULL,
    tariff_id integer NOT NULL,
    pos integer NOT NULL,
    delta_time integer NOT NULL,
    fee numeric NOT NULL,
    iterations integer
);

CREATE INDEX calc_tariff_id_inx ON calc_function USING btree (tariff_id);

--
-- 'calling_number' table
--
CREATE TABLE calling_number (
    id bigserial PRIMARY KEY NOT NULL,
    calling_number character varying(80) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);

CREATE INDEX clg_bacc_id_inx ON public.calling_number USING btree (billing_account_id);

--
-- 'calling_number_deff' table
--
CREATE TABLE calling_number_deff (
    id bigserial PRIMARY KEY NOT NULL,
    calling_number_id integer NOT NULL,
    bill_plan_id integer NOT NULL,
    sm_bill_plan_id integer DEFAULT 0
);

CREATE INDEX clg_deff_clg_id_inx ON calling_number_deff USING btree (calling_number_id);



CREATE TABLE cdr_dbstorage (
    id serial PRIMARY KEY  NOT NULL,
    cdr_server_id integer NOT NULL,
    dbhost character varying(128),
    dbname character varying(128),
    dbuser character varying(128),
    dbpass character varying(128),
    cdr_table character varying(128) NOT NULL,
    dbstorage_type_id integer NOT NULL
);


CREATE TABLE cdr_files (
    id bigserial PRIMARY KEY NOT NULL,
    cdr_server_id integer NOT NULL,
    filename character varying(128) NOT NULL,
    last_update timestamp without time zone
);


--
-- 'get_mode' table
--
CREATE TABLE cdr_get_mode (
    id serial PRIMARY KEY  NOT NULL,
    name character varying(4) UNIQUE NOT NULL
);

COPY cdr_get_mode (id, name) FROM stdin;
1	db
2	file
\.


CREATE TABLE cdr_profiles (
    id serial PRIMARY KEY NOT NULL,
    profile_name character varying(32) UNIQUE NOT NULL,
    profile_version integer DEFAULT 0
);


CREATE TABLE cdr_servers (
    id serial PRIMARY KEY NOT NULL,
    cdr_profiles_id integer NOT NULL,
    get_mode_id integer NOT NULL,
    active boolean
);


CREATE TABLE cdr_storage_sched (
    ts bigint DEFAULT (0)::bigint NOT NULL,
    cdr_server_id integer NOT NULL,
    last_chk_ts bigint DEFAULT (0)::bigint NOT NULL,
    start_ts bigint NOT NULL,
    replies bigint DEFAULT (0)::bigint NOT NULL
);

CREATE INDEX cdr_storage_sched_uniq ON cdr_storage_sched USING btree (cdr_server_id);

--
-- 'cdrs' table
--
CREATE TABLE public.cdrs (
    id bigserial PRIMARY KEY NOT NULL,
    cdr_server_id integer NOT NULL,
    call_uid character varying(128) NOT NULL,
    leg_a integer DEFAULT 0 NOT NULL,
    leg_b integer DEFAULT 0 NOT NULL,
    start_ts timestamp with time zone NOT NULL,
    src character varying(80),
    dst character varying(80) NOT NULL,
    prefix_filter_id integer,
    calling_number character varying(80),
    called_number character varying(80) NOT NULL,
    clg_nadi integer DEFAULT 0 NOT NULL,
    cld_nadi integer DEFAULT 0 NOT NULL,
    src_context character varying(80),
    dst_context character varying(80),
    billsec integer NOT NULL,
    duration integer NOT NULL,
    account_code character varying(80),
    src_tgroup character varying(80),
    dst_tgroup character varying(80),
    uduration bigint DEFAULT (0)::bigint NOT NULL,
    billusec bigint DEFAULT (0)::bigint NOT NULL,
    cdr_rec_type_id integer DEFAULT 0,
    rdnis character varying(32),
    rdnis_nadi integer DEFAULT 0,
    ocn character varying(32),
    ocn_nadi integer DEFAULT 0,
    start_epoch integer DEFAULT 0,
    answer_epoch integer DEFAULT 0,
    end_epoch integer DEFAULT 0,
    answer_ts character varying(32),
    end_ts character varying(32)
);

CREATE INDEX leg_a_ind   ON cdrs USING btree (leg_a);
CREATE INDEX leg_b_ind   ON cdrs USING btree (leg_b);
CREATE INDEX cdrs_ts_ind ON cdrs USING btree (start_ts);


CREATE TABLE clg_history (
    id bigserial PRIMARY KEY NOT NULL,
    calling_number character varying(80),
    billing_account_id integer,
    bill_plan_id integer,
    last_update timestamp without time zone,
    operation character varying(64)
);


--
-- 'currency' table
--
CREATE TABLE currency (
    id serial PRIMARY KEY NOT NULL,
    name character varying(12) UNIQUE NOT NULL,
    to_bg numeric DEFAULT 0
);

COPY public.currency (id, name, to_bg) FROM stdin;
1	лв.	0
2	евро	1.95583
\.

CREATE TABLE db_screenshot (
    id serial PRIMARY KEY NOT NULL,
    tbl_name character varying(64) UNIQUE NOT NULL,
    last_tbl_operation character(1),
    last_update timestamp without time zone,
    last_update_ts integer NOT NULL
);


CREATE TABLE dbstorage_type (
    id serial PRIMARY KEY NOT NULL,
    name character varying(16) UNIQUE NOT NULL
);



CREATE TABLE dst_context (
    id serial PRIMARY KEY NOT NULL,
    dst_context character varying(64) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);


CREATE TABLE dst_context_deff (
    id serial PRIMARY KEY NOT NULL,
    dst_context_id integer NOT NULL,
    bill_plan_id integer NOT NULL
);


CREATE TABLE dst_tgroup (
    id serial PRIMARY KEY NOT NULL,
    dst_tgroup character varying(80) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);


CREATE TABLE dst_tgroup_deff (
    id integer NOT NULL,
    dst_tgroup_id integer NOT NULL,
    bill_plan_id integer NOT NULL,
    clg_nadi integer DEFAULT 0 NOT NULL,
    cld_nadi integer DEFAULT 0 NOT NULL
);


CREATE TABLE free_billsec (
    id serial PRIMARY KEY NOT NULL,
    free_billsec integer NOT NULL
);


--
-- 'free_billsec_balance' table
--
CREATE TABLE free_billsec_balance (
    id bigserial PRIMARY KEY NOT NULL,
    balance_id integer NOT NULL,
    tariff_id integer DEFAULT 0 NOT NULL,
    free_billsec integer DEFAULT 0 NOT NULL,
    last_update timestamp without time zone DEFAULT now(),
    free_billsec_id integer DEFAULT 0
);

CREATE INDEX free_billsec_bal_inx   ON free_billsec_balance USING btree (free_billsec_id);
CREATE INDEX free_billsec_bal_inx_2 ON free_billsec_balance USING btree (balance_id);


--- ???
CREATE TABLE free_call_price (
    id bigint,
    billing_account_id integer,
    free_call_price numeric
);


--
-- 'pcard' table
--
CREATE TABLE pcard (
    id bigserial PRIMARY KEY NOT NULL,
    amount numeric DEFAULT (0)::numeric NOT NULL,
    start_date character varying(10),
    end_date character varying(10),
    last_update timestamp without time zone DEFAULT now(),
    pcard_status_id integer DEFAULT 0 NOT NULL,
    billing_account_id integer DEFAULT 0 NOT NULL,
    pcard_type_id integer DEFAULT 2 NOT NULL,
    call_number integer DEFAULT 1 NOT NULL,
    saved_amount numeric DEFAULT (0)::numeric NOT NULL,
    sim integer DEFAULT 0 NOT NULL
);

CREATE INDEX pcard_bacc_id_inx ON pcard USING btree (billing_account_id);

-- 
-- 'pcard_status' table
--
CREATE TABLE pcard_status (
    id serial PRIMARY KEY NOT NULL,
    status character varying(64) UNIQUE NOT NULL
);

COPY pcard_status (id, status) FROM stdin;
0	deactive
1	active
2	block
\.


--
-- 'pcard_type' table
--
CREATE TABLE pcard_type (
    id serial PRIMARY KEY NOT NULL,
    name character varying(32) UNIQUE NOT NULL,
    "desc" text
);

COPY pcard_type (id, name, "desc") FROM stdin;
1	debit card	It's prepaid service.\nFirst push money,after speak
2	credit card	It's postpaid service.\nThe amount is credit limit.
\.

CREATE TABLE prefix (
    id bigserial PRIMARY KEY NOT NULL,
    prefix character varying(32) UNIQUE NOT NULL,
    comm text
);


CREATE TABLE prefix_filter (
    id serial PRIMARY KEY NOT NULL,
    filtering_prefix character varying(32),
    filtering_number integer,
    replace_str character varying(32),
    cdr_server_id integer DEFAULT 0,
    len integer DEFAULT 0,
    cdr_profiles_id integer
);


-- 'rate' table

CREATE TABLE rate (
    id bigserial PRIMARY KEY NOT NULL,
    bill_plan_id integer NOT NULL,
    tariff_id integer NOT NULL,
    prefix_id integer
);


CREATE INDEX rate_bplan_id_ind ON rate USING btree (bill_plan_id);
CREATE INDEX rate_pr_id_ind    ON rate USING btree (prefix_id);


CREATE RULE db_screenshot_d AS
    ON DELETE TO rate DO ( DELETE FROM db_screenshot
  WHERE ((db_screenshot.tbl_name)::text = 'rate'::text);
 INSERT INTO db_screenshot (tbl_name, last_tbl_operation, last_update, last_update_ts)  SELECT 'rate',
            'D',
            now() AS now,
            date_part('epoch'::text, now()) AS date_part;
);


CREATE RULE db_screenshot_i AS
    ON INSERT TO rate DO ( DELETE FROM db_screenshot
  WHERE ((db_screenshot.tbl_name)::text = 'rate'::text);
 INSERT INTO db_screenshot (tbl_name, last_tbl_operation, last_update, last_update_ts)  SELECT 'rate',
            'I',
            now() AS now,
            date_part('epoch'::text, now()) AS date_part;
);


CREATE RULE db_screenshot_u AS
    ON UPDATE TO rate DO ( DELETE FROM db_screenshot
  WHERE ((db_screenshot.tbl_name)::text = 'rate'::text);
 INSERT INTO db_screenshot (tbl_name, last_tbl_operation, last_update, last_update_ts)  SELECT 'rate',
            'U',
            now() AS now,
            date_part('epoch'::text, now()) AS date_part;
);

--
-- 'rating' table 
--
CREATE TABLE rating (
    id bigserial PRIMARY KEY  NOT NULL,
    call_price numeric,
    call_billsec integer,
    rate_id integer NOT NULL,
    billing_account_id integer NOT NULL,
    rating_mode_id integer DEFAULT 0 NOT NULL,
    call_id integer NOT NULL,
    time_condition_id integer DEFAULT 0,
    pcard_id integer DEFAULT 0,
    call_ts timestamp without time zone,
    last_update timestamp without time zone,
    free_billsec_id integer DEFAULT 0
);


CREATE INDEX rating_call_id_ind ON rating USING btree (call_id);
CREATE INDEX rating_call_ts_ind ON rating USING btree (call_ts);
CREATE INDEX rating_fb_ind      ON rating USING btree (free_billsec_id);
CREATE INDEX rating_ind         ON rating USING btree (billing_account_id);

--
-- 'rating_mode' table
--
CREATE TABLE rating_mode (
    id serial PRIMARY KEY NOT NULL,
    name character varying(64) UNIQUE NOT NULL
);

COPY rating_mode (id, name) FROM stdin;
1	calling_number
2	account_code
3	src_context
4	dst_context
5	src_tgroup
6	dst_tgroup
7	calling_number_sms
\.


-- 
-- 'round_mode' table
--
CREATE TABLE round_mode (
    id serial PRIMARY KEY NOT NULL,
    name character varying(32) UNIQUE NOT NULL,
    comm text
);

COPY round_mode (id, name, comm) FROM stdin;
0	no rounding	no rounding
1	ceil	Round fractions up(0,[1-9] = 1)
2	floor	Round fractions down(0,[1-9] = 0)
3	round	Round (a >= 0.5,a = 1;a < 0.5,a = 0)
\.

CREATE TABLE src_context (
    id serial PRIMARY KEY NOT NULL,
    src_context character varying(64) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);


CREATE TABLE src_context_deff (
    id serial PRIMARY KEY NOT NULL,
    src_context_id integer NOT NULL,
    bill_plan_id integer NOT NULL
);


CREATE TABLE src_tgroup (
    id serial PRIMARY KEY NOT NULL,
    src_tgroup character varying(80) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);


CREATE TABLE src_tgroup_deff (
    id serial PRIMARY KEY NOT NULL,
    src_tgroup_id integer NOT NULL,
    bill_plan_id integer NOT NULL,
    clg_nadi integer DEFAULT 0 NOT NULL,
    cld_nadi integer DEFAULT 0 NOT NULL
);


CREATE TABLE tariff (
    id serial PRIMARY KEY NOT NULL,
    name character varying(64) UNIQUE NOT NULL,
    temp_id integer DEFAULT 0 NOT NULL,
    start_period integer DEFAULT 0 NOT NULL,
    end_period integer DEFAULT 0 NOT NULL,
    free_billsec_id integer DEFAULT 0
);


CREATE TABLE time_condition (
    id serial PRIMARY KEY NOT NULL,
    tariff_id integer NOT NULL,
    time_condition_id integer NOT NULL,
    prior integer DEFAULT 40
);


CREATE TABLE time_condition_deff (
    id serial PRIMARY KEY NOT NULL,
    hours character varying(11),
    days_week character varying(7),
    tc_name character varying(64) UNIQUE NOT NULL,
    tc_date character varying(11),
    year character varying(4),
    month character varying(2),
    day_month character varying(2)
);


--
-- 'version' table
--
CREATE TABLE version (
    release character varying NOT NULL,
    date timestamp without time zone NOT NULL
);

COPY version (release, date) FROM stdin;
0.6.16(devel)	2020-02-24 16:15:00
\.


COPY public.dbstorage_type (id, name) FROM stdin;
1	pgsql
2	mysql
3	sqlite3
\.



--
-- Name: acc_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.acc_id_seq', 1, true);


--
-- Name: bal_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.bal_id_seq', 1, true);


--
-- Name: bill_plan_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.bill_plan_id_seq', 1, true);


--
-- Name: bill_plan_tree_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.bill_plan_tree_id_seq', 1, true);


--
-- Name: billing_account_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.billing_account_id_seq',1, true);


--
-- Name: binding_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.binding_id_seq', 2, true);


--
-- Name: cdr_dbstorage_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.cdr_dbstorage_id_seq', 1, true);


--
-- Name: cdr_files_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.cdr_files_id_seq', 1, false);


--
-- Name: cdr_mode_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.cdr_mode_id_seq', 2, true);


--
-- Name: cdr_profiles_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.cdr_profiles_id_seq', 1, true);


--
-- Name: cdr_servers_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.cdr_servers_id_seq', 1, true);


--
-- Name: cdrs_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.cdrs_id_seq', 1, true);


--
-- Name: clg_deff__id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.clg_deff__id_seq', 1, false);


--
-- Name: clg_deff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.clg_deff_id_seq', 1, true);


--
-- Name: clg_history_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.clg_history_id_seq', 1, true);


--
-- Name: clg_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.clg_id_seq', 1, true);


--
-- Name: credit_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.credit_id_seq', 1, true);


--
-- Name: curr_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.curr_id_seq', 3, true);


--
-- Name: db_screenshot_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.db_screenshot_id_seq', 1, false);


--
-- Name: dbtype_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.dbtype_id_seq', 3, true);


--
-- Name: debit_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.debit_id_seq', 1, true);


--
-- Name: dst__deff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.dst__deff_id_seq', 1, true);


--
-- Name: dst_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.dst_id_seq', 1, true);


--
-- Name: dst_tgroup_deff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.dst_tgroup_deff_id_seq', 1, true);


--
-- Name: dst_tgroup_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.dst_tgroup_id_seq', 1, true);


--
-- Name: free_billsec_balance_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.free_billsec_balance_id_seq', 1, true);


--
-- Name: free_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.free_id_seq', 1, true);


--
-- Name: pcard_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.pcard_id_seq', 1, true);


--
-- Name: pcard_status_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.pcard_status_id_seq', 1, false);


--
-- Name: pcard_type_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.pcard_type_id_seq', 1, false);


--
-- Name: prefix_filter_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.prefix_filter_id_seq', 1, true);


--
-- Name: prefix_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.prefix_id_seq', 1, true);


--
-- Name: rate_function_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.rate_function_id_seq',1, true);


--
-- Name: rate_id_seq1; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.rate_id_seq1', 1, true);


--
-- Name: rating_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.rating_id_seq', 1, true);


--
-- Name: src__deff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.src__deff_id_seq', 1, true);


--
-- Name: src_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.src_id_seq', 1, true);


--
-- Name: src_tgroup_deff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.src_tgroup_deff_id_seq', 1, true);


--
-- Name: src_tgroup_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.src_tgroup_id_seq', 1, true);


--
-- Name: tariff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.tariff_id_seq', 1, true);


--
-- Name: tc_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

--SELECT pg_catalog.setval('public.tc_id_seq', 1, true);

