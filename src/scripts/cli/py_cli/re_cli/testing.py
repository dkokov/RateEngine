"""Port of lib.testing.php — bulk-create test calling-number accounts.

PHP entry: ``create_test_calling_number_racc($num_acc,$n,$bplan,$sm_bplan)`` built
N accounts named ``35910NNNNNN`` and called ``re5_create_account()`` for each. Since
re5 is dropped (it duplicated re7), the get-or-create flow is reimplemented directly
on the re7 inserters in :mod:`re_cli.re7.db`, each account wrapped in a transaction so
a partial failure rolls back atomically (MIGRATION.md §4.8).

The PHP ``define()`` constants become the defaults below / function arguments.
``sm_bill_plan`` (a secondary plan, e.g. SMS) is not supported by the re7
insert_rating_account and is therefore ignored — noted as a limitation.
"""

from __future__ import annotations

import sys

from .database import Database
from .errors import CliError
from .re7 import db as q

# Defaults mirroring the PHP define()s
PREFIX = "35910"
CDR_SERVER_ID = 1
CURR_ID = 1
LEG = "a"
RATING_MODE = "calling_number"
PCARD_TYPE = "credit"
PCARD_STATUS = "active"


def create_test_calling_number_accounts(
    db: Database,
    count: int,
    start: int,
    bill_plan: str,
    amount=20,
    prefix: str = PREFIX,
    sm_bill_plan: str | None = None,
) -> list[str]:
    """Create `count` calling-number accounts starting at index `start`.

    Each account: a billing account ``bacc_<number>``, a ``calling_number`` rating
    account, and a prepaid card — all get-or-create, mirroring the PHP. Returns the
    list of calling numbers created.
    """
    bill_plan_id = q.get_bill_plan_id(db, bill_plan)
    if not bill_plan_id:
        raise CliError(f"bill plan not found: {bill_plan!r}")

    if sm_bill_plan:
        print(f"warning: sm_bill_plan ({sm_bill_plan!r}) is not supported by the re7 "
              f"rating-account insert and will be ignored", file=sys.stderr)

    # Resolve pcard type/status once (PHP looked them up per account).
    type_id = q.get_pcard_type_id(db, PCARD_TYPE) or 2
    status_id = q.get_pcard_status_id(db, PCARD_STATUS) or 0

    created: list[str] = []
    for i in range(start, start + count):
        number = f"{prefix}{i:06d}"          # PHP: PREFIX + zero-padded(i) to 6 digits
        bacc_username = f"bacc_{number}"

        with db.transaction():
            bacc_id = (
                q.get_billing_account_id(db, bacc_username)
                or q.insert_billing_account(db, bacc_username, CURR_ID, LEG, CDR_SERVER_ID)
            )
            if not q.get_rating_account_id(db, RATING_MODE, number):
                q.insert_rating_account(db, RATING_MODE, number, bacc_id, bill_plan_id)
            if not q.get_pcard_id(db, bacc_id, type_id, status_id, amount, None, None):
                q.insert_pcard(db, bacc_id, type_id, status_id, amount, None, None)

        created.append(number)
        print(number)

    return created
