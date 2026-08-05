-- RE7 API auth store (SQLite). Separate from the RE7 billing DB so the API
-- server can deploy on a different host/container. Holds users, refresh tokens
-- and the audit log only — no billing data.

PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS api_role (
    id     INTEGER PRIMARY KEY,
    name   TEXT UNIQUE NOT NULL,
    scopes TEXT NOT NULL DEFAULT ''      -- space-separated scopes
);

CREATE TABLE IF NOT EXISTS api_user (
    id            INTEGER PRIMARY KEY,
    username      TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,          -- password_hash(), argon2id/bcrypt
    role_id       INTEGER NOT NULL REFERENCES api_role(id),
    active        INTEGER NOT NULL DEFAULT 1,
    created_at    TEXT NOT NULL DEFAULT (datetime('now')),
    last_login    TEXT
);

CREATE TABLE IF NOT EXISTS api_token (
    id           INTEGER PRIMARY KEY,
    user_id      INTEGER NOT NULL REFERENCES api_user(id),
    refresh_hash TEXT UNIQUE NOT NULL,     -- sha256 of the opaque refresh token
    issued_at    TEXT NOT NULL DEFAULT (datetime('now')),
    expires_at   TEXT NOT NULL,
    revoked      INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX IF NOT EXISTS idx_api_token_user ON api_token(user_id);

CREATE TABLE IF NOT EXISTS audit_log (
    id        INTEGER PRIMARY KEY,
    ts        TEXT NOT NULL DEFAULT (datetime('now')),
    user_id   INTEGER,
    username  TEXT,
    method    TEXT,
    path      TEXT,
    status    INTEGER,
    detail    TEXT,
    remote_ip TEXT
);
CREATE INDEX IF NOT EXISTS idx_audit_ts ON audit_log(ts);

-- seed roles (admin user is created via bin/re7-api-user.php, needs a password)
INSERT OR IGNORE INTO api_role (id, name, scopes) VALUES
    (1, 'admin',    'provisioning:read provisioning:write rating:read admin'),
    (2, 'operator', 'provisioning:read provisioning:write rating:read'),
    (3, 'readonly', 'provisioning:read rating:read');
