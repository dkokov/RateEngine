"""PostgreSQL access layer — replaces the global ``$dbconn`` + ``pg_*`` calls.

One :class:`Database` instance wraps one connection. The legacy code kept the
connection in module globals (``$dbconn``, ``$RE['dbconn']``, ``$RE6['dbconn']``) and
every function did ``global $dbconn``; here the connection is an object the ported
functions receive explicitly (MIGRATION.md §3.2).

All helpers take a SQL string with ``%s`` placeholders and a parameter tuple — never
interpolate values into SQL (MIGRATION.md §3.1). For dynamic identifiers use
:class:`psycopg.sql.Identifier`.
"""

from __future__ import annotations

from contextlib import contextmanager
from typing import TYPE_CHECKING, Any, Iterator, Sequence

from .config import DbConfig
from .errors import CliError

if TYPE_CHECKING:  # import only for type checkers; avoids a hard runtime dependency
    import psycopg

Params = Sequence[Any] | None


def _import_psycopg():
    """Lazily import psycopg so config/--help work even when the driver is absent."""
    try:
        import psycopg  # noqa: PLC0415
    except ModuleNotFoundError as exc:  # pragma: no cover - environment dependent
        raise CliError(
            "the 'psycopg' package is required for database access; "
            "install it with: pip install -e .  (or: pip install 'psycopg[binary]')"
        ) from exc
    return psycopg


class Database:
    """A thin wrapper over a psycopg connection with convenience fetch/exec helpers.

    Rows are returned as dicts (``psycopg.rows.dict_row``) so ported code uses column
    names instead of the legacy numeric ``pg_fetch_row()[0]`` indexing.
    """

    def __init__(self, conn: psycopg.Connection, label: str = "db") -> None:
        self._conn = conn
        self.label = label

    # -- lifecycle ---------------------------------------------------------------

    @classmethod
    def connect(cls, cfg: DbConfig, label: str = "db") -> "Database":
        """Open a connection from a DbConfig. Raises CliError on failure."""
        psycopg = _import_psycopg()
        from psycopg.rows import dict_row

        try:
            conn = psycopg.connect(cfg.conninfo(), row_factory=dict_row)
        except psycopg.Error as exc:
            raise CliError(f"cannot connect to {label} ({cfg.name}): {exc}") from exc
        return cls(conn, label=label)

    @property
    def connection(self) -> psycopg.Connection:
        return self._conn

    def close(self) -> None:
        if self._conn and not self._conn.closed:
            self._conn.close()

    def __enter__(self) -> "Database":
        return self

    def __exit__(self, *_exc: object) -> None:
        self.close()

    # -- transactions ------------------------------------------------------------

    @contextmanager
    def transaction(self) -> Iterator["Database"]:
        """Atomic block: commits on success, rolls back on any exception.

        Replaces the manual ``re5_begin/commit/rollback`` and the goto-based fake
        transaction in ``re5_create_account`` (MIGRATION.md §3.3, §4.4).
        """
        with self._conn.transaction():
            yield self

    # -- queries -----------------------------------------------------------------

    def execute(self, sql: str, params: Params = None) -> int:
        """Run a statement, return affected row count. Auto-commits (autocommit-style).

        Uses an implicit transaction per call unless wrapped in :meth:`transaction`.
        """
        with self._conn.cursor() as cur:
            cur.execute(sql, params)
            rowcount = cur.rowcount
        self._conn.commit()
        return rowcount

    def fetch_all(self, sql: str, params: Params = None) -> list[dict[str, Any]]:
        """Return all rows as a list of dicts ([] when none)."""
        with self._conn.cursor() as cur:
            cur.execute(sql, params)
            return cur.fetchall()

    def fetch_one(self, sql: str, params: Params = None) -> dict[str, Any] | None:
        """Return the first row as a dict, or None when there is no row."""
        with self._conn.cursor() as cur:
            cur.execute(sql, params)
            return cur.fetchone()

    def fetch_val(self, sql: str, params: Params = None) -> Any | None:
        """Return the first column of the first row, or None.

        Replaces the ubiquitous ``$row = pg_fetch_row(...); return $row[0];`` idiom.
        """
        row = self.fetch_one(sql, params)
        if row is None:
            return None
        return next(iter(row.values()))

    def insert_returning(self, sql: str, params: Params = None) -> Any:
        """Run an ``INSERT ... RETURNING <col>`` and return that single value.

        The canonical replacement for the PHP ``insert_x(); goto check; id=get_x();``
        retry loops (MIGRATION.md §3.3).
        """
        with self._conn.cursor() as cur:
            cur.execute(sql, params)
            row = cur.fetchone()
        self._conn.commit()
        if row is None:
            raise CliError("INSERT ... RETURNING produced no row")
        return next(iter(row.values()))
