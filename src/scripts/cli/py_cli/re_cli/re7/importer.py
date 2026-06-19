"""Port of lib.importing.php — import tariff/account settings from a CSV file.

PHP entry: ``re6_import_settings($file)``. Each line's first field is a
``config_mode`` tag that selects what the rest of the row defines:

    1  bill plan
    2  prefix + tariff + rate
    *  calc function (for the current tariff)
    3  billing account + rating account
    4  prepaid card (for the current billing account)
    5  bill plan tree (root + children)

State carried across rows (current bill plan / tariff / billing account) lives in
:class:`ImportState` instead of the PHP loop-local variables. The PHP
"insert-then-goto-reselect" loops become get-or-create using the RETURNING-based
inserters in :mod:`re_cli.re7.db` (MIGRATION.md §3.3, §4.5).

Deviations from the PHP, all faithful to intent (see MIGRATION.md §6):
  * FIX-4 — the mode-3 branch chooses insert_rating_account vs _2 by the rating
    *mode id* (<=4 vs >=5), not by the mode *name* string as the PHP did (a string
    compared to an int — broken under PHP 8).
  * The commented-out free_billsec / time_condition blocks in mode 2 stay omitted.
"""

from __future__ import annotations

import csv
import json
from dataclasses import dataclass
from decimal import Decimal, InvalidOperation
from pathlib import Path
from typing import Any, Sequence

from ..database import Database
from ..errors import CliError
from . import db as q

FORMATS = ("auto", "csv", "json", "yaml")


def _strip_quotes(value: str) -> str:
    """Mimic the PHP trim($x, '"') applied to quoted fields."""
    return value.strip().strip('"')


def _int(value: str, default: int = 0) -> int:
    """Parse an integer field, falling back to `default` for empty/blank values."""
    value = value.strip()
    if not value:
        return default
    try:
        return int(value)
    except ValueError:
        return default


def _num(value: str):
    """Parse a numeric (money/rate) field as Decimal; None when empty."""
    value = value.strip()
    if not value:
        return None
    try:
        return Decimal(value)
    except InvalidOperation as exc:
        raise CliError(f"invalid numeric value: {value!r}") from exc


def _get(row: Sequence[str], idx: int, default: str = "") -> str:
    """Safe row access (PHP silently yields '' for missing indices)."""
    return row[idx] if idx < len(row) else default


@dataclass
class ImportState:
    """Cross-row state, mirroring the PHP loop-local variables."""

    bill_plan_id: int | None = None
    tariff_id: int | None = None
    prefix_id: int | None = None
    billing_account_id: int | None = None


def import_settings(db: Database, path: str) -> ImportState:
    """Import every row of `path` into the re7 database. Returns the final state.

    Mirrors re6_import_settings: malformed/comment lines whose first field is not a
    known mode are skipped silently.
    """
    state = ImportState()
    with open(path, newline="", encoding="utf-8") as fh:
        reader = csv.reader(fh)
        for row in reader:
            if not row:
                continue
            mode = row[0].strip()
            if mode == "1":
                _mode_bill_plan(db, state, row)
            elif mode == "2":
                _mode_prefix_tariff_rate(db, state, row)
            elif mode == "*":
                _mode_calc_function(db, state, row)
            elif mode == "3":
                _mode_accounts(db, state, row)
            elif mode == "4":
                _mode_pcard(db, state, row)
            elif mode == "5":
                _mode_bill_tree(db, row)
            # anything else (comments, headers) is ignored, as in the PHP
    return state


def _mode_bill_plan(db: Database, state: ImportState, row: Sequence[str]) -> None:
    """Mode 1: get-or-create a bill plan; set it as current."""
    bill_plan = _strip_quotes(_get(row, 1))
    bill_plan_type = _strip_quotes(_get(row, 2)) or "postpaid"  # postpaid=2, prepaid=1
    start_period = _int(_get(row, 3))
    end_period = _int(_get(row, 4))

    type_id = q.get_bill_plan_type_id(db, bill_plan_type)
    if not type_id:
        raise CliError(f"unknown bill_plan_type: {bill_plan_type!r}")

    bill_plan_id = q.get_bill_plan_id(db, bill_plan)
    if not bill_plan_id:
        bill_plan_id = q.insert_bill_plan(db, bill_plan, type_id, start_period, end_period)
    state.bill_plan_id = bill_plan_id


def _mode_prefix_tariff_rate(db: Database, state: ImportState, row: Sequence[str]) -> None:
    """Mode 2: get-or-create prefix, tariff and rate under the current bill plan."""
    if not state.bill_plan_id:
        raise CliError("mode 2 (prefix/tariff/rate) seen before any bill plan (mode 1)")

    prefix = _get(row, 1).strip()
    prefix_desc = _strip_quotes(_get(row, 2))
    tariff = _strip_quotes(_get(row, 3))
    free_billsec_id = _int(_get(row, 4))
    start_period_tr = _int(_get(row, 6))
    end_period_tr = 0  # PHP referenced an undefined $end_period_tr -> always 0

    prefix_id = q.get_prefix_id(db, prefix)
    if not prefix_id:
        prefix_id = q.insert_prefix(db, prefix, prefix_desc)
    state.prefix_id = prefix_id

    tariff_id = q.get_tariff_id(db, state.bill_plan_id, tariff)
    if not tariff_id:
        tariff_id = q.insert_tariff(
            db, state.bill_plan_id, tariff, start_period_tr, end_period_tr, free_billsec_id
        )
    state.tariff_id = tariff_id

    rate_id = q.get_rate_id(db, state.bill_plan_id, prefix_id, tariff_id)
    if not rate_id and state.bill_plan_id and prefix_id and tariff_id:
        rate_id = q.insert_rate(db, state.bill_plan_id, prefix_id, tariff_id)

    print(f"bplan({state.bill_plan_id}),prefix({prefix_id}),tariff({tariff_id}),rate({rate_id})")


def _mode_calc_function(db: Database, state: ImportState, row: Sequence[str]) -> None:
    """Mode *: add a calc function to the current tariff (skip if it already exists)."""
    if not state.tariff_id:
        return
    pos = _int(_get(row, 1))
    fee = _num(_get(row, 2))
    delta_time = _int(_get(row, 3))
    iterations = _get(row, 4).strip()

    if q.get_calc_id(db, state.tariff_id, pos):
        return
    q.insert_calc_func(db, state.tariff_id, pos, fee, delta_time, _int(iterations) if iterations else None)


def _mode_accounts(db: Database, state: ImportState, row: Sequence[str]) -> None:
    """Mode 3: get-or-create billing account + rating account."""
    billing_account = _strip_quotes(_get(row, 1))
    leg = _get(row, 2).strip()
    curr_id = _int(_get(row, 3))
    rating_mode_id = _int(_get(row, 4))
    rating_account = _strip_quotes(_get(row, 5))
    cdr_server_id = _int(_get(row, 6))
    clg_nadi = _int(_get(row, 7)) if _get(row, 7).strip() else None
    cld_nadi = _int(_get(row, 8)) if _get(row, 8).strip() else None

    if rating_mode_id == 0:
        return
    rating_mode = q.get_rating_mode(db, rating_mode_id)
    if not rating_mode:
        return

    billing_account_id = q.get_billing_account_id(db, billing_account)
    if not billing_account_id:
        billing_account_id = q.insert_billing_account(db, billing_account, curr_id, leg, cdr_server_id)
    state.billing_account_id = billing_account_id

    rating_account_id = q.get_rating_account_id(db, rating_mode, rating_account)
    if not rating_account_id:
        # FIX-4: branch on the mode id, not the mode name (see module docstring).
        if rating_mode_id <= 4:
            q.insert_rating_account(db, rating_mode, rating_account, billing_account_id, state.bill_plan_id)
        else:
            q.insert_rating_account_2(
                db, rating_mode, rating_account, billing_account_id, state.bill_plan_id, clg_nadi, cld_nadi
            )


def _mode_pcard(db: Database, state: ImportState, row: Sequence[str]) -> None:
    """Mode 4: add a prepaid card to the current billing account (if not present)."""
    if not state.billing_account_id:
        return
    pcard_type = _strip_quotes(_get(row, 1))
    pcard_status = _strip_quotes(_get(row, 2))
    amount = _num(_get(row, 3))
    start_date = _strip_quotes(_get(row, 4)) or None
    end_date = _strip_quotes(_get(row, 5)) or None

    status_id = q.get_pcard_status_id(db, pcard_status) if pcard_status else 0
    type_id = q.get_pcard_type_id(db, pcard_type) if pcard_type else 2

    if not type_id:
        return
    if q.get_pcard_id(db, state.billing_account_id, type_id, status_id, amount, start_date, end_date):
        return
    q.insert_pcard(db, state.billing_account_id, type_id, status_id, amount, start_date, end_date)


def _mode_bill_tree(db: Database, row: Sequence[str]) -> None:
    """Mode 5: build a bill plan tree — first field is the root, the rest are children."""
    root_id = 0
    p = 1
    while _get(row, p).strip():
        name = _strip_quotes(_get(row, p))
        tree_id = q.get_bill_plan_id(db, name)
        if p == 1:
            root_id = tree_id if tree_id else q.insert_bill_plan_v2(db, name)
            p += 1
            continue
        if tree_id and root_id and not q.get_bill_tree_id(db, tree_id, root_id):
            q.insert_bill_tree(db, tree_id, root_id)
        p += 1


# ===========================================================================
# Format dispatch + structured (JSON/YAML) import
# ===========================================================================

def resolve_format(path: str, fmt: str) -> str:
    """Resolve 'auto' to a concrete format from the file extension (else csv)."""
    if fmt != "auto":
        return fmt
    ext = Path(path).suffix.lower()
    if ext == ".json":
        return "json"
    if ext in (".yaml", ".yml"):
        return "yaml"
    return "csv"


def _load_structured(path: str, fmt: str) -> dict[str, Any]:
    """Parse a JSON or YAML bill-plan model file (the shape produced by `dump`)."""
    text = Path(path).read_text(encoding="utf-8")
    if fmt == "json":
        return json.loads(text)
    if fmt == "yaml":
        try:
            import yaml
        except ImportError as exc:  # YAML has no built-in parser fallback
            raise CliError("YAML import requires PyYAML: pip install pyyaml") from exc
        return yaml.safe_load(text)
    raise CliError(f"cannot parse format {fmt!r} as structured data")


def import_file(db: Database, path: str, fmt: str = "auto") -> None:
    """Import `path` in the given format (auto/csv/json/yaml).

    CSV uses the full config_mode state machine (:func:`import_settings`); JSON/YAML
    use the structured bill-plan model (:func:`import_model`) — the same shape `dump`
    emits, enabling dump -> edit -> import round-trips.
    """
    fmt = resolve_format(path, fmt)
    if fmt == "csv":
        import_settings(db, path)
    else:
        import_model(db, _load_structured(path, fmt))


def _fee(value: Any):
    """Coerce a JSON/YAML fee (float/int/str) to Decimal for exact storage; None stays None."""
    if value is None:
        return None
    if isinstance(value, Decimal):
        return value
    return Decimal(str(value))


def import_model(db: Database, model: dict[str, Any]) -> None:
    """Import a structured bill-plan model (the shape produced by `dump --format json/yaml`).

    Scope mirrors `dump`: bill plan(s) -> prefixes -> tariffs (+free_billsec id) -> rates
    -> calc functions. Everything is get-or-create and keyed by *name/prefix string*, so a
    model is portable to a fresh DB (the numeric ids in a dump are informational). Bill
    plans are created as 'postpaid' with zero periods (the dump model carries neither).
    Account/pcard/tree provisioning is CSV-only (config modes 3/4/5).
    """
    if not isinstance(model, dict):
        raise CliError("structured import expects a mapping with a 'plans' list")
    plans = model.get("plans")
    if not plans:
        raise CliError("no 'plans' found in the input model")

    for plan in plans:
        name = plan.get("name") or model.get("bill_plan")
        if not name:
            raise CliError("a plan entry is missing its 'name'")

        bill_plan_id = q.get_bill_plan_id(db, name)
        if not bill_plan_id:
            type_id = q.get_bill_plan_type_id(db, "postpaid")
            if not type_id:
                raise CliError("unknown bill_plan_type: 'postpaid'")
            bill_plan_id = q.insert_bill_plan(db, name, type_id, 0, 0)

        for rate in plan.get("rates") or []:
            prefix = str(rate["prefix"])
            tariff = rate["tariff"]
            free = rate.get("free_billsec")
            free_id = free["id"] if free else 0

            prefix_id = q.get_prefix_id(db, prefix) or q.insert_prefix(db, prefix, None)

            tariff_id = q.get_tariff_id(db, bill_plan_id, tariff)
            if not tariff_id:
                tariff_id = q.insert_tariff(db, bill_plan_id, tariff, 0, 0, free_id)

            if not q.get_rate_id(db, bill_plan_id, prefix_id, tariff_id):
                q.insert_rate(db, bill_plan_id, prefix_id, tariff_id)

            for calc in rate.get("calc_functions") or []:
                pos = calc["pos"]
                if q.get_calc_id(db, tariff_id, pos):
                    continue
                q.insert_calc_func(
                    db, tariff_id, pos, _fee(calc.get("fee")),
                    calc.get("delta_time"), calc.get("iterations"),
                )

            print(f"bplan({bill_plan_id}),prefix({prefix_id}),tariff({tariff_id})")
