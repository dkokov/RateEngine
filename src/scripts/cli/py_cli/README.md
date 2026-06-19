# re7commander — RateEngine V7 CLI (Python 3)

Python 3 port of the legacy PHP `RE6Commander` admin/provisioning CLI. See
[`../MIGRATION.md`](../MIGRATION.md) for the full porting plan, function mapping, and
conventions. This is **Phase 1** (foundation): config, DB layer, and the command
skeleton. Import/dump/test handlers are stubs filled in by later phases.

## Layout

```
py_cli/
├── re7commander.py        # runnable wrapper (./re7commander.py ...)
├── pyproject.toml         # deps: psycopg[binary]; console script "re7commander"
├── .env.example           # copy to .env, set DB credentials
└── re_cli/
    ├── config.py          # .env loader + Config/DbConfig  (was config.app.php)
    ├── database.py        # Database: connect, fetch_*, insert_returning, transaction
    ├── errors.py          # CliError / ConfigError  (replaces PHP exit/die)
    ├── cli.py             # argparse entry point       (was RE6Commander switch)
    └── re7/               # ports of lib.db/importing/dumping/cdrserver  (Phases 2,3,5)
```

The legacy `lib.re5.php` is **not** ported: its functions duplicate the re7 ones in
`lib.db.php`. Test-account creation (Phase 5) is built on the re7 layer instead.

## Setup

```bash
cd py_cli
cp .env.example .env          # then edit credentials
python3 -m venv .venv && . .venv/bin/activate
pip install -e .              # installs psycopg + the "re7commander" command
```

No-install alternative (needs `psycopg` importable): run `./re7commander.py ...`.

## Usage

```bash
./re7commander.py import test_bp/SMS.csv     # was: ./RE6Commander -f test_bp/SMS.csv
./re7commander.py dump MobilePromo1          # was: ./RE6Commander -d MobilePromo1
```

Short aliases `-f`/`-d`/`-t`/`-p` are accepted as subcommand names too.

## Status

Phase 1 complete: configuration, connection layer, and CLI wiring are in place and
import cleanly. Database-backed commands raise a clear "not implemented yet" error
until their phase lands.
