--
-- PostgreSQL database dump
--

-- Dumped from database version 9.5.7
-- Dumped by pg_dump version 9.5.7

SET statement_timeout = 0;
SET lock_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


SET search_path = public, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: account_code; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE account_code (
    id bigint NOT NULL,
    account_code character varying(64) NOT NULL,
    billing_account_id integer NOT NULL
);


ALTER TABLE account_code OWNER TO global;

--
-- Name: acc_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE acc_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE acc_id_seq OWNER TO global;

--
-- Name: acc_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE acc_id_seq OWNED BY account_code.id;


--
-- Name: account_code_deff; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE account_code_deff (
    id bigint NOT NULL,
    account_code_id integer NOT NULL,
    bill_plan_id integer NOT NULL
);


ALTER TABLE account_code_deff OWNER TO global;

--
-- Name: bal_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE bal_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE bal_id_seq OWNER TO global;

--
-- Name: balance; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE balance (
    id bigint DEFAULT nextval('bal_id_seq'::regclass) NOT NULL,
    billing_account_id integer NOT NULL,
    amount numeric DEFAULT 0,
    last_update timestamp without time zone,
    start_date character varying(32),
    end_date character varying(32),
    active boolean DEFAULT false,
    last_update_flag timestamp without time zone
);


ALTER TABLE balance OWNER TO global;

--
-- Name: bill_plan; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE bill_plan (
    id bigint NOT NULL,
    name character varying(128) NOT NULL,
    bill_plan_type_id integer DEFAULT 2 NOT NULL,
    start_period integer DEFAULT 0,
    end_period integer DEFAULT 0
);


ALTER TABLE bill_plan OWNER TO global;

--
-- Name: bill_plan_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE bill_plan_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE bill_plan_id_seq OWNER TO global;

--
-- Name: bill_plan_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE bill_plan_id_seq OWNED BY bill_plan.id;


--
-- Name: bill_plan_tree; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE bill_plan_tree (
    id integer NOT NULL,
    bill_plan_id integer NOT NULL,
    root_bplan_id integer NOT NULL
);


ALTER TABLE bill_plan_tree OWNER TO global;

--
-- Name: bill_plan_tree_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE bill_plan_tree_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE bill_plan_tree_id_seq OWNER TO global;

--
-- Name: bill_plan_tree_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE bill_plan_tree_id_seq OWNED BY bill_plan_tree.id;


--
-- Name: bill_plan_type; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE bill_plan_type (
    id integer,
    name character varying(32)
);


ALTER TABLE bill_plan_type OWNER TO global;

--
-- Name: billing_account; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE billing_account (
    id bigint NOT NULL,
    username character varying(64) NOT NULL,
    currency_id integer DEFAULT 1 NOT NULL,
    leg character varying(3) DEFAULT 'a'::character varying NOT NULL,
    cdr_server_id integer DEFAULT 0 NOT NULL,
    billing_day character varying(2) DEFAULT '01'::character varying,
    round_mode_id integer DEFAULT 0,
    day_of_payment integer DEFAULT 0
);


ALTER TABLE billing_account OWNER TO global;

--
-- Name: billing_account_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE billing_account_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE billing_account_id_seq OWNER TO global;

--
-- Name: billing_account_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE billing_account_id_seq OWNED BY billing_account.id;


--
-- Name: binding_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE binding_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE binding_id_seq OWNER TO global;

--
-- Name: binding_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE binding_id_seq OWNED BY account_code_deff.id;


--
-- Name: calc_function; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE calc_function (
    id bigint NOT NULL,
    tariff_id integer NOT NULL,
    pos integer NOT NULL,
    delta_time integer NOT NULL,
    fee numeric NOT NULL,
    iterations integer
);


ALTER TABLE calc_function OWNER TO global;

--
-- Name: clg_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE clg_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE clg_id_seq OWNER TO global;

--
-- Name: calling_number; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE calling_number (
    id bigint DEFAULT nextval('clg_id_seq'::regclass) NOT NULL,
    calling_number character varying(80) NOT NULL,
    billing_account_id integer NOT NULL
);


ALTER TABLE calling_number OWNER TO global;

--
-- Name: clg_deff_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE clg_deff_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE clg_deff_id_seq OWNER TO global;

--
-- Name: calling_number_deff; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE calling_number_deff (
    id bigint DEFAULT nextval('clg_deff_id_seq'::regclass) NOT NULL,
    calling_number_id integer NOT NULL,
    bill_plan_id integer NOT NULL,
    sm_bill_plan_id integer
);


ALTER TABLE calling_number_deff OWNER TO global;

--
-- Name: cdr_dbstorage; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE cdr_dbstorage (
    id bigint NOT NULL,
    cdr_server_id integer NOT NULL,
    dbhost character varying(128),
    dbname character varying(128),
    dbuser character varying(128),
    dbpass character varying(128),
    cdr_table character varying(128) NOT NULL,
    dbstorage_type_id integer NOT NULL
);


ALTER TABLE cdr_dbstorage OWNER TO global;

--
-- Name: cdr_dbstorage_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE cdr_dbstorage_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE cdr_dbstorage_id_seq OWNER TO global;

--
-- Name: cdr_dbstorage_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE cdr_dbstorage_id_seq OWNED BY cdr_dbstorage.id;


--
-- Name: cdr_files; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE cdr_files (
    id bigint NOT NULL,
    cdr_server_id integer NOT NULL,
    filename character varying(128) NOT NULL,
    last_update timestamp without time zone
);


ALTER TABLE cdr_files OWNER TO global;

--
-- Name: cdr_files_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE cdr_files_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE cdr_files_id_seq OWNER TO global;

--
-- Name: cdr_files_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE cdr_files_id_seq OWNED BY cdr_files.id;


--
-- Name: cdr_get_mode; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE cdr_get_mode (
    id bigint NOT NULL,
    name character varying(4) NOT NULL
);


ALTER TABLE cdr_get_mode OWNER TO global;

--
-- Name: cdr_mode_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE cdr_mode_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE cdr_mode_id_seq OWNER TO global;

--
-- Name: cdr_mode_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE cdr_mode_id_seq OWNED BY cdr_get_mode.id;


--
-- Name: cdr_profiles; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE cdr_profiles (
    id integer NOT NULL,
    profile_name character varying(32) NOT NULL,
    profile_version integer DEFAULT 0
);


ALTER TABLE cdr_profiles OWNER TO global;

--
-- Name: cdr_profiles_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE cdr_profiles_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE cdr_profiles_id_seq OWNER TO global;

--
-- Name: cdr_profiles_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE cdr_profiles_id_seq OWNED BY cdr_profiles.id;


--
-- Name: cdr_servers; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE cdr_servers (
    id bigint NOT NULL,
    cdr_profiles_id integer NOT NULL,
    get_mode_id integer NOT NULL,
    active boolean
);


ALTER TABLE cdr_servers OWNER TO global;

--
-- Name: cdr_servers_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE cdr_servers_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE cdr_servers_id_seq OWNER TO global;

--
-- Name: cdr_servers_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE cdr_servers_id_seq OWNED BY cdr_servers.id;


--
-- Name: cdr_storage_sched; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE cdr_storage_sched (
    ts bigint DEFAULT (0)::bigint NOT NULL,
    cdr_server_id integer NOT NULL,
    last_chk_ts bigint DEFAULT (0)::bigint NOT NULL,
    start_ts bigint NOT NULL,
    replies bigint DEFAULT (0)::bigint NOT NULL
);


ALTER TABLE cdr_storage_sched OWNER TO global;

--
-- Name: cdrs_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

--CREATE SEQUENCE cdrs_id_seq
--    START WITH 1
--    INCREMENT BY 1
--    NO MINVALUE
--    NO MAXVALUE
--    CACHE 1;


--ALTER TABLE cdrs_id_seq OWNER TO global;

--
-- Name: cdrs; Type: TABLE; Schema: public; Owner: global
--

--CREATE TABLE cdrs (
--    id integer DEFAULT nextval('cdrs_id_seq'::regclass) NOT NULL,
--    cdr_server_id integer NOT NULL,
--    call_uid character varying(128) NOT NULL,
--    leg_a integer DEFAULT 0 NOT NULL,
--    leg_b integer DEFAULT 0 NOT NULL,
--    ts timestamp with time zone NOT NULL,
--   src character varying(80),
--    dst character varying(80) NOT NULL,
--    prefix_filter_id integer,
--    calling_number character varying(80),
--    called_number character varying(80) NOT NULL,
--    clg_nadi integer DEFAULT 0 NOT NULL,
--    cld_nadi integer DEFAULT 0 NOT NULL,
--    presentation integer DEFAULT 0 NOT NULL,
--    screen integer DEFAULT 0 NOT NULL,
--   "clg-type" character varying(32),
--    "cld-type" character varying(32),
--    src_context character varying(80),
--    dst_context character varying(80),
--    billsec integer NOT NULL,
--    duration integer NOT NULL,
--    incoming_channel character varying(128),
--    outgoing_channel character varying(128),
--    incoming_codec character varying(16),
--    outgoing_codec character varying(16),
--    account_code character varying(80),
--    src_tgroup character varying(80),
--    dst_tgroup character varying(80),
--    uduration bigint DEFAULT (0)::bigint NOT NULL,
--    billusec bigint DEFAULT (0)::bigint NOT NULL,
--    epoch integer
--);


--ALTER TABLE cdrs OWNER TO global;

--
-- Name: cdrs_v2; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE cdrs (
    id integer NOT NULL,
    cdr_server_id integer NOT NULL,
    cdr_rec_type_id integer DEFAULT 0,
    leg_a integer DEFAULT 0,
    leg_b integer DEFAULT 0,
    call_uid character varying(128) NOT NULL,
    start_ts character varying(32),
    answer_ts character varying(32),
    end_ts character varying(32),
    start_epoch integer DEFAULT 0,
    answer_epoch integer DEFAULT 0,
    end_epoch integer DEFAULT 0,
    src character varying(80),
    dst character varying(80),
    calling_number character varying(32),
    clg_nadi integer DEFAULT 0,
    called_number character varying(32),
    cld_nadi integer DEFAULT 0,
    rdnis character varying(32),
    rdnis_nadi integer DEFAULT 0,
    ocn character varying(32),
    ocn_nadi integer DEFAULT 0,
    prefix_filter_id integer DEFAULT 0,
    account_code character varying(80),
    src_context character varying(80),
    src_tgroup character varying(80),
    dst_context character varying(80),
    dst_tgroup character varying(80),
    billsec integer DEFAULT 0,
    duration integer DEFAULT 0,
    uduration integer DEFAULT 0,
    billusec integer DEFAULT 0
);


ALTER TABLE cdrs OWNER TO global;

--
-- Name: cdrs_v2_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE cdrs_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE cdrs_id_seq OWNER TO global;

--
-- Name: cdrs_v2_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE cdrs_id_seq OWNED BY cdrs.id;


--
-- Name: clg_deff__id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE clg_deff__id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE clg_deff__id_seq OWNER TO global;

--
-- Name: credit_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE credit_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE credit_id_seq OWNER TO global;

--
-- Name: curr_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE curr_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE curr_id_seq OWNER TO global;

--
-- Name: currency; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE currency (
    id bigint DEFAULT nextval('curr_id_seq'::regclass) NOT NULL,
    name character varying(12),
    to_bg numeric DEFAULT 0
);


ALTER TABLE currency OWNER TO global;

--
-- Name: dbstorage_type; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE dbstorage_type (
    id bigint NOT NULL,
    name character varying(16)
);


ALTER TABLE dbstorage_type OWNER TO global;

--
-- Name: dbtype_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE dbtype_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE dbtype_id_seq OWNER TO global;

--
-- Name: dbtype_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE dbtype_id_seq OWNED BY dbstorage_type.id;


--
-- Name: debit_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE debit_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE debit_id_seq OWNER TO global;

--
-- Name: dst__deff_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE dst__deff_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE dst__deff_id_seq OWNER TO global;

--
-- Name: dst_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE dst_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE dst_id_seq OWNER TO global;

--
-- Name: dst_context; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE dst_context (
    id bigint DEFAULT nextval('dst_id_seq'::regclass) NOT NULL,
    dst_context character varying(64) NOT NULL,
    billing_account_id integer NOT NULL
);


ALTER TABLE dst_context OWNER TO global;

--
-- Name: dst_context_deff; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE dst_context_deff (
    id bigint DEFAULT nextval('dst__deff_id_seq'::regclass) NOT NULL,
    dst_context_id integer NOT NULL,
    bill_plan_id integer NOT NULL
);


ALTER TABLE dst_context_deff OWNER TO global;

--
-- Name: dst_tgroup; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE dst_tgroup (
    id integer NOT NULL,
    dst_tgroup character varying(80) NOT NULL,
    billing_account_id integer NOT NULL
);


ALTER TABLE dst_tgroup OWNER TO global;

--
-- Name: dst_tgroup_deff; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE dst_tgroup_deff (
    id integer NOT NULL,
    dst_tgroup_id integer NOT NULL,
    bill_plan_id integer NOT NULL,
    clg_nadi integer DEFAULT 0 NOT NULL,
    cld_nadi integer DEFAULT 0 NOT NULL
);


ALTER TABLE dst_tgroup_deff OWNER TO global;

--
-- Name: dst_tgroup_deff_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE dst_tgroup_deff_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE dst_tgroup_deff_id_seq OWNER TO global;

--
-- Name: dst_tgroup_deff_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE dst_tgroup_deff_id_seq OWNED BY dst_tgroup_deff.id;


--
-- Name: dst_tgroup_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE dst_tgroup_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE dst_tgroup_id_seq OWNER TO global;

--
-- Name: dst_tgroup_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE dst_tgroup_id_seq OWNED BY dst_tgroup.id;


--
-- Name: free_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE free_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE free_id_seq OWNER TO global;

--
-- Name: free_billsec; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE free_billsec (
    id bigint DEFAULT nextval('free_id_seq'::regclass) NOT NULL,
    free_billsec integer NOT NULL,
    tariff_id integer DEFAULT 0
);


ALTER TABLE free_billsec OWNER TO global;

--
-- Name: free_billsec_balance; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE free_billsec_balance (
    id bigint NOT NULL,
    balance_id integer NOT NULL,
    tariff_id integer NOT NULL,
    free_billsec integer DEFAULT 0 NOT NULL,
    last_update timestamp without time zone DEFAULT now()
);


ALTER TABLE free_billsec_balance OWNER TO global;

--
-- Name: free_billsec_balance_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE free_billsec_balance_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE free_billsec_balance_id_seq OWNER TO global;

--
-- Name: free_billsec_balance_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE free_billsec_balance_id_seq OWNED BY free_billsec_balance.id;


--
-- Name: free_call_price; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE free_call_price (
    id bigint,
    billing_account_id integer,
    free_call_price numeric
);


ALTER TABLE free_call_price OWNER TO global;

--
-- Name: pcard; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE pcard (
    id bigint NOT NULL,
    amount numeric DEFAULT (0)::numeric NOT NULL,
    start_date character varying(10),
    end_date character varying(10),
    last_update timestamp without time zone DEFAULT '2011-10-31 19:04:40.438417'::timestamp without time zone NOT NULL,
    pcard_status_id integer DEFAULT 0 NOT NULL,
    billing_account_id integer DEFAULT 0 NOT NULL,
    pcard_type_id integer DEFAULT 2 NOT NULL,
    call_number integer DEFAULT 1 NOT NULL,
    saved_amount numeric DEFAULT (0)::numeric NOT NULL,
    sim integer DEFAULT 0 NOT NULL
);


ALTER TABLE pcard OWNER TO global;

--
-- Name: pcard_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE pcard_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE pcard_id_seq OWNER TO global;

--
-- Name: pcard_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE pcard_id_seq OWNED BY pcard.id;


--
-- Name: pcard_status; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE pcard_status (
    id bigint NOT NULL,
    status character varying(64) NOT NULL
);


ALTER TABLE pcard_status OWNER TO global;

--
-- Name: pcard_status_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE pcard_status_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE pcard_status_id_seq OWNER TO global;

--
-- Name: pcard_status_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE pcard_status_id_seq OWNED BY pcard_status.id;


--
-- Name: pcard_type; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE pcard_type (
    id bigint NOT NULL,
    name character varying(32) NOT NULL,
    "desc" text
);


ALTER TABLE pcard_type OWNER TO global;

--
-- Name: pcard_type_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE pcard_type_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE pcard_type_id_seq OWNER TO global;

--
-- Name: pcard_type_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE pcard_type_id_seq OWNED BY pcard_type.id;


--
-- Name: prefix; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE prefix (
    id integer NOT NULL,
    prefix character varying(32) NOT NULL,
    comm text
);


ALTER TABLE prefix OWNER TO global;

--
-- Name: prefix_filter; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE prefix_filter (
    id bigint NOT NULL,
    filtering_prefix character varying(32),
    filtering_number integer,
    replace_str character varying(32),
    cdr_server_id integer DEFAULT 0,
    len integer DEFAULT 0,
    cdr_profiles_id integer
);


ALTER TABLE prefix_filter OWNER TO global;

--
-- Name: prefix_filter_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE prefix_filter_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE prefix_filter_id_seq OWNER TO global;

--
-- Name: prefix_filter_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE prefix_filter_id_seq OWNED BY prefix_filter.id;


--
-- Name: prefix_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE prefix_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE prefix_id_seq OWNER TO global;

--
-- Name: prefix_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE prefix_id_seq OWNED BY prefix.id;


--
-- Name: rate; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE rate (
    id bigint NOT NULL,
    bill_plan_id integer NOT NULL,
    tariff_id integer NOT NULL,
    prefix_id integer
);


ALTER TABLE rate OWNER TO global;

--
-- Name: rate_function_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE rate_function_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE rate_function_id_seq OWNER TO global;

--
-- Name: rate_function_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE rate_function_id_seq OWNED BY calc_function.id;


--
-- Name: rate_id_seq1; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE rate_id_seq1
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE rate_id_seq1 OWNER TO global;

--
-- Name: rate_id_seq1; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE rate_id_seq1 OWNED BY rate.id;


--
-- Name: rating_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE rating_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE rating_id_seq OWNER TO global;

--
-- Name: rating; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE rating (
    id bigint DEFAULT nextval('rating_id_seq'::regclass) NOT NULL,
    call_price numeric,
    call_billsec integer,
    rate_id integer NOT NULL,
    billing_account_id integer NOT NULL,
    rating_mode_id integer DEFAULT 0 NOT NULL,
    call_id integer NOT NULL,
    time_condition_id integer DEFAULT 0,
    pcard_id integer DEFAULT 0,
    call_ts timestamp without time zone,
    last_update timestamp without time zone
);


ALTER TABLE rating OWNER TO global;

--
-- Name: rating_mode; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE rating_mode (
    id integer NOT NULL,
    name character varying(64) NOT NULL
);


ALTER TABLE rating_mode OWNER TO global;

--
-- Name: round_mode; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE round_mode (
    id integer,
    name character varying(32),
    comm text
);


ALTER TABLE round_mode OWNER TO global;

--
-- Name: src__deff_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE src__deff_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE src__deff_id_seq OWNER TO global;

--
-- Name: src_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE src_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE src_id_seq OWNER TO global;

--
-- Name: src_context; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE src_context (
    id bigint DEFAULT nextval('src_id_seq'::regclass) NOT NULL,
    src_context character varying(64) NOT NULL,
    billing_account_id integer NOT NULL
);


ALTER TABLE src_context OWNER TO global;

--
-- Name: src_context_deff; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE src_context_deff (
    id bigint DEFAULT nextval('src__deff_id_seq'::regclass) NOT NULL,
    src_context_id integer NOT NULL,
    bill_plan_id integer NOT NULL
);


ALTER TABLE src_context_deff OWNER TO global;

--
-- Name: src_tgroup; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE src_tgroup (
    id integer NOT NULL,
    src_tgroup character varying(80) NOT NULL,
    billing_account_id integer NOT NULL
);


ALTER TABLE src_tgroup OWNER TO global;

--
-- Name: src_tgroup_deff; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE src_tgroup_deff (
    id integer NOT NULL,
    src_tgroup_id integer NOT NULL,
    bill_plan_id integer NOT NULL,
    clg_nadi integer DEFAULT 0 NOT NULL,
    cld_nadi integer DEFAULT 0 NOT NULL
);


ALTER TABLE src_tgroup_deff OWNER TO global;

--
-- Name: src_tgroup_deff_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE src_tgroup_deff_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE src_tgroup_deff_id_seq OWNER TO global;

--
-- Name: src_tgroup_deff_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE src_tgroup_deff_id_seq OWNED BY src_tgroup_deff.id;


--
-- Name: src_tgroup_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE src_tgroup_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE src_tgroup_id_seq OWNER TO global;

--
-- Name: src_tgroup_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE src_tgroup_id_seq OWNED BY src_tgroup.id;


--
-- Name: tariff; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE tariff (
    id integer NOT NULL,
    name character varying(64) NOT NULL,
    temp_id integer DEFAULT 0 NOT NULL,
    start_period integer DEFAULT 0 NOT NULL,
    end_period integer DEFAULT 0 NOT NULL,
    free_billsec_id integer DEFAULT 0
);


ALTER TABLE tariff OWNER TO global;

--
-- Name: tariff_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE tariff_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE tariff_id_seq OWNER TO global;

--
-- Name: tariff_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE tariff_id_seq OWNED BY tariff.id;


--
-- Name: tc_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE tc_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE tc_id_seq OWNER TO global;

--
-- Name: time_condition; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE time_condition (
    id bigint DEFAULT nextval('tc_id_seq'::regclass) NOT NULL,
    tariff_id integer NOT NULL,
    time_condition_id integer,
    prior integer DEFAULT 40
);


ALTER TABLE time_condition OWNER TO global;

--
-- Name: time_condition_deff; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE time_condition_deff (
    id bigint NOT NULL,
    hours character varying(11),
    days_week character varying(7),
    tc_name character varying(64),
    tc_date character varying(11)
);


ALTER TABLE time_condition_deff OWNER TO global;

--
-- Name: time_conditions_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE time_conditions_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE time_conditions_id_seq OWNER TO global;

--
-- Name: time_conditions_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE time_conditions_id_seq OWNED BY time_condition_deff.id;


--
-- Name: users; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE users (
    id bigint NOT NULL,
    username character varying(64),
    password character varying(64)
);


ALTER TABLE users OWNER TO global;

--
-- Name: users_id_seq; Type: SEQUENCE; Schema: public; Owner: global
--

CREATE SEQUENCE users_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE users_id_seq OWNER TO global;

--
-- Name: users_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: global
--

ALTER SEQUENCE users_id_seq OWNED BY users.id;


--
-- Name: version; Type: TABLE; Schema: public; Owner: global
--

CREATE TABLE version (
    release character varying NOT NULL,
    date timestamp without time zone NOT NULL
);


ALTER TABLE version OWNER TO global;

--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY account_code ALTER COLUMN id SET DEFAULT nextval('acc_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY account_code_deff ALTER COLUMN id SET DEFAULT nextval('binding_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY bill_plan ALTER COLUMN id SET DEFAULT nextval('bill_plan_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY bill_plan_tree ALTER COLUMN id SET DEFAULT nextval('bill_plan_tree_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY billing_account ALTER COLUMN id SET DEFAULT nextval('billing_account_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY calc_function ALTER COLUMN id SET DEFAULT nextval('rate_function_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_dbstorage ALTER COLUMN id SET DEFAULT nextval('cdr_dbstorage_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_files ALTER COLUMN id SET DEFAULT nextval('cdr_files_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_get_mode ALTER COLUMN id SET DEFAULT nextval('cdr_mode_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_profiles ALTER COLUMN id SET DEFAULT nextval('cdr_profiles_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_servers ALTER COLUMN id SET DEFAULT nextval('cdr_servers_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdrs_v2 ALTER COLUMN id SET DEFAULT nextval('cdrs_v2_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY dbstorage_type ALTER COLUMN id SET DEFAULT nextval('dbtype_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY dst_tgroup ALTER COLUMN id SET DEFAULT nextval('dst_tgroup_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY dst_tgroup_deff ALTER COLUMN id SET DEFAULT nextval('dst_tgroup_deff_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY free_billsec_balance ALTER COLUMN id SET DEFAULT nextval('free_billsec_balance_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY pcard ALTER COLUMN id SET DEFAULT nextval('pcard_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY pcard_status ALTER COLUMN id SET DEFAULT nextval('pcard_status_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY pcard_type ALTER COLUMN id SET DEFAULT nextval('pcard_type_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY prefix ALTER COLUMN id SET DEFAULT nextval('prefix_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY prefix_filter ALTER COLUMN id SET DEFAULT nextval('prefix_filter_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY rate ALTER COLUMN id SET DEFAULT nextval('rate_id_seq1'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY src_tgroup ALTER COLUMN id SET DEFAULT nextval('src_tgroup_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY src_tgroup_deff ALTER COLUMN id SET DEFAULT nextval('src_tgroup_deff_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY tariff ALTER COLUMN id SET DEFAULT nextval('tariff_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY time_condition_deff ALTER COLUMN id SET DEFAULT nextval('time_conditions_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: global
--

ALTER TABLE ONLY users ALTER COLUMN id SET DEFAULT nextval('users_id_seq'::regclass);


--
-- Name: acc_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('acc_id_seq', 1, true);

--
-- Name: bal_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('bal_id_seq', 1, true);

--
-- Name: bill_plan_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('bill_plan_id_seq', 1, true);

--
-- Name: bill_plan_tree_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('bill_plan_tree_id_seq', 1, true);

--
-- Data for Name: bill_plan_type; Type: TABLE DATA; Schema: public; Owner: global
--

COPY bill_plan_type (id, name) FROM stdin;
1	prepaid
2	postpaid
\.

--
-- Name: billing_account_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('billing_account_id_seq', 1, true);

--
-- Name: binding_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('binding_id_seq', 1, true);

--
-- Name: cdr_dbstorage_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('cdr_dbstorage_id_seq', 1, true);

--
-- Name: cdr_files_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('cdr_files_id_seq', 1, true);

--
-- Data for Name: cdr_get_mode; Type: TABLE DATA; Schema: public; Owner: global
--

COPY cdr_get_mode (id, name) FROM stdin;
1	db
2	file
\.


--
-- Name: cdr_mode_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('cdr_mode_id_seq', 2, true);


--
-- Name: cdr_profiles_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('cdr_profiles_id_seq', 1, true);

--
-- Name: cdr_servers_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('cdr_servers_id_seq', 1, true);

--
-- Name: cdrs_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('cdrs_id_seq', 1, true);

--
-- Name: cdrs_v2_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('cdrs_v2_id_seq', 1, true);


--
-- Name: clg_deff__id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('clg_deff__id_seq', 1, false);


--
-- Name: clg_deff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('clg_deff_id_seq', 1, true);


--
-- Name: clg_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('clg_id_seq', 1, true);


--
-- Name: credit_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('credit_id_seq', 1, true);


--
-- Name: curr_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('curr_id_seq', 3, true);

--
-- Data for Name: currency; Type: TABLE DATA; Schema: public; Owner: global
--

COPY currency (id, name, to_bg) FROM stdin;
1	лв.	0
2	евро	1.95583
\.

--
-- Data for Name: dbstorage_type; Type: TABLE DATA; Schema: public; Owner: global
--

COPY dbstorage_type (id, name) FROM stdin;
1	pgsql
2	mysql
3	sqlite3
\.

--
-- Name: dbtype_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('dbtype_id_seq', 3, true);


--
-- Name: debit_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('debit_id_seq', 1, true);


--
-- Name: dst__deff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('dst__deff_id_seq', 1, true);

--
-- Name: dst_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('dst_id_seq', 1, true);

--
-- Name: dst_tgroup_deff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('dst_tgroup_deff_id_seq', 1, true);


--
-- Name: dst_tgroup_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('dst_tgroup_id_seq', 1, true);

--
-- Name: free_billsec_balance_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('free_billsec_balance_id_seq', 1, true);


--
-- Name: free_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('free_id_seq', 1, true);

--
-- Name: pcard_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('pcard_id_seq', 1, true);


--
-- Data for Name: pcard_status; Type: TABLE DATA; Schema: public; Owner: global
--

COPY pcard_status (id, status) FROM stdin;
0	deactive
1	active
2	block
\.


--
-- Name: pcard_status_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('pcard_status_id_seq', 1, false);


--
-- Data for Name: pcard_type; Type: TABLE DATA; Schema: public; Owner: global
--

COPY pcard_type (id, name, "desc") FROM stdin;
1	debit card	It's prepaid service.\nFirst push money,after speak
2	credit card	It's postpaid service.\nThe amount is credit limit.
\.


--
-- Name: pcard_type_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('pcard_type_id_seq', 1, false);

--
-- Name: prefix_filter_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('prefix_filter_id_seq', 1, true);


--
-- Name: prefix_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('prefix_id_seq', 1, true);


--
-- Name: rate_function_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('rate_function_id_seq', 1, true);


--
-- Name: rate_id_seq1; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('rate_id_seq1', 1, true);


--
-- Name: rating_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('rating_id_seq', 1, true);


--
-- Data for Name: rating_mode; Type: TABLE DATA; Schema: public; Owner: global
--

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
-- Data for Name: round_mode; Type: TABLE DATA; Schema: public; Owner: global
--

COPY round_mode (id, name, comm) FROM stdin;
0	no rounding	no rounding
1	ceil	Round fractions up(0,[1-9] = 1)
2	floor	Round fractions down(0,[1-9] = 0)
3	round	Round (a >= 0.5,a = 1;a < 0.5,a = 0)
\.


--
-- Name: src__deff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('src__deff_id_seq', 1, true);

--
-- Name: src_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('src_id_seq', 1, true);

--
-- Name: src_tgroup_deff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('src_tgroup_deff_id_seq', 1, true);


--
-- Name: src_tgroup_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('src_tgroup_id_seq', 1, true);


--
-- Name: tariff_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('tariff_id_seq', 1, true);


--
-- Name: tc_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('tc_id_seq', 1, true);

--
-- Data for Name: time_condition_deff; Type: TABLE DATA; Schema: public; Owner: global
--

COPY time_condition_deff (id, hours, days_week, tc_name, tc_date) FROM stdin;
1	07-21	mon-fri	pick-on-mobile	\N
2	21-07	mon-fri	pick-off-mobile	\N
3		sat-sun	weekend	\N
4	08-20	mon-fri	pick-on-fix	\N
5	20-08	mon-fri	pick-off-fix	\N
8	\N	\N	\N	2015-05-06
9				2015-01-01
10			2015-01-02	2015-01-02
11			2015-03-02	2015-03-02
12			2015-03-03	2015-03-03
13			2015-04-10	2015-04-10
14			2015-04-13	2015-04-13
15			2015-05-01	2015-05-01
16			2015-05-06	2015-05-06
17			2015-09-21	2015-09-21
18			2015-09-22	2015-09-22
19			2015-12-24	2015-12-24
20			2015-12-25	2015-12-25
21			2015-12-31	2015-12-31
\.


--
-- Name: time_conditions_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('time_conditions_id_seq', 3, true);

--
-- Name: users_id_seq; Type: SEQUENCE SET; Schema: public; Owner: global
--

SELECT pg_catalog.setval('users_id_seq', 1, true);


--
-- Data for Name: version; Type: TABLE DATA; Schema: public; Owner: global
--

COPY version (release, date) FROM stdin;
0.6.10(beta)	2017-09-15 10:36:45
\.


--
-- Name: acc_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY account_code
    ADD CONSTRAINT acc_pkey PRIMARY KEY (id);


--
-- Name: account_code_account_code_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY account_code
    ADD CONSTRAINT account_code_account_code_key UNIQUE (account_code);


--
-- Name: bal_id_pri; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY balance
    ADD CONSTRAINT bal_id_pri PRIMARY KEY (id);


--
-- Name: bill_plan_name_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY bill_plan
    ADD CONSTRAINT bill_plan_name_key UNIQUE (name);


--
-- Name: bill_plan_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY bill_plan
    ADD CONSTRAINT bill_plan_pkey PRIMARY KEY (id);


--
-- Name: bill_plan_tree_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY bill_plan_tree
    ADD CONSTRAINT bill_plan_tree_pkey PRIMARY KEY (id);


--
-- Name: billing_account_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY billing_account
    ADD CONSTRAINT billing_account_pkey PRIMARY KEY (id);


--
-- Name: billing_account_username_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY billing_account
    ADD CONSTRAINT billing_account_username_key UNIQUE (username);


--
-- Name: binding_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY account_code_deff
    ADD CONSTRAINT binding_pkey PRIMARY KEY (id);


--
-- Name: btype_id_uniq; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY bill_plan_type
    ADD CONSTRAINT btype_id_uniq UNIQUE (id);


--
-- Name: btype_name_uniq; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY bill_plan_type
    ADD CONSTRAINT btype_name_uniq UNIQUE (name);


--
-- Name: call_uid_uniq; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdrs
    ADD CONSTRAINT call_uid_uniq UNIQUE (call_uid);


--
-- Name: calln_unique; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY calling_number
    ADD CONSTRAINT calln_unique UNIQUE (calling_number);


--
-- Name: cdr_dbstorage_cdr_server_id_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_dbstorage
    ADD CONSTRAINT cdr_dbstorage_cdr_server_id_key UNIQUE (cdr_server_id);


--
-- Name: cdr_dbstorage_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_dbstorage
    ADD CONSTRAINT cdr_dbstorage_pkey PRIMARY KEY (id);


--
-- Name: cdr_files_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_files
    ADD CONSTRAINT cdr_files_pkey PRIMARY KEY (id);


--
-- Name: cdr_mode_name_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_get_mode
    ADD CONSTRAINT cdr_mode_name_key UNIQUE (name);


--
-- Name: cdr_mode_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_get_mode
    ADD CONSTRAINT cdr_mode_pkey PRIMARY KEY (id);


--
-- Name: cdr_profiles_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_profiles
    ADD CONSTRAINT cdr_profiles_pkey PRIMARY KEY (id);


--
-- Name: cdr_profiles_profile_name_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_profiles
    ADD CONSTRAINT cdr_profiles_profile_name_key UNIQUE (profile_name);


--
-- Name: cdr_servers_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdr_servers
    ADD CONSTRAINT cdr_servers_pkey PRIMARY KEY (id);


--
-- Name: cdrs_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdrs
    ADD CONSTRAINT cdrs_pkey PRIMARY KEY (id);


--
-- Name: cdrs_v2_call_uid_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdrs_v2
    ADD CONSTRAINT cdrs_v2_call_uid_key UNIQUE (call_uid);


--
-- Name: cdrs_v2_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY cdrs_v2
    ADD CONSTRAINT cdrs_v2_pkey PRIMARY KEY (id);


--
-- Name: clg_deff_id_pri; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY calling_number_deff
    ADD CONSTRAINT clg_deff_id_pri PRIMARY KEY (id);


--
-- Name: clg_id_pri; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY calling_number
    ADD CONSTRAINT clg_id_pri PRIMARY KEY (id);


--
-- Name: curr_id; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY currency
    ADD CONSTRAINT curr_id PRIMARY KEY (id);


--
-- Name: currency_name_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY currency
    ADD CONSTRAINT currency_name_key UNIQUE (name);


--
-- Name: dbtype_name_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY dbstorage_type
    ADD CONSTRAINT dbtype_name_key UNIQUE (name);


--
-- Name: dbtype_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY dbstorage_type
    ADD CONSTRAINT dbtype_pkey PRIMARY KEY (id);


--
-- Name: dst_context_dst_context_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY dst_context
    ADD CONSTRAINT dst_context_dst_context_key UNIQUE (dst_context);


--
-- Name: dst_deff__id_pri; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY dst_context_deff
    ADD CONSTRAINT dst_deff__id_pri PRIMARY KEY (id);


--
-- Name: dst_id_pri; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY dst_context
    ADD CONSTRAINT dst_id_pri PRIMARY KEY (id);


--
-- Name: dst_tgroup_deff_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY dst_tgroup_deff
    ADD CONSTRAINT dst_tgroup_deff_pkey PRIMARY KEY (id);


--
-- Name: dst_tgroup_dst_tgroup_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY dst_tgroup
    ADD CONSTRAINT dst_tgroup_dst_tgroup_key UNIQUE (dst_tgroup);


--
-- Name: dst_tgroup_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY dst_tgroup
    ADD CONSTRAINT dst_tgroup_pkey PRIMARY KEY (id);


--
-- Name: free_billsec_balance_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY free_billsec_balance
    ADD CONSTRAINT free_billsec_balance_pkey PRIMARY KEY (id);


--
-- Name: free_id_pri; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY free_billsec
    ADD CONSTRAINT free_id_pri PRIMARY KEY (id);


--
-- Name: pcard_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY pcard
    ADD CONSTRAINT pcard_pkey PRIMARY KEY (id);


--
-- Name: pcard_status_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY pcard_status
    ADD CONSTRAINT pcard_status_pkey PRIMARY KEY (id);


--
-- Name: pcard_status_status_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY pcard_status
    ADD CONSTRAINT pcard_status_status_key UNIQUE (status);


--
-- Name: pcard_type_name_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY pcard_type
    ADD CONSTRAINT pcard_type_name_key UNIQUE (name);


--
-- Name: pcard_type_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY pcard_type
    ADD CONSTRAINT pcard_type_pkey PRIMARY KEY (id);


--
-- Name: prefix_filter_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY prefix_filter
    ADD CONSTRAINT prefix_filter_pkey PRIMARY KEY (id);


--
-- Name: prefix_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY prefix
    ADD CONSTRAINT prefix_pkey PRIMARY KEY (id);


--
-- Name: prefix_prefix_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY prefix
    ADD CONSTRAINT prefix_prefix_key UNIQUE (prefix);


--
-- Name: rate_function_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY calc_function
    ADD CONSTRAINT rate_function_pkey PRIMARY KEY (id);


--
-- Name: rate_pkey1; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY rate
    ADD CONSTRAINT rate_pkey1 PRIMARY KEY (id);


--
-- Name: rating_mode_id_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY rating_mode
    ADD CONSTRAINT rating_mode_id_key UNIQUE (id);


--
-- Name: rating_mode_name_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY rating_mode
    ADD CONSTRAINT rating_mode_name_key UNIQUE (name);


--
-- Name: rating_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY rating
    ADD CONSTRAINT rating_pkey PRIMARY KEY (id);


--
-- Name: src_context_src_context_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY src_context
    ADD CONSTRAINT src_context_src_context_key UNIQUE (src_context);


--
-- Name: src_deff_id_pri; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY src_context_deff
    ADD CONSTRAINT src_deff_id_pri PRIMARY KEY (id);


--
-- Name: src_id_pri; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY src_context
    ADD CONSTRAINT src_id_pri PRIMARY KEY (id);


--
-- Name: src_tgroup_deff_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY src_tgroup_deff
    ADD CONSTRAINT src_tgroup_deff_pkey PRIMARY KEY (id);


--
-- Name: src_tgroup_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY src_tgroup
    ADD CONSTRAINT src_tgroup_pkey PRIMARY KEY (id);


--
-- Name: src_tgroup_src_tgroup_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY src_tgroup
    ADD CONSTRAINT src_tgroup_src_tgroup_key UNIQUE (src_tgroup);


--
-- Name: tariff_name_uniq; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY tariff
    ADD CONSTRAINT tariff_name_uniq UNIQUE (name);


--
-- Name: tc_id_pri; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY time_condition
    ADD CONSTRAINT tc_id_pri PRIMARY KEY (id);


--
-- Name: tc_name_uniq; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY time_condition_deff
    ADD CONSTRAINT tc_name_uniq UNIQUE (tc_name);


--
-- Name: time_conditions_pkey; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY time_condition_deff
    ADD CONSTRAINT time_conditions_pkey PRIMARY KEY (id);


--
-- Name: version_release_key; Type: CONSTRAINT; Schema: public; Owner: global
--

ALTER TABLE ONLY version
    ADD CONSTRAINT version_release_key UNIQUE (release);


--
-- Name: bal_bacc_id_ind; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX bal_bacc_id_ind ON balance USING btree (billing_account_id);


--
-- Name: bal_startd_ind; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX bal_startd_ind ON balance USING btree (start_date);


--
-- Name: cdr_storage_sched_uniq; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX cdr_storage_sched_uniq ON cdr_storage_sched USING btree (cdr_server_id);


--
-- Name: leg_a_ind; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX leg_a_ind ON cdrs USING btree (leg_a);


--
-- Name: leg_a_ind_v2; Type: INDEX; Schema: public; Owner: global
--

--CREATE INDEX leg_a_ind_v2 ON cdrs_v2 USING btree (leg_a);


--
-- Name: leg_b_ind; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX leg_b_ind ON cdrs USING btree (leg_b);


--
-- Name: leg_b_ind_v2; Type: INDEX; Schema: public; Owner: global
--

--CREATE INDEX leg_b_ind_v2 ON cdrs_v2 USING btree (leg_b);


--
-- Name: rate_bplan_id_ind; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX rate_bplan_id_ind ON rate USING btree (bill_plan_id);


--
-- Name: rating_call_id_ind; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX rating_call_id_ind ON rating USING btree (call_id);


--
-- Name: rating_call_ts_ind; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX rating_call_ts_ind ON rating USING btree (call_ts);


--
-- Name: rating_ind; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX rating_ind ON rating USING btree (billing_account_id);


--
-- Name: tariff_id_ind; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX tariff_id_ind ON tariff USING btree (id);


--
-- Name: tree_root_id_ind; Type: INDEX; Schema: public; Owner: global
--

CREATE INDEX tree_root_id_ind ON bill_plan_tree USING btree (root_bplan_id);


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--

