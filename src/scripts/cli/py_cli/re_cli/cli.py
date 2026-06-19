"""Command-line entry point — replaces RE6Commander's ``switch($opt)``.

Subcommands: import (-f), dump (-d), test (-t, create test accounts), gen-cdrs
(insert random CDRs), perf (-p, not implemented). The original short flags map to
the named subcommands. Handlers open their DB connection lazily and import the
relevant re7 module on demand so config/--help work without the psycopg driver.
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
    from .re7.importer import import_settings

    try:
        with _open_re7(cfg) as db:
            import_settings(db, args.file)
    except OSError as exc:
        raise CliError(f"cannot read {args.file!r}: {exc}") from exc
    return 0


def cmd_dump(args: argparse.Namespace, cfg: Config) -> int:
    """`-d bplan` — dump a bill plan (lib.dumping.php / re6_dump_bplan_settings)."""
    from .re7.dumper import dump_bplan

    with _open_re7(cfg) as db:
        if not dump_bplan(db, args.bplan, sys.stdout, fmt=args.format):
            raise CliError(f"bill plan not found: {args.bplan!r}")
    return 0


def cmd_test(args: argparse.Namespace, cfg: Config) -> int:
    """`-t` — bulk-create test calling-number accounts (lib.testing.php)."""
    from .testing import create_test_calling_number_accounts

    with _open_re7(cfg) as db:
        accounts = create_test_calling_number_accounts(
            db, args.count, args.start, args.bill_plan,
            amount=args.amount, prefix=args.prefix, sm_bill_plan=args.sm_bill_plan,
        )
    print(f"created {len(accounts)} account(s)", file=sys.stderr)
    return 0


def cmd_gen_cdrs(args: argparse.Namespace, cfg: Config) -> int:
    """Insert random test CDRs into the fs_cdrs database (lib.cdrserver.php test block)."""
    from .re7.cdrserver import insert_test_cdrs

    with Database.connect(cfg.cdr, label="cdr") as db:
        n = insert_test_cdrs(db, args.count)
    print(f"inserted {n} test CDR(s)", file=sys.stderr)
    return 0


def cmd_perf(args: argparse.Namespace, cfg: Config) -> int:
    """`-p` — rating performance testing. PHP stub was empty."""
    raise CliError("perf not implemented yet (TODO)")


# Legacy RE6Commander flags -> sub-command names. argparse can't use dash-prefixed
# names as positional sub-commands, so we translate them before parsing (below).
LEGACY_FLAGS = {"-f": "import", "-d": "dump", "-t": "test", "-p": "perf"}


def _normalize_argv(argv: list[str]) -> list[str]:
    """Translate the first legacy flag (-f/-d/-t/-p) into its sub-command name.

    Keeps backward compatibility with RE6Commander's ``-d bplan`` / ``-f file`` while
    still supporting the named sub-commands. Only the first legacy flag is rewritten,
    so a bill-plan/file argument that happens to look like a flag is left alone.
    """
    out: list[str] = []
    replaced = False
    for tok in argv:
        if not replaced and tok in LEGACY_FLAGS:
            out.append(LEGACY_FLAGS[tok])
            replaced = True
        else:
            out.append(tok)
    return out


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="re7commander",
        description="RateEngine V7 admin/provisioning CLI (port of RE6Commander).",
        epilog="Legacy flags are accepted too: -f=import, -d=dump, -t=test, -p=perf "
               "(e.g. 're7commander -d MobilePromo1').",
    )
    parser.add_argument("--env", metavar="PATH", help="path to .env file (default: py_cli/.env)")
    sub = parser.add_subparsers(dest="command", required=True)

    p_import = sub.add_parser("import", help="import tariff settings from CSV file")
    p_import.add_argument("file", help="input CSV file")
    p_import.set_defaults(func=cmd_import)

    p_dump = sub.add_parser("dump", help="dump a bill plan (csv/json/yaml)")
    p_dump.add_argument("bplan", help="bill plan name")
    p_dump.add_argument("--format", choices=["csv", "json", "yaml"], default="csv",
                        help="output format (default: csv, the legacy layout)")
    p_dump.set_defaults(func=cmd_dump)

    p_test = sub.add_parser("test", help="create test calling-number accounts")
    p_test.add_argument("bill_plan", help="bill plan to assign to the accounts")
    p_test.add_argument("count", type=int, help="how many accounts to create")
    p_test.add_argument("--start", type=int, default=0, help="starting index (default 0)")
    p_test.add_argument("--amount", default=20, help="prepaid card amount (default 20)")
    p_test.add_argument("--prefix", default="35910", help="number prefix (default 35910)")
    p_test.add_argument("--sm-bill-plan", dest="sm_bill_plan", default=None,
                        help="secondary bill plan (currently ignored)")
    p_test.set_defaults(func=cmd_test)

    p_gen = sub.add_parser("gen-cdrs", help="insert random test CDRs into fs_cdrs")
    p_gen.add_argument("--count", type=int, default=1, help="how many CDRs to insert (default 1)")
    p_gen.set_defaults(func=cmd_gen_cdrs)

    p_perf = sub.add_parser("perf", aliases=["-p"], help="rating performance testing")
    p_perf.set_defaults(func=cmd_perf)

    return parser


def main(argv: list[str] | None = None) -> int:
    if argv is None:
        argv = sys.argv[1:]
    parser = build_parser()
    args = parser.parse_args(_normalize_argv(argv))
    try:
        cfg = Config.load(args.env)
        return args.func(args, cfg) or 0
    except CliError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
