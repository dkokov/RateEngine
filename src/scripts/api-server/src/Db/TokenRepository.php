<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Db;

/** Persists refresh-token hashes and the audit log in the SQLite auth store. */
final class TokenRepository
{
    public function __construct(private readonly AuthDb $db)
    {
    }

    public function storeRefresh(int $userId, string $hash, int $expiresAt): void
    {
        $this->db->execute(
            "INSERT INTO api_token (user_id, refresh_hash, expires_at)
             VALUES (?, ?, datetime(?, 'unixepoch'))",
            [$userId, $hash, $expiresAt],
        );
    }

    /** @return array{id:int,user_id:int}|null a live (unrevoked, unexpired) token row */
    public function findLiveRefresh(string $hash): ?array
    {
        $row = $this->db->one(
            "SELECT id, user_id FROM api_token
             WHERE refresh_hash = ? AND revoked = 0 AND expires_at > datetime('now')",
            [$hash],
        );

        return $row === null ? null : ['id' => (int) $row['id'], 'user_id' => (int) $row['user_id']];
    }

    public function revokeByHash(string $hash): void
    {
        $this->db->execute('UPDATE api_token SET revoked = 1 WHERE refresh_hash = ?', [$hash]);
    }

    public function audit(
        ?int $userId,
        ?string $username,
        string $method,
        string $path,
        int $status,
        string $remoteIp,
        ?string $detail = null,
    ): void {
        $this->db->execute(
            'INSERT INTO audit_log (user_id, username, method, path, status, detail, remote_ip)
             VALUES (?, ?, ?, ?, ?, ?, ?)',
            [$userId, $username, $method, $path, $status, $detail, $remoteIp],
        );
    }
}
