"""Exception types for the CLI.

Replaces the PHP code's hard ``exit;`` / ``die()`` calls (see MIGRATION.md §3.4).
The entry point catches :class:`CliError`, prints the message, and sets a non-zero
exit code; everything below the entry point raises instead of terminating the process.
"""


class CliError(Exception):
    """A recoverable, user-facing error. Aborts the current command cleanly."""


class ConfigError(CliError):
    """Configuration is missing or invalid (bad/absent .env, unset DB name, ...)."""
