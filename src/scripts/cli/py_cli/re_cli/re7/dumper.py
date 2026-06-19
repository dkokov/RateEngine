"""Port of lib.dumping.php — dump a bill plan's tariff settings.

PHP entry: ``re6_dump_bplan_settings($bplan)``. The data is collected once into a
structured model (:func:`collect_bplan`) and then rendered in one of three formats:

  * ``csv``  — the legacy layout, reproduced faithfully (header row, leading-space
    empty fields, the irregular newlines the PHP ``echo`` produced) so it stays
    diff-comparable with the old tool (MIGRATION.md §4.6). This is the default.
  * ``json`` — nested JSON (stdlib).
  * ``yaml`` — YAML via PyYAML if installed, else a small built-in emitter.
"""

from __future__ import annotations

import json
import sys
from decimal import Decimal
from typing import Any, TextIO

from ..database import Database
from ..errors import CliError
from . import db as q

HEADER = (
    "bill_plan,prefix_id,prefix,tariff_id,tariff,"
    "free_billsec_id,free_billsec,pos,delta_time,fee,iterations"
)

FORMATS = ("csv", "json", "yaml")


# --------------------------------------------------------------------------
# Data collection
# --------------------------------------------------------------------------

def collect_bplan(db: Database, bplan: str) -> dict[str, Any] | None:
    """Collect a bill plan (and its tree, if a root) into a structured dict.

    Returns None if the bill plan name is unknown. Shape::

        {"bill_plan": <name>, "plans": [
            {"id": int, "name": str, "rates": [
                {"prefix_id", "prefix", "tariff_id", "tariff",
                 "free_billsec": {"id", "value"} | None,
                 "calc_functions": [{"pos","delta_time","fee","iterations"}, ...]}
            ]}
        ]}
    """
    bid = q.get_bill_plan_id(db, bplan)
    if not bid:
        return None

    plan_rows = q.get_bill_plans_tree(db, bid) or q.get_bill_plans_v2(db, bid)

    plans: list[dict[str, Any]] = []
    for bp in plan_rows:
        if not bp["id"]:
            continue
        rates: list[dict[str, Any]] = []
        for rate in q.get_rate_data(db, bp["id"]):
            free = q.get_free_billsec_v2(db, rate["tariff_id"])
            rates.append({
                "prefix_id": rate["prefix_id"],
                "prefix": rate["prefix"],
                "tariff_id": rate["tariff_id"],
                "tariff": rate["tariff"],
                "free_billsec": (
                    {"id": free["free_billsec_id"], "value": free["free_billsec"]}
                    if free else None
                ),
                "calc_functions": [
                    {"pos": c["pos"], "delta_time": c["delta_time"],
                     "fee": c["fee"], "iterations": c["iterations"]}
                    for c in q.get_calc_functions_v2(db, rate["tariff_id"])
                ],
            })
        plans.append({"id": bp["id"], "name": bp["bplan"], "rates": rates})

    return {"bill_plan": bplan, "plans": plans}


# --------------------------------------------------------------------------
# Renderers
# --------------------------------------------------------------------------

def _s(value: object) -> str:
    """Render a value the way the PHP echo did: None (SQL NULL) -> empty string."""
    return "" if value is None else str(value)


def render_csv(model: dict[str, Any], out: TextIO) -> None:
    """Reproduce the legacy re6_dump_bplan_settings text output byte-for-byte."""
    out.write("\n" + HEADER + "\n")
    for plan in model["plans"]:
        out.write("\n" + _s(plan["name"]) + ",")
        for rate in plan["rates"]:
            out.write(
                "\n ,"
                + _s(rate["prefix_id"]) + "," + _s(rate["prefix"]) + ","
                + _s(rate["tariff_id"]) + "," + _s(rate["tariff"])
            )
            free = rate["free_billsec"]
            if free:
                out.write("," + _s(free["id"]) + "," + _s(free["value"]))
            for calc in rate["calc_functions"]:
                out.write(
                    "\n , , , , , , ,"
                    + _s(calc["pos"]) + "," + _s(calc["delta_time"]) + ","
                    + _s(calc["fee"]) + "," + _s(calc["iterations"])
                )
            out.write("\n")
        out.write("\n")


def _plain(obj: Any) -> Any:
    """Recursively convert Decimals to floats so json/yaml can serialize the model."""
    if isinstance(obj, dict):
        return {k: _plain(v) for k, v in obj.items()}
    if isinstance(obj, list):
        return [_plain(v) for v in obj]
    if isinstance(obj, Decimal):
        return float(obj)
    return obj


def render_json(model: dict[str, Any], out: TextIO) -> None:
    """Render the model as indented JSON (UTF-8, Cyrillic preserved)."""
    json.dump(_plain(model), out, ensure_ascii=False, indent=2)
    out.write("\n")


def _yaml_scalar(value: Any) -> str:
    if value is None:
        return "null"
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (int, float)):
        return repr(value) if isinstance(value, float) else str(value)
    s = str(value).replace("\\", "\\\\").replace('"', '\\"')
    return '"' + s + '"'


def _yaml_emit(obj: Any, indent: int, out: TextIO) -> None:
    """Minimal YAML emitter (fallback when PyYAML is absent)."""
    pad = "  " * indent
    if isinstance(obj, dict):
        for key, val in obj.items():
            if isinstance(val, (dict, list)) and val:
                out.write(f"{pad}{key}:\n")
                _yaml_emit(val, indent + 1, out)
            elif isinstance(val, (dict, list)):
                out.write(f"{pad}{key}: {'{}' if isinstance(val, dict) else '[]'}\n")
            else:
                out.write(f"{pad}{key}: {_yaml_scalar(val)}\n")
    elif isinstance(obj, list):
        for item in obj:
            if isinstance(item, (dict, list)) and item:
                out.write(f"{pad}-\n")
                _yaml_emit(item, indent + 1, out)
            else:
                out.write(f"{pad}- {_yaml_scalar(item)}\n")


def render_yaml(model: dict[str, Any], out: TextIO) -> None:
    """Render the model as YAML — PyYAML if available, else the built-in emitter."""
    plain = _plain(model)
    try:
        import yaml  # optional dependency (pip install 're7-cli[yaml]')
    except ImportError:
        _yaml_emit(plain, 0, out)
        return
    yaml.safe_dump(plain, out, allow_unicode=True, sort_keys=False, default_flow_style=False)


_RENDERERS = {"csv": render_csv, "json": render_json, "yaml": render_yaml}


def dump_bplan(db: Database, bplan: str, out: TextIO | None = None, fmt: str = "csv") -> bool:
    """Dump bill plan ``bplan`` to ``out`` in ``fmt`` (csv/json/yaml).

    Returns True if the plan was found and dumped, False if the name is unknown.
    """
    if fmt not in _RENDERERS:
        raise CliError(f"unknown format: {fmt!r} (choose from {', '.join(FORMATS)})")
    model = collect_bplan(db, bplan)
    if model is None:
        return False
    _RENDERERS[fmt](model, out or sys.stdout)
    return True
