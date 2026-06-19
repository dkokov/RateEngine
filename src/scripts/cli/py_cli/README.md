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

Dependencies only (no console script): `pip install -r requirements.txt`, then run
`./re7commander.py ...`. The deps are `psycopg[binary]` (required) and `pyyaml`
(optional, for `dump --format yaml`).

## Usage

```bash
./re7commander.py import ../test_bp/SMS.csv      # was: ./RE6Commander -f test_bp/SMS.csv
./re7commander.py dump MobilePromo1              # was: ./RE6Commander -d MobilePromo1
./re7commander.py dump MobilePromo1 --format json   # csv (default) | json | yaml
./re7commander.py test MobilePromo1 100 --start 0 --amount 20   # create 100 test accounts
./re7commander.py gen-cdrs --count 50            # insert 50 random CDRs into fs_cdrs
```

`--format csv` reproduces the legacy layout byte-for-byte. `json` uses the stdlib;
`yaml` uses PyYAML if installed (`pip install -e '.[yaml]'`), otherwise a small
built-in emitter.

Short aliases `-f`/`-d`/`-t`/`-p` are accepted as subcommand names too.

## Status

The PHP CLI is ported: `config.app.php`, all of `lib.db.php`, `lib.importing.php`,
`lib.dumping.php`, `lib.cdrserver.php`, and `lib.testing.php`. `perf` is a stub (the
PHP `-p` was empty). What remains is validation against a live database (the parity
gate) and final cutover — see [`../MIGRATION.md`](../MIGRATION.md) §5.
