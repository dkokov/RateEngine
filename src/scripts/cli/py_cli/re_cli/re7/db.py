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
