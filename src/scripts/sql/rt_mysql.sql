---
--- RateEngine(version 6) PostgreSQL database / 2018-04-13 /
---

---
--- 'account_code' table 
---
CREATE TABLE account_code (
    id serial PRIMARY KEY NOT NULL,
    account_code character varying(64) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);

---
--- 'account_code_deff' table
---
CREATE TABLE account_code_deff (
    id serial PRIMARY KEY NOT NULL,
    account_code_id integer NOT NULL,
    bill_plan_id integer NOT NULL
);

---
--- 'balance' table
---
CREATE TABLE balance (
    id serial PRIMARY KEY NOT NULL,
    billing_account_id integer NOT NULL,
    amount numeric DEFAULT 0,
    last_update timestamp without time zone,
    start_date character varying(32),
    end_date character varying(32),
    active boolean DEFAULT false,
    last_update_flag timestamp without time zone
);

CREATE INDEX bal_bacc_id_ind ON balance USING btree (billing_account_id);
CREATE INDEX bal_startd_ind ON balance USING btree (start_date);

---
--- 'bill_plan' table  
---
CREATE TABLE bill_plan (
    id serial PRIMARY KEY NOT NULL,
    name character varying(128) UNIQUE NOT NULL,
    bill_plan_type_id integer DEFAULT 2,
    start_period integer DEFAULT 0,
    end_period integer DEFAULT 0
);

---
--- 'bill_plan_tree' table
---
CREATE TABLE bill_plan_tree (
    id serial PRIMARY KEY NOT NULL,
    bill_plan_id integer NOT NULL,
    root_bplan_id integer NOT NULL
);

CREATE INDEX tree_root_id_ind ON bill_plan_tree USING btree (root_bplan_id);

---
--- 'bill_plan_type' table .... ne se izpolva !!!???
---
CREATE TABLE bill_plan_type (
    id serial PRIMARY KEY NOT NULL,
    name character varying(32) UNIQUE NOT NULL
);

INSERT INTO bill_plan_type (id, name) VALUES (1,'prepaid'),(2,'postpaid');

---
--- 'billing_account' table 
---
CREATE TABLE billing_account (
    id serial PRIMARY KEY NOT NULL,
    username character varying(64) UNIQUE NOT NULL,
    currency_id integer DEFAULT 1,
    leg character varying(3) DEFAULT 'a'::character varying,
    cdr_server_id integer DEFAULT 0,
    billing_day character varying(2) DEFAULT '01'::character varying,
    round_mode_id integer DEFAULT 0,
    day_of_payment integer DEFAULT 0
);

---
--- 'calc_function' table 
---
CREATE TABLE calc_function (
    id serial PRIMARY KEY NOT NULL,
    tariff_id integer NOT NULL,
    pos integer NOT NULL,
    delta_time integer NOT NULL,
    fee numeric NOT NULL,
    iterations integer
);

CREATE INDEX calc_tariff_id_inx ON calc_function USING btree (tariff_id);

---
--- 'calling_number' table 
---
CREATE TABLE calling_number (
    id serial PRIMARY KEY NOT NULL,
    calling_number character varying(32) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);

CREATE INDEX clg_bacc_id_inx ON calling_number USING btree (billing_account_id);

---
--- 'calling_number_deff' table --
---
CREATE TABLE calling_number_deff (
    id serial PRIMARY KEY NOT NULL,
    calling_number_id integer NOT NULL,
    bill_plan_id integer NOT NULL,
    sm_bill_plan_id integer DEFAULT 0
);

CREATE INDEX clg_deff_clg_id_inx ON calling_number_deff USING btree (calling_number_id);

---
--- 'cdr_dbstorage' table
---
CREATE TABLE cdr_dbstorage (
    id serial PRIMARY KEY NOT NULL,
    cdr_server_id integer NOT NULL,
    dbhost character varying(128),
    dbname character varying(128),
    dbuser character varying(128),
    dbpass character varying(128),
    cdr_table character varying(128) NOT NULL,
    dbstorage_type_id integer NOT NULL
);

---
--- 'cdr_files' table
---
CREATE TABLE cdr_files (
    id serial PRIMARY KEY NOT NULL,
    cdr_server_id integer NOT NULL,
    filename character varying(128) NOT NULL,
    last_update timestamp
);

---
--- 'cdr_get_mode' table
---
CREATE TABLE cdr_get_mode (
    id serial PRIMARY KEY,
    name character varying(4) UNIQUE
);

INSERT INTO cdr_get_mode (id, name) VALUES (1,'db'),(2,'file');

---
--- 'cdr_profiles' table
---
CREATE TABLE cdr_profiles (
    id serial PRIMARY KEY NOT NULL,
    profile_name character varying(32) UNIQUE NOT NULL,
    profile_version integer DEFAULT 0
);

---
--- 'cdr_profiles' table
---
CREATE TABLE cdr_servers (
    id serial PRIMARY KEY NOT NULL,
    cdr_profiles_id integer NOT NULL,
    get_mode_id integer NOT NULL,
    active CHAR
);

---
--- 'cdr_storage_sched' table
---
CREATE TABLE cdr_storage_sched (
    ts serial PRIMARY KEY NOT NULL,
    cdr_server_id integer NOT NULL,
    last_chk_ts integer DEFAULT 0,
    start_ts integer NOT NULL,
    replies integer DEFAULT 0
);

CREATE INDEX cdr_storage_sched_uniq ON cdr_storage_sched USING btree (cdr_server_id);

---
--- 'cdrs' table
---
CREATE TABLE cdrs (
    id serial PRIMARY KEY NOT NULL,
    cdr_server_id integer NOT NULL,
    cdr_rec_type_id integer DEFAULT 0,
    call_uid character varying(128) UNIQUE NOT NULL,
    leg_a integer DEFAULT 0,
    leg_b integer DEFAULT 0,
    start_ts timestamp,
--    answer_ts timestamp without time zone,
--    end_ts timestamp without time zone,
    answer_ts character varying(32),
    end_ts character varying(32),
    start_epoch integer DEFAULT 0,
    answer_epoch integer DEFAULT 0,
    end_epoch integer DEFAULT 0,
    src character varying(80),
    dst character varying(80),    
    calling_number character varying(32),
    called_number character varying(32) NOT NULL,
    prefix_filter_id integer DEFAULT 0,
    clg_nadi integer DEFAULT 0,
    cld_nadi integer DEFAULT 0,
    billsec integer NOT NULL,
    duration integer NOT NULL,
    uduration bigint DEFAULT 0,
    billusec bigint DEFAULT 0,   
    account_code character varying(64),
    src_context character varying(64),
    dst_context character varying(64),
    src_tgroup character varying(64),
    dst_tgroup character varying(64),
    rdnis character varying(32),
    rdnis_nadi integer DEFAULT 0,
    ocn character varying(32),
    ocn_nadi integer DEFAULT 0
);

CREATE INDEX leg_a_ind ON cdrs USING btree (leg_a);
CREATE INDEX leg_b_ind ON cdrs USING btree (leg_b);
CREATE INDEX cdrs_ts_ind ON cdrs USING btree (start_ts);

---
--- 'clg_history' table
---
CREATE TABLE clg_history (
    id serial PRIMARY KEY NOT NULL,
    calling_number character varying(32) NOT NULL,
    billing_account_id integer,
    bill_plan_id integer,
    last_update timestamp without time zone,
    operation character varying(64)
);

---
--- 'currency' table 
---
CREATE TABLE currency (
    id serial PRIMARY KEY NOT NULL,
    name character varying(12) UNIQUE NOT NULL,
    to_bg numeric DEFAULT 0
);

INSERT INTO currency (id, name, to_bg) VALUES (1,'лв.',0),(2,'евро',1.95583);

---
--- 'db_screenshot' table
---
CREATE TABLE db_screenshot (
    id serial PRIMARY KEY NOT NULL,
    tbl_name character varying(64) UNIQUE NOT NULL,
    last_tbl_operation character(1),
    last_update timestamp without time zone,
    last_update_ts integer NOT NULL
);

---
--- 'dbstorage_type' table ... ??? izpolzwali se ???
---
CREATE TABLE dbstorage_type (
    id serial PRIMARY KEY NOT NULL,
    name character varying(16) UNIQUE NOT NULL
);

INSERT INTO dbstorage_type (id, name) VALUES (1,'pgsql'),(2,'mysql'),(3,'sqlite3');

---
--- 'dst_context' table
---
CREATE TABLE dst_context (
    id serial PRIMARY KEY NOT NULL,
    dst_context character varying(64) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);

---
--- 'dst_context_deff' table
--- 
CREATE TABLE dst_context_deff (
    id serial PRIMARY KEY NOT NULL,
    dst_context_id integer NOT NULL,
    bill_plan_id integer NOT NULL
);

---
--- 'dst_tgroup' table
---
CREATE TABLE dst_tgroup (
    id serial PRIMARY KEY NOT NULL,
    dst_tgroup character varying(64) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);

---
--- 'dst_tgroup_deff' table
---
CREATE TABLE dst_tgroup_deff (
    id serial PRIMARY KEY NOT NULL,
    dst_tgroup_id integer NOT NULL,
    bill_plan_id integer NOT NULL,
    clg_nadi integer DEFAULT 0,
    cld_nadi integer DEFAULT 0
);

---
--- 'free_billsec' table
---
CREATE TABLE free_billsec (
    id serial PRIMARY KEY NOT NULL,
    free_billsec integer NOT NULL
);

-- 'free_billsec' start values
-- can be deleted these values !
INSERT INTO free_billsec (id,free_billsec) values (1,24000),(2,60000),(3,300000),(4,10);

---

--- 'free_billsec_balance' table
---
CREATE TABLE free_billsec_balance (
    id serial PRIMARY KEY NOT NULL,
    balance_id integer NOT NULL,
    tariff_id integer DEFAULT 0,
    free_billsec integer DEFAULT 0,
    last_update timestamp without time zone DEFAULT now(),
    free_billsec_id integer DEFAULT 0
);

CREATE INDEX free_billsec_bal_inx ON free_billsec_balance USING btree (free_billsec_id);
CREATE INDEX free_billsec_bal_inx_2 ON free_billsec_balance USING btree (balance_id);

-- izpolzvali se ????
--CREATE TABLE free_call_price (
--    id serial PRIMARY KEY,
--    billing_account_id integer,
--    free_call_price numeric
--);

---
--- 'pcard' table 
---
CREATE TABLE pcard (
    id serial PRIMARY KEY NOT NULL,
    amount numeric DEFAULT (0)::numeric NOT NULL,
    start_date character varying(10),
    end_date character varying(10),
    last_update timestamp without time zone NOT NULL,
    pcard_status_id integer DEFAULT 0 NOT NULL,
    billing_account_id integer DEFAULT 0 NOT NULL,
    pcard_type_id integer DEFAULT 2 NOT NULL,
    call_number integer DEFAULT 1 NOT NULL,
    saved_amount numeric DEFAULT (0)::numeric NOT NULL,
    sim integer DEFAULT 0 NOT NULL
);

CREATE INDEX pcard_bacc_id_inx ON pcard USING btree (billing_account_id);

---
--- 'pcard_status' table 
---
CREATE TABLE pcard_status (
    id serial PRIMARY KEY NOT NULL,
    status character varying(12) UNIQUE NOT NULL
);

INSERT INTO pcard_status (id, status) VALUES (0,'deactive'),(1,'active'),(2,'block');

---
--- 'pcard_type' table 
---
CREATE TABLE pcard_type (
    id serial PRIMARY KEY NOT NULL,
    name character varying(32) UNIQUE NOT NULL,
    "desc" text
);

INSERT INTO pcard_type (id, name) VALUES (1,'debit card'),(2,'credit card');

---
--- 'prefix' table 
---
CREATE TABLE prefix (
    id serial PRIMARY KEY NOT NULL,
    prefix character varying(32) UNIQUE NOT NULL,
    comm text
);

---
--- 'prefix_filter' table 
---
CREATE TABLE prefix_filter (
    id serial PRIMARY KEY NOT NULL,
    filtering_prefix character varying(32) NOT NULL,
    filtering_number integer,
    replace_str character varying(32),
    cdr_server_id integer DEFAULT 0,
    len integer DEFAULT 0,
    cdr_profiles_id integer
);

---
--- 'rate' table 
---
CREATE TABLE rate (
    id serial PRIMARY KEY NOT NULL,
    bill_plan_id integer NOT NULL,
    tariff_id integer NOT NULL,
    prefix_id integer NOT NULL
);

CREATE INDEX rate_bplan_id_ind ON rate USING btree (bill_plan_id);

CREATE RULE db_screenshot_d AS
    ON DELETE TO public.rate DO ( DELETE FROM public.db_screenshot
  WHERE ((db_screenshot.tbl_name)::text = 'rate'::text);
 INSERT INTO public.db_screenshot (tbl_name, last_tbl_operation, last_update, last_update_ts)  SELECT 'rate',
            'D',
            now() AS now,
            date_part('epoch'::text, now()) AS date_part;
);

CREATE RULE db_screenshot_i AS
    ON INSERT TO public.rate DO ( DELETE FROM public.db_screenshot
  WHERE ((db_screenshot.tbl_name)::text = 'rate'::text);
 INSERT INTO public.db_screenshot (tbl_name, last_tbl_operation, last_update, last_update_ts)  SELECT 'rate',
            'I',
            now() AS now,
            date_part('epoch'::text, now()) AS date_part;
);

CREATE RULE db_screenshot_u AS
    ON UPDATE TO public.rate DO ( DELETE FROM public.db_screenshot
  WHERE ((db_screenshot.tbl_name)::text = 'rate'::text);
 INSERT INTO public.db_screenshot (tbl_name, last_tbl_operation, last_update, last_update_ts)  SELECT 'rate',
            'U',
            now() AS now,
            date_part('epoch'::text, now()) AS date_part;
);

---
--- 'rating' table 
---
CREATE TABLE rating (
    id serial PRIMARY KEY NOT NULL,
    call_price numeric,
    call_billsec integer,
    rate_id integer NOT NULL,
    billing_account_id integer NOT NULL,
    rating_mode_id integer NOT NULL,
    call_id integer NOT NULL,
    time_condition_id integer DEFAULT 0,
    pcard_id integer DEFAULT 0,
    call_ts timestamp without time zone NOT NULL,
    last_update timestamp without time zone NOT NULL,
    free_billsec_id integer DEFAULT 0
);

CREATE INDEX rating_call_id_ind ON rating USING btree (call_id);
CREATE INDEX rating_call_ts_ind ON rating USING btree (call_ts);
CREATE INDEX rating_fb_ind ON rating USING btree (free_billsec_id);
CREATE INDEX rating_ind ON rating USING btree (billing_account_id);

---
--- 'rating_mode' table 
---
CREATE TABLE rating_mode (
    id serial PRIMARY KEY NOT NULL,
    name character varying(64) UNIQUE NOT NULL
);

INSERT INTO rating_mode (id, name) VALUES 
(1,'calling_number'),(2,'account_code'),(3,'src_context'),(4,'dst_context'),(5,'src_tgroup'),(6,'dst_tgroup'),(7,'calling_number_sms');


---
--- 'round_mode' table 
---
CREATE TABLE round_mode (
    id serial PRIMARY KEY NOT NULL,
    name character varying(32) UNIQUE NOT NULL,
    comm text
);

INSERT INTO round_mode (id, name, comm) VALUES 
(0,'no rounding','no rounding'),
(1,'ceil','Round fractions up(0,[1-9] = 1)'),
(2,'floor','Round fractions down(0,[1-9] = 0)'),
(3,'round','Round (a >= 0.5,a = 1;a < 0.5,a = 0)');

---
--- 'src_context' table 
---
CREATE TABLE src_context (
    id serial PRIMARY KEY NOT NULL,
    src_context character varying(64) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);

---
--- 'src_context_deff' table 
---
CREATE TABLE src_context_deff (
    id serial PRIMARY KEY NOT NULL,
    src_context_id integer NOT NULL,
    bill_plan_id integer NOT NULL
);

---
--- 'src_tgroup' table
---
CREATE TABLE src_tgroup (
    id serial PRIMARY KEY NOT NULL,
    src_tgroup character varying(64) UNIQUE NOT NULL,
    billing_account_id integer NOT NULL
);

---
--- 'src_tgroup_deff' table
---
CREATE TABLE src_tgroup_deff (
    id serial PRIMARY KEY NOT NULL,
    src_tgroup_id integer NOT NULL,
    bill_plan_id integer NOT NULL,
    clg_nadi integer DEFAULT 0,
    cld_nadi integer DEFAULT 0
);

---
--- 'tariff' table 
--- 
CREATE TABLE tariff (
    id serial PRIMARY KEY NOT NULL,
    name character varying(64) UNIQUE NOT NULL,
    temp_id integer DEFAULT 0,
    start_period integer DEFAULT 0,
    end_period integer DEFAULT 0,
    free_billsec_id integer DEFAULT 0
);

CREATE INDEX tariff_id_ind ON tariff USING btree (id);

---
--- 'time_condition' table 
---
CREATE TABLE time_condition (
    id serial PRIMARY KEY NOT NULL,
    tariff_id integer NOT NULL,
    time_condition_id integer,
    prior integer DEFAULT 40
);

---
--- 'time_condition_deff' table
---
CREATE TABLE time_condition_deff (
    id serial PRIMARY KEY NOT NULL,
    hours character varying(11),
    days_week character varying(7),
    tc_name character varying(64),
    tc_date character varying(11),
    year character varying(4),
    month character varying(2),
    day_month character varying(2)
);

---
--- 'version' table 
---
CREATE TABLE version (
    release character varying NOT NULL,
    date timestamp without time zone NOT NULL
);

INSERT INTO version (release, date) VALUES ('0.6.13(beta)',now());

--- 
--- END
---
