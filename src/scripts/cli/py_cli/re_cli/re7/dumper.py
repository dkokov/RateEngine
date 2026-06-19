"""Port of lib.dumping.php — dump a bill plan's tariff settings.

PHP entry: ``re6_dump_bplan_settings($bplan)``. The output format is reproduced
faithfully (header row, leading-space empty fields, the irregular newline layout
the PHP ``echo`` statements produce) so a dump stays diff-comparable with the
legacy tool (MIGRATION.md §4.6).
"""

from __future__ import annotations

import sys
from typing import TextIO

from ..database import Database
from . import db as q

HEADER = (
    "bill_plan,prefix_id,prefix,tariff_id,tariff,"
    "free_billsec_id,free_billsec,pos,delta_time,fee,iterations"
)


def _s(value: object) -> str:
    """Render a DB value the way the PHP echo did: None (SQL NULL) -> empty string."""
    return "" if value is None else str(value)


def dump_bplan(db: Database, bplan: str, out: TextIO | None = None) -> bool:
    """Dump bill plan ``bplan`` (and its tree, if it is a root) to ``out``.

    Returns True if the plan was found and dumped, False if the name is unknown
    (the PHP did nothing silently in that case).
    """
    if out is None:
        out = sys.stdout

    bid = q.get_bill_plan_id(db, bplan)
    if not bid:
        return False

    # A tree root expands to its child plans; otherwise dump the single plan.
    bplans = q.get_bill_plans_tree(db, bid)
    if not bplans:
        bplans = q.get_bill_plans_v2(db, bid)

    out.write("\n" + HEADER + "\n")

    for bp in bplans:
        if not bp["id"]:
            continue
        out.write("\n" + _s(bp["bplan"]) + ",")

        for rate in q.get_rate_data(db, bp["id"]):
            out.write(
                "\n ,"
                + _s(rate["prefix_id"]) + "," + _s(rate["prefix"]) + ","
                + _s(rate["tariff_id"]) + "," + _s(rate["tariff"])
            )

            free = q.get_free_billsec_v2(db, rate["tariff_id"])
            if free:
                out.write("," + _s(free["free_billsec_id"]) + "," + _s(free["free_billsec"]))

            for calc in q.get_calc_functions_v2(db, rate["tariff_id"]):
                out.write(
                    "\n , , , , , , ,"
                    + _s(calc["pos"]) + "," + _s(calc["delta_time"]) + ","
                    + _s(calc["fee"]) + "," + _s(calc["iterations"])
                )

            out.write("\n")

        out.write("\n")

    return True
