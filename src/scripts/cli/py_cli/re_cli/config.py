"""Configuration loading — replaces config.app.php.

Credentials live in a ``.env`` file (see ``.env.example``), not in source. We parse
it with a tiny built-in reader so the CLI has no hard dependency on python-dotenv;
real environment variables always win over ``.env`` entries.
"""

from __future__ import annotations

import os
from dataclasses import dataclass
from decimal import Decimal
from pathlib import Path

from .errors import ConfigError


def _load_dotenv(path: Path) -> dict[str, str]:
    """Parse a minimal ``KEY=value`` .env file. Ignores blanks and ``#`` comments.

    Surrounding single/double quotes on values are stripped. Missing file -> {}.
    """
    values: dict[str, str] = {}
    if not path.is_file():
        return values
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, _, val = line.partition("=")
        key = key.strip()
        val = val.strip()
        if len(val) >= 2 and val[0] == val[-1] and val[0] in "\"'":
            val = val[1:-1]
        values[key] = val
    return values


@dataclass(frozen=True)
class DbConfig:
    """Connection parameters for one PostgreSQL database."""

    host: str
    port: int
    name: str
    user: str
    password: str

    @property
    def configured(self) -> bool:
        """True when a database name is set (blank name => this DB is unused)."""
        return bool(self.name)

    def conninfo(self) -> str:
        """libpq connection string for psycopg.connect()."""
        if not self.configured:
            raise ConfigError("database name is not configured (check your .env)")
        return (
            f"host={self.host} port={self.port} dbname={self.name} "
            f"user={self.user} password={self.password}"
        )


@dataclass(frozen=True)
class Config:
    """Full CLI configuration: the three databases, MyCC, and billing defaults."""

    re7: DbConfig          # current engine schema (legacy config.app.php dbname "re7")
    re5: DbConfig          # legacy schema used by lib.re5.php
    cdr: DbConfig          # FreeSWITCH CDR store (fs_cdrs)
    mycc_host: str
    mycc_port: int
    billing_day: str
    credit_limit: Decimal
    rounding: int

    @classmethod
    def load(cls, env_path: str | os.PathLike[str] | None = None) -> "Config":
        """Build a Config from ``.env`` (next to py_cli/ by default) + os.environ."""
        if env_path is None:
            env_path = Path(__file__).resolve().parent.parent / ".env"
        file_env = _load_dotenv(Path(env_path))

        def get(key: str, default: str = "") -> str:
            return os.environ.get(key, file_env.get(key, default))

        def db(prefix: str, default_name: str = "") -> DbConfig:
            return DbConfig(
                host=get(f"{prefix}_DB_HOST", "127.0.0.1"),
                port=int(get(f"{prefix}_DB_PORT", "5432")),
                name=get(f"{prefix}_DB_NAME", default_name),
                user=get(f"{prefix}_DB_USER", "global"),
                password=get(f"{prefix}_DB_PASS", ""),
            )

        return cls(
            re7=db("RE7", "re7"),
            re5=db("RE5"),
            cdr=db("CDR", "fs_cdrs"),
            mycc_host=get("MYCC_HOST", "127.0.0.1"),
            mycc_port=int(get("MYCC_PORT", "9090")),
            billing_day=get("RE_BILLING_DAY", "01"),
            credit_limit=Decimal(get("RE_CREDIT_LIMIT", "50.00")),
            rounding=int(get("RE_ROUNDING", "0")),
        )
