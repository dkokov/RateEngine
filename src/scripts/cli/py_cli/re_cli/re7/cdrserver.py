"""Port of lib.cdrserver.php — insert a FreeSWITCH CDR into the `fs_cdrs` database.

PHP entry: ``insert_cdr($arrXml)`` takes the nested array produced by parsing a
FreeSWITCH CDR XML document and writes one row (~53 columns) into `cdrs`. The PHP
file also had a trailing block that fabricated a random CDR for testing; that is
ported as :func:`make_test_cdr`.

Differences from the PHP (see MIGRATION.md §4.7):
  * The insert is parameterized instead of string-concatenated.
  * The DB connection is passed in (a `cdr` :class:`~re_cli.database.Database`)
    rather than opened ad hoc with hard-coded credentials.
  * date()/mktime()/mt_rand() -> datetime / time.time() / random.
  * The commented-out MyCC `myCC_term()` call is omitted (OPEN-2).
"""

from __future__ import annotations

import random
import time
from datetime import datetime
from typing import Any

from ..database import Database

# Each entry maps a target column to its path in the nested CDR dict and a default
# applied when the value is missing/empty (mirroring the PHP `if(empty(...))` block).
# Paths use the underscore-normalized keys the PHP used (it str_replace(":","_")'d them).
_V = ("variables",)
_CP = ("callflow", "caller_profile")
_IN = ("call-stats", "audio", "inbound")

# (column, path, default)
COLUMNS: list[tuple[str, tuple[str, ...], Any]] = [
    ("call_uid", (*_V, "uuid"), None),
    ("src_context", (*_CP, "context"), None),
    ("incoming_channel", (*_CP, "chan_name"), None),
    ("billsec", (*_V, "billsec"), None),
    ("duration", (*_V, "duration"), None),
    ("src", (*_V, "ANI"), None),
    ("dst", (*_V, "DNIS"), None),
    # ts handled specially (epoch -> timestamp) — see _build_row
    ("clg_nadi", (*_V, "sip_h_X-freetdm-clg-nadi"), 0),
    ("cld_nadi", (*_V, "sip_h_X-freetdm-cld-nadi"), 0),
    ("outgoing_channel", (*_CP, "chan_name"), None),
    ("screen", (*_V, "sip_h_X-freetdm-screen"), 0),
    ("presentation", (*_V, "sip_h_X-freetdm-presentation"), 0),
    ("hangup_cause", (*_V, "hangup_cause"), None),
    ("originate_disposition", (*_V, "originate_disposition"), None),
    ("hangup_cause_q850", (*_V, "hangup_cause_q850"), None),
    ("dst_tgroup", (*_V, "dst_tgroup"), None),
    ("src_tgroup", (*_V, "src_tgroup"), None),
    ("incoming_codec", (*_V, "read_codec"), None),
    ("outgoing_codec", (*_V, "write_codec"), None),
    ("uduration", (*_V, "uduration"), None),
    ("billusec", (*_V, "billusec"), None),
    ("rdnis", (*_V, "sip_h_X-freetdm-rdnis"), ""),
    ("rdnis_nadi", (*_V, "sip_h_X-freetdm-rdnis-nadi"), 0),
    ("rdnis_screen", (*_V, "sip_h_X-freetdm-rdnis-screen"), 0),
    ("rdnis_presentation", (*_V, "sip_h_X-freetdm-rdnis-presentation"), 0),
    ("local_network_addr", (*_V, "sip_local_network_addr"), ""),
    ("contact_uri", (*_V, "sip_contact_uri"), ""),
    ("to_uri", (*_V, "sip_to_uri"), ""),
    ("full_via", (*_V, "sip_full_via"), ""),
    ("remote_media_ip", (*_V, "remote_media_ip"), ""),
    ("remote_media_port", (*_V, "remote_media_port"), 0),
    ("maxsec", (*_V, "maxsec"), -1),
    ("call_usage", (*_V, "limit_usage"), -1),
    ("account_code", (*_V, "account_code"), None),
    ("packet_count", (*_IN, "packet_count"), -1),
    ("media_packet_count", (*_IN, "media_packet_count"), -1),
    ("skip_packet_count", (*_IN, "skip_packet_count"), -1),
    ("jitter_packet_count", (*_IN, "jitter_packet_count"), -1),
    ("dtmf_packet_count", (*_IN, "dtmf_packet_count"), -1),
    ("cng_packet_count", (*_IN, "cng_packet_count"), -1),
    ("flush_packet_count", (*_IN, "flush_packet_count"), -1),
    ("largest_jb_size", (*_IN, "largest_jb_size"), -1),
    ("jitter_min", (*_IN, "jitter_min_variance"), -1),
    ("jitter_max", (*_IN, "jitter_max_variance"), -1),
    ("jitter_loss_rate", (*_IN, "jitter_loss_rate"), -1),
    ("jitter_burst_rate", (*_IN, "jitter_burst_rate"), -1),
    ("mean_interval", (*_IN, "mean_interval"), -1),
    ("flaw_total", (*_IN, "flaw_total"), -1),
    ("mos", (*_IN, "mos"), -1),
    ("quality_percentage", (*_IN, "quality_percentage"), -1),
    ("error_period_msec", ("call-stats", "audio", "error-log", "error-period", "duration-msec"), -1),
    ("gps", (*_V, "sip_h_X-UserCoordinate"), None),
]


def _dig(data: dict, path: tuple[str, ...], default: Any) -> Any:
    """Walk nested dicts by `path`; return `default` if any key is missing or value is empty."""
    cur: Any = data
    for key in path:
        if not isinstance(cur, dict) or key not in cur:
            return default
        cur = cur[key]
    # PHP empty(): None, '', 0, '0', [] all fall back to the default
    if cur is None or cur == "" or cur == 0 or cur == "0" or cur == []:
        return default
    return cur


def _build_row(data: dict) -> tuple[list[str], list[Any]]:
    """Produce (columns, values) for one CDR insert from the nested dict."""
    columns: list[str] = []
    values: list[Any] = []

    for col, path, default in COLUMNS:
        # Insert the special ts column right after dst, preserving the PHP column order.
        if col == "clg_nadi" and "ts" not in columns:
            epoch = _dig(data, (*_V, "start_epoch"), None)
            columns.append("ts")
            values.append(
                datetime.fromtimestamp(int(epoch)).strftime("%Y-%m-%d %H:%M:%S")
                if epoch is not None else None
            )
        columns.append(col)
        values.append(_dig(data, path, default))

    return columns, values


def insert_cdr(db: Database, data: dict) -> None:
    """Insert one CDR (nested dict) into `cdrs`. PHP: insert_cdr()."""
    columns, values = _build_row(data)
    placeholders = ", ".join(["%s"] * len(values))
    db.execute(
        f"insert into cdrs ({', '.join(columns)}) values ({placeholders})",
        tuple(values),
    )


def make_test_cdr() -> dict:
    """Fabricate a random CDR dict for testing. PHP: the trailing block of lib.cdrserver.php."""
    uuid = f"{random.randint(1000, 9999)}-test-{random.randint(100000, 999999)}-dddd-{random.randint(1000, 9999)}"
    dur = random.randint(5_000_000, 3_600_000_000)
    bill = dur - 5_000_000
    return {
        "variables": {
            "uuid": uuid,
            "billsec": round(bill / 1_000_000),
            "duration": round(dur / 1_000_000),
            "ANI": "35910000000",
            "DNIS": "35910123456",
            "start_epoch": int(time.time()),
            "hangup_cause": "NORMAL CLEARING",
            "hangup_cause_q850": 16,
            "uduration": dur,
            "billusec": bill,
            "dst_tgroup": None,
            "src_tgroup": None,
            "read_codec": None,
            "write_codec": None,
            "account_code": None,
            "originate_disposition": None,
            "sip_h_X-UserCoordinate": None,
        }
    }


def insert_test_cdrs(db: Database, count: int = 1) -> int:
    """Insert `count` random test CDRs; return how many were written."""
    for _ in range(count):
        insert_cdr(db, make_test_cdr())
    return count
