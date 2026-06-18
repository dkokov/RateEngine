"""Command-line entry point — replaces RE6Commander's ``switch($opt)``.

Phase 1 wires up argparse, configuration, and lazy DB connections; the actual
import/dump/test/perf handlers are stubs filled in by later phases (MIGRATION.md §5).
The original short flags (-f/-d/-t/-p) map to named subcommands.
"""

from __future__ import annotations

import argparse
import sys

from .config import Config
from .database import Database
from .errors import CliError


def _open_re7(cfg: Config) -> Database:
    return Database.connect(cfg.re7, label="re7")


def cmd_import(args: argparse.Namespace, cfg: Config) -> int:
    """`-f file` — import tariff/account settings from CSV (lib.importing.php)."""
    with _open_re7(cfg) as db:
        # Phase 3: from .re7.importer import import_settings; import_settings(db, args.file)
        raise CliError(f"import not implemented yet (Phase 3); would load {args.file!r} into {db.label}")


def cmd_dump(args: argparse.Namespace, cfg: Config) -> int:
    """`-d bplan` — dump a bill plan to CSV (lib.dumping.php)."""
    with _open_re7(cfg) as db:
        # Phase 3: from .re7.dumper import dump_bplan; dump_bplan(db, args.bplan, sys.stdout)
        raise CliError(f"dump not implemented yet (Phase 3); would dump {args.bplan!r} from {db.label}")


def cmd_test(args: argparse.Namespace, cfg: Config) -> int:
    """`-t` — testing helpers (lib.testing.php). PHP stub was empty."""
    raise CliError("test not implemented yet (Phase 5)")


def cmd_perf(args: argparse.Namespace, cfg: Config) -> int:
    """`-p` — rating performance testing. PHP stub was empty."""
    raise CliError("perf not implemented yet (TODO)")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="re7commander",
        description="RateEngine V7 admin/provisioning CLI (port of RE6Commander).",
    )
    parser.add_argument("--env", metavar="PATH", help="path to .env file (default: py_cli/.env)")
    sub = parser.add_subparsers(dest="command", required=True)

    p_import = sub.add_parser("import", aliases=["-f"], help="import tariff settings from CSV file")
    p_import.add_argument("file", help="input CSV file")
    p_import.set_defaults(func=cmd_import)

    p_dump = sub.add_parser("dump", aliases=["-d"], help="dump a bill plan to CSV")
    p_dump.add_argument("bplan", help="bill plan name")
    p_dump.set_defaults(func=cmd_dump)

    p_test = sub.add_parser("test", aliases=["-t"], help="rating functions testing")
    p_test.set_defaults(func=cmd_test)

    p_perf = sub.add_parser("perf", aliases=["-p"], help="rating performance testing")
    p_perf.set_defaults(func=cmd_perf)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        cfg = Config.load(args.env)
        return args.func(args, cfg) or 0
    except CliError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
