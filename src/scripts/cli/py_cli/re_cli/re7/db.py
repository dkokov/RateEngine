"""Ported data-access functions from lib.db.php (re7 schema).

Each PHP ``global $dbconn`` function becomes a module function taking a
:class:`~re_cli.database.Database` as its first argument. All queries are
parameterized (MIGRATION.md §3.1); getters return ``None`` / ``[]`` for
"not found" instead of the PHP ``0`` / ``""`` sentinels (§3.6).

Phase 2 ports the full ~120-function set. This first slice covers exactly the
functions the dump path (§4.6) depends on; the rest follow.
"""

from __future__ import annotations

import sys
from datetime import datetime
from typing import Any, Sequence, TextIO

from ..database import Database

# NOTE ON SHAPE: several lib.db.php functions mutated a PHP object passed by
# reference ($process / $task / $balance / DebitCard ...). Python has no such
# objects here, so those ports take the scalar inputs they need and *return* the
# resolved values (a dict or scalar) instead of mutating a caller object. This is
# the only structural deviation from the PHP; SQL and semantics are preserved.


def _leg_column(leg: str):
    """Compose a ``leg_<x>`` (or already-qualified) column identifier safely.

    The PHP built ``"leg_".$_leg`` and interpolated it into SQL. Several callers
    pass the bare suffix ("a"/"b"); a few pass the full column ("leg_a"). We
    normalize to ``leg_<suffix>`` and return a psycopg Identifier (never a value).
    """
    from psycopg import sql

    name = leg if str(leg).startswith("leg_") else f"leg_{leg}"
    return sql.Identifier(name)


def get_bill_plan_id(db: Database, bill_plan: str) -> int | None:
    """bill_plan.id by name, or None. PHP: get_bill_plan_id()."""
    return db.fetch_val("select id from bill_plan where name = %s", (bill_plan,))


def get_bill_plans_v2(db: Database, bid: int) -> list[dict[str, Any]]:
    """[{id, bplan}] for a single bill plan id. PHP: get_bill_plans_v2()."""
    return db.fetch_all(
        "select id, name as bplan from bill_plan where id = %s", (bid,)
    )


def get_bill_plans_tree(db: Database, tree_id: int) -> list[dict[str, Any]]:
    """[{id, bplan}] child plans of a tree root, ordered by id. PHP: get_bill_plans_tree()."""
    return db.fetch_all(
        "select bill_plan.id, bill_plan.name as bplan "
        "from bill_plan_tree, bill_plan "
        "where bill_plan_tree.bill_plan_id = bill_plan.id and root_bplan_id = %s "
        "order by bill_plan.id",
        (tree_id,),
    )


def get_rate_data(db: Database, bill_plan_id: int) -> list[dict[str, Any]]:
    """[{id, tariff_id, tariff, prefix_id, prefix}] for a bill plan. PHP: get_rate_data()."""
    return db.fetch_all(
        "select rate.id, rate.tariff_id, tariff.name as tariff, "
        "rate.prefix_id, prefix.prefix "
        "from rate, prefix, tariff "
        "where rate.tariff_id = tariff.id and rate.prefix_id = prefix.id "
        "and rate.bill_plan_id = %s order by rate.tariff_id",
        (bill_plan_id,),
    )


def get_free_billsec_v2(db: Database, tariff_id: int) -> dict[str, Any] | None:
    """{free_billsec_id, free_billsec} for a tariff, or None. PHP: get_free_billsec_v2()."""
    return db.fetch_one(
        "select tariff.free_billsec_id, free_billsec.free_billsec "
        "from tariff, free_billsec "
        "where tariff.id = %s and tariff.free_billsec_id = free_billsec.id",
        (tariff_id,),
    )


def get_calc_functions_v2(db: Database, tariff_id: int) -> list[dict[str, Any]]:
    """[{id, pos, delta_time, fee, iterations}] for a tariff. PHP: get_calc_functions_v2()."""
    if not tariff_id:
        return []
    return db.fetch_all(
        "select id, pos, delta_time, fee, iterations "
        "from calc_function where tariff_id = %s order by pos",
        (tariff_id,),
    )


# ---------------------------------------------------------------------------
# Import-path lookups (lib.db.php getters used by re6_import_settings)
# ---------------------------------------------------------------------------

def get_bill_plan_type_id(db: Database, bill_plan_type: str) -> int | None:
    """bill_plan_type.id by name, or None. PHP: get_bill_plan_type_id()."""
    return db.fetch_val(
        "select id from bill_plan_type where name = %s", (bill_plan_type,)
    )


def get_prefix_id(db: Database, prefix: str) -> int | None:
    """prefix.id by prefix string, or None. PHP: get_prefix_id()."""
    return db.fetch_val("select id from prefix where prefix = %s", (prefix,))


def get_tariff_id(db: Database, bill_plan_id: int, tariff: str) -> int | None:
    """tariff id by name (two-step, mirrors PHP get_tariff_id()).

    First tries an existing rate->tariff join by name; if none, falls back to a
    tariff staged for this bill plan (``temp_id``). Returns None if neither.
    """
    rid = db.fetch_val(
        "select rt.tariff_id from rate as rt, tariff as tr "
        "where tr.name = %s and rt.tariff_id = tr.id",
        (tariff,),
    )
    if rid:
        return rid
    return db.fetch_val(
        "select id from tariff where name = %s and temp_id = %s",
        (tariff, bill_plan_id),
    )


def get_rate_id(db: Database, bill_plan_id: int, prefix_id: int, tariff_id: int) -> int | None:
    """rate.id for (bill_plan, prefix, tariff), or None. PHP: get_rate_id()."""
    return db.fetch_val(
        "select id from rate where tariff_id = %s and bill_plan_id = %s and prefix_id = %s",
        (tariff_id, bill_plan_id, prefix_id),
    )


def get_calc_id(db: Database, tariff_id: int, pos: int) -> int | None:
    """calc_function.id for (tariff, pos), or None. PHP: get_calc_id()."""
    return db.fetch_val(
        "select id from calc_function where tariff_id = %s and pos = %s",
        (tariff_id, pos),
    )


def get_billing_account_id(db: Database, billing_account: str) -> int | None:
    """billing_account.id by username, or None. PHP: get_billing_account_id()."""
    return db.fetch_val(
        "select id from billing_account where username = %s", (billing_account,)
    )


def get_rating_mode(db: Database, mode_id: int) -> str | None:
    """rating_mode.name by id, or None. PHP: get_rating_mode()."""
    return db.fetch_val("select name from rating_mode where id = %s", (mode_id,))


def get_rating_account_id(db: Database, rating_mode: str, rating_account: str) -> int | None:
    """id of a rating account in the per-mode table (e.g. calling_number).

    The rating mode name is both the table and column name, so it is composed as
    an identifier — never interpolated as a value. PHP: get_rating_account_id().
    """
    from psycopg import sql

    query = sql.SQL("select id from {tbl} where {col} = %s").format(
        tbl=sql.Identifier(rating_mode), col=sql.Identifier(rating_mode)
    )
    return db.fetch_val(query, (rating_account,))


def get_pcard_type_id(db: Database, pcard_type: str) -> int | None:
    """pcard_type.id by name prefix (LIKE 'name%'), or None. PHP: get_pcard_type_id()."""
    return db.fetch_val(
        "select id from pcard_type where name like %s", (pcard_type + "%",)
    )


def get_pcard_status_id(db: Database, pcard_status: str) -> int | None:
    """pcard_status.id by status prefix (LIKE 'status%'), or None. PHP: get_pcard_status_id()."""
    return db.fetch_val(
        "select id from pcard_status where status like %s", (pcard_status + "%",)
    )


def get_pcard_id(
    db: Database, bacc: int, type_id: int, status_id: int, amount, start, end
) -> int | None:
    """pcard.id matching all of (account, type, status, amount, dates), or None."""
    return db.fetch_val(
        "select id from pcard where billing_account_id = %s and pcard_type_id = %s "
        "and pcard_status_id = %s and amount = %s and start_date = %s and end_date = %s",
        (bacc, type_id, status_id, amount, start, end),
    )


def get_bill_tree_id(db: Database, tree: int, root: int) -> int | None:
    """bill_plan_tree.id linking child `tree` under `root`, or None. PHP: get_bill_tree_id()."""
    return db.fetch_val(
        "select id from bill_plan_tree where bill_plan_id = %s and root_bplan_id = %s",
        (tree, root),
    )


# ---------------------------------------------------------------------------
# Import-path inserts (lib.db.php inserters). Each returns the new row id via
# RETURNING, replacing the PHP "insert then re-select via goto" pattern (§3.3).
# ---------------------------------------------------------------------------

def insert_bill_plan(
    db: Database, bill_plan: str, bill_plan_type_id: int, start_period: int, end_period: int
) -> int:
    """Insert a bill plan, return its id. PHP: insert_bill_plan()."""
    return db.insert_returning(
        "insert into bill_plan (name, bill_plan_type_id, start_period, end_period) "
        "values (%s, %s, %s, %s) returning id",
        (bill_plan, bill_plan_type_id, start_period, end_period),
    )


def insert_bill_plan_v2(db: Database, bill_plan: str) -> int:
    """Insert a name-only bill plan (tree root), return its id. PHP: insert_bill_plan_v2()."""
    return db.insert_returning(
        "insert into bill_plan (name) values (%s) returning id", (bill_plan,)
    )


def insert_bill_tree(db: Database, bill_plan_id: int, root_bplan_id: int) -> None:
    """Link a child bill plan under a tree root. PHP: insert_bill_tree()."""
    db.execute(
        "insert into bill_plan_tree (bill_plan_id, root_bplan_id) values (%s, %s)",
        (bill_plan_id, root_bplan_id),
    )


def insert_tariff(
    db: Database, bill_plan_id: int, tariff: str, start: int, end: int, free_billsec_id: int
) -> int:
    """Insert a tariff (temp_id = bill plan), return its id. PHP: insert_tariff()."""
    return db.insert_returning(
        "insert into tariff (name, temp_id, start_period, end_period, free_billsec_id) "
        "values (%s, %s, %s, %s, %s) returning id",
        (tariff, bill_plan_id, start, end, free_billsec_id),
    )


def insert_billing_account(
    db: Database, billing_account: str, curr_id: int, leg: str, cdr_server_id: int
) -> int:
    """Insert a billing account, return its id. PHP: insert_billing_account()."""
    return db.insert_returning(
        "insert into billing_account (username, currency_id, leg, cdr_server_id) "
        "values (%s, %s, %s, %s) returning id",
        (billing_account, curr_id, leg, cdr_server_id),
    )


def insert_rate(db: Database, bill_plan_id: int, prefix_id: int, tariff_id: int) -> int:
    """Insert a rate, return its id. PHP: insert_rate()."""
    return db.insert_returning(
        "insert into rate (bill_plan_id, prefix_id, tariff_id) "
        "values (%s, %s, %s) returning id",
        (bill_plan_id, prefix_id, tariff_id),
    )


def insert_prefix(db: Database, prefix: str, desc: str | None) -> int:
    """Insert a prefix (with optional comment), return its id. PHP: insert_prefix()."""
    if not desc:
        return db.insert_returning(
            "insert into prefix (prefix) values (%s) returning id", (prefix,)
        )
    return db.insert_returning(
        "insert into prefix (prefix, comm) values (%s, %s) returning id", (prefix, desc)
    )


def insert_pcard(
    db: Database, bacc_id: int, pcard_tp: int, pcard_sts: int, amount, start_date, end_date
) -> int:
    """Insert a prepaid card, return its id. PHP: insert_pcard().

    Note: the PHP wrote the literal string ``'now()'`` into last_update (an invalid
    timestamp); we use the SQL ``now()`` function instead — see FIX-5 in MIGRATION.md.
    Empty start/end dates become SQL NULL rather than '' (which a date column rejects).
    """
    return db.insert_returning(
        "insert into pcard (billing_account_id, pcard_status_id, pcard_type_id, amount, "
        "start_date, end_date, last_update) values (%s, %s, %s, %s, %s, %s, now()) returning id",
        (bacc_id, pcard_sts, pcard_tp, amount, start_date or None, end_date or None),
    )


def insert_rating_account(
    db: Database, rating_mode: str, rating_account: str, billing_account_id: int, bill_plan_id: int
) -> int:
    """Create a rating account + its _deff row, return the account id.

    Inserts into the per-mode table and ``<mode>_deff``; the mode name drives the
    table/column identifiers (composed, never interpolated). PHP: insert_rating_account().
    """
    from psycopg import sql

    ins = sql.SQL(
        "insert into {tbl} ({col}, billing_account_id) values (%s, %s) returning id"
    ).format(tbl=sql.Identifier(rating_mode), col=sql.Identifier(rating_mode))
    rid = db.insert_returning(ins, (rating_account, billing_account_id))

    deff = sql.SQL(
        "insert into {tbl} ({col}, bill_plan_id) values (%s, %s)"
    ).format(tbl=sql.Identifier(f"{rating_mode}_deff"), col=sql.Identifier(f"{rating_mode}_id"))
    db.execute(deff, (rid, bill_plan_id))
    return rid


def insert_rating_account_2(
    db: Database,
    rating_mode: str,
    rating_account: str,
    billing_account_id: int,
    bill_plan_id: int,
    clg_nadi,
    cld_nadi,
) -> int:
    """As insert_rating_account, plus clg_nadi/cld_nadi on the _deff row.

    Used for rating modes >= 5. PHP: insert_rating_account_2().
    """
    from psycopg import sql

    ins = sql.SQL(
        "insert into {tbl} ({col}, billing_account_id) values (%s, %s) returning id"
    ).format(tbl=sql.Identifier(rating_mode), col=sql.Identifier(rating_mode))
    rid = db.insert_returning(ins, (rating_account, billing_account_id))

    deff = sql.SQL(
        "insert into {tbl} ({col}, bill_plan_id, clg_nadi, cld_nadi) values (%s, %s, %s, %s)"
    ).format(tbl=sql.Identifier(f"{rating_mode}_deff"), col=sql.Identifier(f"{rating_mode}_id"))
    db.execute(deff, (rid, bill_plan_id, clg_nadi, cld_nadi))
    return rid


def insert_calc_func(db: Database, tariff_id: int, pos: int, fee, delta_time, iterations) -> None:
    """Insert a calc function; omits `iterations` when empty. PHP: insert_calc_func()."""
    if iterations in (None, "", 0):
        db.execute(
            "insert into calc_function (tariff_id, pos, fee, delta_time) values (%s, %s, %s, %s)",
            (tariff_id, pos, fee, delta_time),
        )
    else:
        db.execute(
            "insert into calc_function (tariff_id, pos, fee, delta_time, iterations) "
            "values (%s, %s, %s, %s, %s)",
            (tariff_id, pos, fee, delta_time, iterations),
        )


# ===========================================================================
# CDRs, servers, storage (lib.db.php)
# ===========================================================================

def get_cdr(db: Database, uniqueid: str) -> dict[str, Any] | None:
    """CDR fields by uniqueid: {called_number, accountcode, billsec, timestamp}. PHP: get_cdr()."""
    return db.fetch_one(
        "select dst as called_number, accountcode, billsec, calldate as timestamp "
        "from cdrs where uniqueid = %s",
        (uniqueid,),
    )


def get_cdr_2(db: Database, cdr_id: int) -> dict[str, Any] | None:
    """Full CDR row by id (with EXTRACT(DOW)). PHP: get_cdr_2()."""
    return db.fetch_one(
        "select call_uid, calling_number, called_number, src_context, dst_context, "
        "account_code, billsec, ts as timestamp, extract(dow from ts) as dow, id as call_id "
        "from cdrs where id = %s",
        (cdr_id,),
    )


def get_cdr_servers(db: Database) -> list[dict[str, Any]]:
    """Active CDR servers ordered by id. PHP: get_cdr_servers()."""
    return db.fetch_all(
        "select id as cdr_server_id, server_name, switch_type_id, get_mode_id "
        "from cdr_servers where active = 't' order by id"
    )


def get_dbstorage(db: Database, server_id: int) -> dict[str, Any] | None:
    """CDR storage config for a server. PHP: get_dbstorage() (selected * -> named cols)."""
    return db.fetch_one(
        "select dbhost, dbname, dbuser, dbpass, cdr_table, dbstorage_type_id "
        "from cdr_dbstorage where cdr_server_id = %s",
        (server_id,),
    )


def get_calling_number_cdrs(db: Database) -> list[str]:
    """uniqueids from cdr_ast with empty accountcode not yet queued. PHP: get_calling_number_cdrs()."""
    rows = db.fetch_all(
        "select a.uniqueid from cdr_ast as a "
        "where a.accountcode = '' and not exists ("
        "  select 1 from cdr_queue_calling_number as q where q.uniqueid = a.uniqueid)"
    )
    return [r["uniqueid"] for r in rows]


def get_accountcode_cdrs(db: Database) -> list[str]:
    """uniqueids from cdr_ast with non-empty accountcode not yet queued. PHP: get_accountcode_cdrs()."""
    rows = db.fetch_all(
        "select a.uniqueid from cdr_ast as a "
        "where a.accountcode != '' and not exists ("
        "  select 1 from cdr_queue_accountcode as q where q.uniqueid = a.uniqueid)"
    )
    return [r["uniqueid"] for r in rows]


def get_norating_cdr(db: Database, leg: str) -> list[int]:
    """Ids of CDRs whose leg column is 0, oldest first. PHP: get_norating_cdr()."""
    from psycopg import sql

    query = sql.SQL("select id from cdrs where {leg} = 0 order by ts").format(leg=_leg_column(leg))
    return [r["id"] for r in db.fetch_all(query)]


# ===========================================================================
# Cards & balances
# ===========================================================================

def get_debit_card(db: Database, card_id: int) -> dict[str, Any] | None:
    """Active debit card by id. PHP: get_debit_card()."""
    return db.fetch_one(
        "select id, billing_account_id, amount, end_date, last_update, active, start_date "
        "from debit_card where id = %s and active = 't'",
        (card_id,),
    )


def get_credit_card(db: Database, card_id: int) -> dict[str, Any] | None:
    """Credit card (id, amount) by id. PHP: get_credit_card()."""
    return db.fetch_one("select id, amount from credit_card where id = %s", (card_id,))


def get_credit_card_id(db: Database, bacc: int, amount) -> int | None:
    """credit_card.id by (account, amount). PHP: get_credit_card_id()."""
    return db.fetch_val(
        "select id from credit_card where billing_account_id = %s and amount = %s",
        (bacc, amount),
    )


def get_debit_card_id(db: Database, bacc: int, amount, start, end) -> int | None:
    """debit_card.id by (account, amount, dates). PHP: get_debit_card_id()."""
    return db.fetch_val(
        "select id from debit_card where billing_account_id = %s and amount = %s "
        "and start_date = %s and end_date = %s",
        (bacc, amount, start, end),
    )


def get_balance(db: Database, billing_account_id: int) -> dict[str, Any] | None:
    """Balance row for a billing account. PHP: get_balance()."""
    return db.fetch_one(
        "select id, debit_card_id, credit_card_id, amount, last_update "
        "from balance where billing_account_id = %s",
        (billing_account_id,),
    )


def get_balance_id(db: Database, bacc: int, debit: int, credit: int) -> int | None:
    """balance.id by (account, debit card, credit card). PHP: get_balance_id()."""
    return db.fetch_val(
        "select id from balance where billing_account_id = %s and debit_card_id = %s "
        "and credit_card_id = %s",
        (bacc, debit, credit),
    )


def get_bacc_balance(db: Database) -> list[int]:
    """All billing_account ids that have a balance, ordered. PHP: get_bacc_balance()."""
    return [r["billing_account_id"]
            for r in db.fetch_all(
                "select billing_account_id from balance order by billing_account_id")]


def make_balance_debit(db: Database, billing_account_id: int, start_date, end_date):
    """Sum of call prices in the debit-card period (or None). PHP: make_balance_debit().

    Returns the computed amount; the PHP wrote it onto $balance->Amount.
    """
    return db.fetch_val(
        "select sum(rt.call_price) from rating as rt, cdrs "
        "where cdrs.id = rt.call_id and rt.billing_account_id = %s "
        "and cdrs.ts >= %s and cdrs.ts <= %s",
        (billing_account_id, f"{start_date} 00:00:00", f"{end_date} 23:59:59"),
    )


def make_balance_credit(db: Database, billing_account_id: int):
    """Sum of positive call prices for the current month. PHP: make_balance_credit().

    Reproduces the PHP date math (current month start to next month start). Returns
    the sum, or Decimal(0) when there are no qualifying calls.
    """
    from decimal import Decimal

    now = datetime.now()
    year, month = now.year, now.month
    nxt = month + 1
    end_year = year + 1 if nxt == 13 else year
    end_month = 1 if nxt == 13 else nxt
    start = f"{year}-{month:02d}-01 00:00:00"
    end = f"{end_year}-{end_month:02d}-01 00:00:00"

    val = db.fetch_val(
        "select sum(rt.call_price) from rating as rt, cdrs "
        "where cdrs.id = rt.call_id and rt.call_price > 0 and rt.billing_account_id = %s "
        "and cdrs.ts >= %s and cdrs.ts <= %s",
        (billing_account_id, start, end),
    )
    return val if val is not None else Decimal("0.00")


# ===========================================================================
# Account / context resolution (PHP find_* mutators -> return dicts)
# ===========================================================================

def find_billing_account_id(db: Database, billing_account: str) -> int | None:
    """billing_account.id by username, or None. PHP: find_billing_account_id()."""
    if not billing_account:
        return None
    return db.fetch_val(
        "select id from billing_account where username = %s", (billing_account,)
    )


def find_billing_account_data(db: Database, billing_account: str) -> dict[str, Any] | None:
    """{id, leg, curr} by username (joins currency). PHP: find_billing_account_data()."""
    if not billing_account:
        return None
    return db.fetch_one(
        "select bacc.id, bacc.leg, cr.name as curr "
        "from billing_account as bacc, currency as cr "
        "where bacc.username = %s and bacc.currency_id = cr.id",
        (billing_account,),
    )


def find_billing_account_data_2(db: Database, bacc_id: int) -> dict[str, Any] | None:
    """{id, leg, curr} by billing account id. PHP: find_billing_account_data_2()."""
    if not bacc_id:
        return None
    return db.fetch_one(
        "select bacc.id, bacc.leg, cr.name as curr "
        "from billing_account as bacc, currency as cr "
        "where bacc.id = %s and bacc.currency_id = cr.id",
        (bacc_id,),
    )


def find_accountcode(db: Database, account_code: str) -> dict[str, Any] | None:
    """{bill_plan_id, billing_account_id} for an account code. PHP: find_accountcode()."""
    if not account_code:
        return None
    return db.fetch_one(
        "select acc_deff.bill_plan_id, acc.billing_account_id "
        "from account_code as acc, account_code_deff as acc_deff "
        "where acc.account_code = %s and acc.id = acc_deff.account_code_id",
        (account_code,),
    )


def find_calling_number(db: Database, calling_number: str) -> dict[str, Any] | None:
    """{bill_plan_id, billing_account_id} for a calling number. PHP: find_calling_number()."""
    return db.fetch_one(
        "select deff.bill_plan_id, cn.billing_account_id "
        "from calling_number as cn, calling_number_deff as deff "
        "where cn.calling_number = %s and cn.id = deff.calling_number_id",
        (calling_number,),
    )


def find_src_context(db: Database, src_context: str) -> dict[str, Any] | None:
    """{bill_plan_id, billing_account_id} for a source context. PHP: find_src_context()."""
    if not src_context:
        return None
    return db.fetch_one(
        "select df.bill_plan_id, src.billing_account_id "
        "from src_context as src, src_context_deff as df "
        "where src.src_context = %s and src.id = df.src_context_id",
        (src_context,),
    )


def find_dst_context(db: Database, dst_context: str) -> dict[str, Any] | None:
    """{bill_plan_id, billing_account_id} for a destination context. PHP: find_dst_context()."""
    if not dst_context:
        return None
    return db.fetch_one(
        "select df.bill_plan_id, dst.billing_account_id "
        "from dst_context as dst, dst_context_deff as df "
        "where dst.dst_context = %s and dst.id = df.dst_context_id",
        (dst_context,),
    )


def find_prefix(db: Database) -> list[dict[str, Any]]:
    """All prefixes, longest first. PHP: find_prefix() (FIX-6: PHP SQL was broken —
    ``select id,prefix from order by ...`` had no table; reconstructed as `prefix`)."""
    return db.fetch_all("select id, prefix from prefix order by prefix desc")


# ===========================================================================
# Bill plans / tariffs / prefixes (remaining lookups)
# ===========================================================================

def get_bill_plans(db: Database) -> list[dict[str, Any]]:
    """All bill plans with timestamped periods. PHP: get_bill_plans()."""
    return db.fetch_all(
        "select id, name as bplan, to_timestamp(start_period) as speriod, "
        "to_timestamp(end_period) as eperiod from bill_plan"
    )


def find_bill_plan(db: Database, bill_plan_id: int) -> list[dict[str, Any]]:
    """Rate rows for a bill plan, prefixes longest first. PHP: find_bill_plan()."""
    return db.fetch_all(
        "select rt.tariff_id, pr.prefix, pr.id as prefix_id, rt.id as rate_id, "
        "rt.free_billsec_id from rate as rt, prefix as pr "
        "where rt.prefix_id = pr.id and rt.bill_plan_id = %s order by pr.prefix desc",
        (bill_plan_id,),
    )


def find_bill_plan_periods(db: Database, bill_plan_id: int) -> dict[str, Any] | None:
    """{start, end} periods for a bill plan. PHP: find_bill_plan_periods()."""
    return db.fetch_one(
        "select start_period as start, end_period as end from bill_plan where id = %s",
        (bill_plan_id,),
    )


def get_tariff_name(db: Database, billing_account_id: int) -> list[str]:
    """Distinct tariff names billed to an account. PHP: get_tariff_name()."""
    rows = db.fetch_all(
        "select tr.name from rating as rt, rate, tariff as tr "
        "where rt.billing_account_id = %s and rt.rate_id = rate.id "
        "and tr.id = rate.tariff_id group by tr.name",
        (billing_account_id,),
    )
    return [r["name"] for r in rows]


def get_prefix_filters(db: Database) -> list[dict[str, Any]]:
    """Prefix filters with replacement strings. PHP: get_prefix_filters()."""
    return db.fetch_all(
        "select filtering_prefix as prefix, filtering_number as num, replace_str as replace "
        "from prefix_filter"
    )


# ===========================================================================
# Time conditions & celebration days
# ===========================================================================

def get_free_id(db: Database, tariff_id: int) -> int | None:
    """free_billsec.id for a tariff, or None. PHP: get_free_id()."""
    return db.fetch_val("select id from free_billsec where tariff_id = %s", (tariff_id,))


def get_time_condition_id(db: Database, tariff_id: int, time_zone: str) -> int | None:
    """time_condition.id for (tariff, tc_name), or None. PHP: get_time_condition_id()."""
    return db.fetch_val(
        "select tc.id from time_condition as tc, time_condition_deff as deff "
        "where deff.tc_name = %s and deff.id = tc.time_condition_id and tc.tariff_id = %s",
        (time_zone, tariff_id),
    )


def find_time_condition(db: Database, tariff_id: int) -> list[dict[str, Any]]:
    """Time-condition definitions for a tariff. PHP: find_time_condition()."""
    return db.fetch_all(
        "select df.id, df.hours, df.days_week, df.month, df.day_month "
        "from time_condition as tc, time_condition_deff as df "
        "where tc.tariff_id = %s and tc.time_condition_id = df.id order by tc.id",
        (tariff_id,),
    )


def find_celebr_days(db: Database, ts: dict[str, Any]) -> int | None:
    """celebration_dates.id matching the ts year-month prefix. PHP: find_celebr_days()."""
    dt = f"{ts['year']}-{ts['month']}"
    return db.fetch_val(
        "select id from celebration_dates where date like %s", (dt + "%",)
    )


def find_celebr_dt_deff(db: Database, dt_id: int) -> dict[str, Any] | None:
    """celebration_dates row by id. PHP: find_celebr_dt_deff() (FIX-1: PHP referenced an
    undefined ``$dt`` and returned an undefined ``$tc``; reconstructed as a by-id lookup)."""
    return db.fetch_one("select id, date from celebration_dates where id = %s", (dt_id,))


def get_tc_name(db: Database, tc_id: int) -> str | None:
    """tc_name via time_condition -> _deff join. PHP: get_tc_name() (debug echo dropped)."""
    return db.fetch_val(
        "select tc_df.tc_name from time_condition_deff as tc_df, time_condition as tc "
        "where tc_df.id = tc.time_condition_id and tc.id = %s",
        (tc_id,),
    )


def get_tc_name_2(db: Database, tc_id: int) -> str | None:
    """tc_name directly from time_condition_deff by id. PHP: get_tc_name_2()."""
    return db.fetch_val(
        "select tc_name from time_condition_deff where id = %s", (tc_id,)
    )


# ===========================================================================
# Calc functions & free billsec (rating-runtime variants)
# ===========================================================================

def get_calc_functions(db: Database, tariff_id: int) -> dict[int, dict[str, Any]]:
    """Calc functions keyed by pos: {pos: {id, delta_time, fee, iterations}}. PHP: get_calc_functions()."""
    out: dict[int, dict[str, Any]] = {}
    if not tariff_id:
        return out
    for row in db.fetch_all(
        "select id, pos, delta_time, fee, iterations from calc_function "
        "where tariff_id = %s order by pos",
        (tariff_id,),
    ):
        out[row["pos"]] = {
            "id": row["id"],
            "delta_time": row["delta_time"],
            "fee": row["fee"],
            "iterations": row["iterations"],
        }
    return out


def get_free_billsec(db: Database, tariff_id: int, rate_id: int):
    """free_billsec value for a tariff when rate_id > 0, else 0. PHP: get_free_billsec()."""
    if not rate_id or rate_id <= 0:
        return 0
    val = db.fetch_val(
        "select free_billsec from free_billsec where tariff_id = %s", (tariff_id,)
    )
    return val if val is not None else 0


def get_free_billsec_id(db: Database, free_billsec_id: int):
    """free_billsec value by free_billsec id, else 0. PHP: get_free_billsec_id()."""
    val = db.fetch_val(
        "select free_billsec from free_billsec where id = %s", (free_billsec_id,)
    )
    return val if val is not None else 0


def get_free_bill(db: Database, billing_account_id: int):
    """Sum of negative call billsec (free traffic) for an account. PHP: get_free_bill()."""
    val = db.fetch_val(
        "select sum(call_billsec) from rating where billing_account_id = %s and call_price < 0",
        (billing_account_id,),
    )
    return val if val is not None else 0


# ===========================================================================
# Billing checks & reports
# ===========================================================================

def check_bill(db: Database, billing_account_id: int, leg: str, time1: str, time2: str):
    """Sum of positive call prices for a billing period. PHP: check_bill()."""
    from psycopg import sql

    query = sql.SQL(
        "select sum(rt.call_price) from rating as rt, cdrs "
        "where rt.billing_account_id = %s and cdrs.ts >= %s and cdrs.ts < %s "
        "and cdrs.{leg} = rt.id and rt.call_price > 0"
    ).format(leg=_leg_column(leg))
    val = db.fetch_val(query, (billing_account_id, f"{time1} 00:00:00", f"{time2} 00:00:00"))
    return val if val is not None else 0


def check_free_bill(db: Database, billing_account_id: int, time1: str, time2: str):
    """Sum of negative call prices (free) for a period. PHP: check_free_bill()."""
    val = db.fetch_val(
        "select sum(rt.call_price) from rating as rt, cdrs "
        "where rt.billing_account_id = %s and cdrs.id = rt.call_id "
        "and cdrs.ts >= %s and cdrs.ts < %s and rt.call_price < 0",
        (billing_account_id, f"{time1} 00:00:00", f"{time2} 00:00:00"),
    )
    return val if val is not None else 0


def get_tariff_stat(
    db: Database, billing_account_id: int, leg: str, time1: str, time2: str, tariff: str,
    out: TextIO | None = None,
) -> None:
    """Print 'tariff,calls,leva,mins' for a tariff over a period. PHP: get_tariff_stat()."""
    from psycopg import sql

    out = out or sys.stdout
    query = sql.SQL(
        "select count(rt.id) as calls, sum(rt.call_price) as leva, sum(rt.call_billsec) as mins "
        "from rating as rt, cdrs as cd, tariff as tr, rate "
        "where tr.name = %s and rt.billing_account_id = %s and rate.id = rt.rate_id "
        "and rate.tariff_id = tr.id and cd.{leg} = rt.id "
        "and cd.ts < %s and cd.ts >= %s"
    ).format(leg=_leg_column(leg))
    for row in db.fetch_all(query, (tariff, billing_account_id, f"{time2} 00:00:00", f"{time1} 00:00:00")):
        mins = round((row["mins"] or 0) / 60, 2)
        out.write(f"{tariff},{row['calls']},{row['leva']},{mins}\n")


def get_rating_calls(
    db: Database, billing_account_id: int, leg: str, time1: str, time2: str,
    out: TextIO | None = None,
) -> None:
    """Print rated-call detail lines for an account/period. PHP: get_rating_calls()."""
    from psycopg import sql

    out = out or sys.stdout
    query = sql.SQL(
        "select cd.call_uid, cd.ts, cd.calling_number, cd.called_number, rt.call_price, "
        "rt.call_billsec, tr.name, rate.id, rt.time_condition_id "
        "from rating as rt, cdrs as cd, tariff as tr, rate "
        "where rt.billing_account_id = %s and rate.id = rt.rate_id and rate.tariff_id = tr.id "
        "and cd.{leg} = rt.id and cd.ts < %s and cd.ts >= %s order by cd.ts"
    ).format(leg=_leg_column(leg))
    for row in db.fetch_all(query, (billing_account_id, f"{time2} 00:00:00", f"{time1} 00:00:00")):
        vals = list(row.values())
        tc_id = vals[8]
        tc_name = get_tc_name_2(db, tc_id)
        out.write(",".join(str(v) for v in vals[:7]) + f",{tc_name}[{tc_id}]\n")


def get_norating_calls(
    db: Database, billing_account_id: int, time1: str, time2: str, out: TextIO | None = None,
) -> None:
    """Print call lines joined via call_id (no leg). PHP: get_norating_calls()."""
    out = out or sys.stdout
    rows = db.fetch_all(
        "select cd.call_uid, cd.ts, cd.calling_number, cd.called_number, rt.call_price, "
        "rt.call_billsec, tr.name from rating as rt, cdrs as cd, tariff as tr, rate "
        "where rt.billing_account_id = %s and rate.id = rt.rate_id and rate.tariff_id = tr.id "
        "and cd.id = rt.call_id and cd.ts < %s and cd.ts >= %s order by cd.ts",
        (billing_account_id, f"{time2} 00:00:00", f"{time1} 00:00:00"),
    )
    for row in rows:
        out.write(",".join(str(v) for v in row.values()) + "\n")


def clear_rating(db: Database, billing_account_id: int) -> int:
    """Delete rating rows for a billing account, return count deleted. PHP: clear_rating()
    (FIX-2: the PHP built the DELETE but never executed it; here it runs)."""
    return db.execute(
        "delete from rating where billing_account_id = %s", (billing_account_id,)
    )


# ===========================================================================
# Writes: rating, balance, CDRs, marks, updates
# ===========================================================================

def write_rating_in_db(
    db: Database, rate_id: int, call_price, call_billsec, call_id: int,
    billing_account_id: int, rating_mode_id: int, time_condition_id: int,
) -> int:
    """Insert a rating row, return its id (RETURNING replaces the PHP reselect)."""
    return db.insert_returning(
        "insert into rating (rate_id, call_price, call_billsec, call_id, billing_account_id, "
        "rating_mode_id, time_condition_id) values (%s, %s, %s, %s, %s, %s, %s) returning id",
        (rate_id, call_price, call_billsec, call_id, billing_account_id,
         rating_mode_id, time_condition_id),
    )


def write_balance(db: Database, balance_id: int, amount, last_update) -> None:
    """Update a balance's amount + last_update. PHP: write_balance()."""
    if not balance_id:
        return
    db.execute(
        "update balance set amount = %s, last_update = %s where id = %s",
        (amount, last_update, balance_id),
    )


def insert_credit_card(db: Database, bacc: int, amount) -> int:
    """Insert a credit card, return its id. PHP: insert_credit_card()."""
    return db.insert_returning(
        "insert into credit_card (billing_account_id, amount) values (%s, %s) returning id",
        (bacc, amount),
    )


def insert_debit_card(db: Database, bacc: int, amount, start, end) -> int:
    """Insert a debit card, return its id. PHP: insert_debit_card() (uses SQL now(), see FIX-5)."""
    return db.insert_returning(
        "insert into debit_card (billing_account_id, amount, start_date, end_date, last_update) "
        "values (%s, %s, %s, %s, now()) returning id",
        (bacc, amount, start or None, end or None),
    )


def insert_balance(db: Database, bacc: int, debit: int, credit: int) -> int:
    """Insert a balance row, return its id. PHP: insert_balance() (uses SQL now(), see FIX-5)."""
    return db.insert_returning(
        "insert into balance (billing_account_id, debit_card_id, credit_card_id, last_update) "
        "values (%s, %s, %s, now()) returning id",
        (bacc, debit, credit),
    )


def insert_free_billsec(db: Database, free_billsec, tariff_id: int) -> None:
    """Insert a free_billsec row. PHP: insert_free_billsec()."""
    db.execute(
        "insert into free_billsec (free_billsec, tariff_id) values (%s, %s)",
        (free_billsec, tariff_id),
    )


def insert_time_condition(db: Database, tariff_id: int, time_zone: str) -> None:
    """Link a tariff to a named time condition. PHP: insert_time_condition()
    (raises CliError if the tc_name is unknown, replacing the PHP echo+exit)."""
    tc_id = db.fetch_val(
        "select id from time_condition_deff where tc_name = %s", (time_zone,)
    )
    if not tc_id:
        from ..errors import CliError
        raise CliError(f"unknown time condition: {time_zone!r}")
    db.execute(
        "insert into time_condition (tariff_id, time_condition_id) values (%s, %s)",
        (tariff_id, tc_id),
    )


def mark_cdr(db: Database, uniqueid: str) -> None:
    """Queue a CDR uniqueid in cdr_queue_accountcode. PHP: mark_cdr()."""
    db.execute(
        "insert into cdr_queue_accountcode (id, uniqueid) values (1, %s)", (uniqueid,)
    )


def update_rating_id_cdrs(db: Database, cdr_id: int, leg: str, rating_id: int) -> None:
    """Set a CDR's leg column to a rating id. PHP: update_rating_id_cdrs()."""
    from psycopg import sql

    query = sql.SQL("update cdrs set {leg} = %s where id = %s").format(leg=_leg_column(leg))
    db.execute(query, (rating_id, cdr_id))


def _insert_cdrs(db: Database, cdrs: Sequence[dict[str, Any]], with_server: bool) -> int:
    """Insert CDR dicts that are not already present (by call_uid); return inserted count.

    Shared body for insert_cdrs_into / insert_cdrs_into_2 (PHP). Each dict uses the
    keys the PHP read: call_uid, calling_number, called_number, dst_context, billsec,
    accountcode, ts, server_name, userfield (-> src_context), src, dst [, cdr_server_id].
    """
    count = 0
    cols = ("call_uid", "calling_number", "called_number", "dst_context", "billsec",
            "account_code", "ts", "server_name", "src_context", "src", "dst")
    for cdr in cdrs:
        if not cdr.get("call_uid"):
            continue
        if db.fetch_val("select id from cdrs where call_uid = %s", (cdr["call_uid"],)):
            continue
        values = [
            cdr.get("call_uid"), cdr.get("calling_number"), cdr.get("called_number"),
            cdr.get("dst_context"), cdr.get("billsec"), cdr.get("accountcode"),
            cdr.get("ts"), cdr.get("server_name"), cdr.get("userfield"),
            cdr.get("src"), cdr.get("dst"),
        ]
        col_list = list(cols)
        if with_server:
            col_list.append("cdr_server_id")
            values.append(cdr.get("cdr_server_id"))
        placeholders = ", ".join(["%s"] * len(values))
        db.execute(
            f"insert into cdrs ({', '.join(col_list)}) values ({placeholders})",
            tuple(values),
        )
        count += 1
    return count


def insert_cdrs_into(db: Database, cdrs: Sequence[dict[str, Any]]) -> int:
    """Batch-insert CDRs (without cdr_server_id). PHP: insert_cdrs_into()."""
    return _insert_cdrs(db, cdrs, with_server=False)


def insert_cdrs_into_2(db: Database, cdrs: Sequence[dict[str, Any]]) -> int:
    """Batch-insert CDRs (with cdr_server_id). PHP: insert_cdrs_into_2()."""
    return _insert_cdrs(db, cdrs, with_server=True)


# ===========================================================================
# Timestamp parsing & time-condition matching (rating logic)
# ===========================================================================

def get_ts(timestamp: str, dow: int) -> dict[str, Any]:
    """Split a 'YYYY-MM-DD HH:MM:SS' timestamp into parts. PHP: get_ts()
    (FIX-3: the PHP split an undefined ``$date``; here it uses the date portion)."""
    date_part, _, time_part = timestamp.partition(" ")
    hour, _, minute = time_part.partition(":")
    minute = minute.split(":")[0] if minute else ""
    year, month, day = (date_part.split("-") + ["", "", ""])[:3]
    d = dow if dow else 0
    if d == 0:
        d = 7
    return {"date": date_part, "hour": hour, "min": minute,
            "year": year, "month": month, "day": day, "dow": d}


def _to_int(value) -> int | None:
    """Helper: numeric coercion for time-condition comparisons ('' / None -> None)."""
    if value is None or value == "":
        return None
    try:
        return int(value)
    except (ValueError, TypeError):
        return None


def compare_tc_ts(tc: Sequence[dict[str, Any]], ts_parts: dict[str, Any], days: dict[str, int]):
    """Match a timestamp against a tariff's time conditions. PHP: compare_tc_ts().

    Returns (time_condition_id, matched). `days` maps weekday names to ids — the PHP
    read a global ``$DAYS`` array not defined in any present file (OPEN-1), so the
    caller must supply it. Debug echo statements from the PHP are omitted.
    """
    if not tc or not tc[0].get("id"):
        return 0, True  # PHP: no conditions -> time_condition_id 0, matched

    dow = _to_int(ts_parts.get("dow"))
    hour = _to_int(ts_parts.get("hour"))

    for entry in tc:
        if not entry.get("id"):
            continue
        tc_buf = str(entry.get("hours") or "").split("-")
        h1 = tc_buf[0].split(":")[0] if len(tc_buf) > 0 else ""
        h2 = tc_buf[1].split(":")[0] if len(tc_buf) > 1 else ""
        tc_hour_1, tc_hour_2 = _to_int(h1), _to_int(h2)

        dweek = str(entry.get("days_week") or "").split("-")
        day1_id = days.get(dweek[0]) if len(dweek) > 0 else None
        day2_id = days.get(dweek[1]) if len(dweek) > 1 else None

        if day1_id is not None and day2_id is not None and dow is not None \
                and day1_id <= dow <= day2_id:
            # II. no hours specified -> match the whole day
            if tc_hour_1 is None and tc_hour_2 is None:
                return entry["id"], True
            # I. normal window time1 < time2: time1 <= hour < time2
            if tc_hour_1 is not None and tc_hour_2 is not None and tc_hour_1 < tc_hour_2:
                if hour is not None and tc_hour_1 <= hour < tc_hour_2:
                    return entry["id"], True
            # III. overnight window time1 > time2: [0..time2] or [time1..24)
            if tc_hour_1 is not None and tc_hour_2 is not None and tc_hour_1 > tc_hour_2 \
                    and hour is not None:
                if hour <= tc_hour_2 and 0 <= hour and tc_hour_1 >= hour:
                    return entry["id"], True
                if hour >= tc_hour_2 and hour < 24 and tc_hour_1 <= hour:
                    return entry["id"], True

    return 0, False


def compare_prefix(prefix: str, called_number: str) -> bool:
    """True if `called_number` starts with `prefix`. PHP: compare_prefix() (echo dropped)."""
    return bool(prefix) and str(called_number).startswith(prefix)
