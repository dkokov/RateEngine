"""Ported data-access functions from lib.db.php (re7 schema).

Each PHP ``global $dbconn`` function becomes a module function taking a
:class:`~re_cli.database.Database` as its first argument. All queries are
parameterized (MIGRATION.md §3.1); getters return ``None`` / ``[]`` for
"not found" instead of the PHP ``0`` / ``""`` sentinels (§3.6).

Phase 2 ports the full ~120-function set. This first slice covers exactly the
functions the dump path (§4.6) depends on; the rest follow.
"""

from __future__ import annotations

from typing import Any

from ..database import Database


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
