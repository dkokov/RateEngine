<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Db;

use RateEngine\RE7\Api\Exception\ApiException;

/** Verifies username/password against the SQLite auth store. */
final class UserRepository
{
    public function __construct(private readonly AuthDb $db)
    {
    }

    /**
     * @return array{id:int,username:string,scopes:string}
     *
     * @throws ApiException 401 on unknown user, bad password or inactive account
     */
    public function authenticate(string $username, string $password): array
    {
        $row = $this->db->one(
            'SELECT u.id, u.username, u.password_hash, u.active, r.scopes
             FROM api_user u JOIN api_role r ON r.id = u.role_id
             WHERE u.username = ?',
            [$username],
        );

        // verify even when the user is missing to reduce timing signal
        $hash = $row['password_hash'] ?? '$2y$10$invalidinvalidinvalidinvalidinvalidinvalidinva';
        $ok = password_verify($password, $hash);

        if ($row === null || (int) $row['active'] !== 1 || !$ok) {
            throw new ApiException(401, 'invalid credentials');
        }

        $this->db->execute(
            "UPDATE api_user SET last_login = datetime('now') WHERE id = ?",
            [$row['id']],
        );

        return [
            'id' => (int) $row['id'],
            'username' => $row['username'],
            'scopes' => $row['scopes'],
        ];
    }

    /**
     * Load an active user by id (used on refresh, when the password was already
     * proven at login).
     *
     * @return array{id:int,username:string,scopes:string}
     */
    public function authenticateById(int $id): array
    {
        $row = $this->db->one(
            'SELECT u.id, u.username, u.active, r.scopes
             FROM api_user u JOIN api_role r ON r.id = u.role_id
             WHERE u.id = ?',
            [$id],
        );

        if ($row === null || (int) $row['active'] !== 1) {
            throw new ApiException(401, 'account not available');
        }

        return [
            'id' => (int) $row['id'],
            'username' => $row['username'],
            'scopes' => $row['scopes'],
        ];
    }

    /** Create a user with a hashed password. Returns the new id. */
    public function create(string $username, string $password, int $roleId): int
    {
        $hash = password_hash($password, PASSWORD_DEFAULT);
        $this->db->execute(
            'INSERT INTO api_user (username, password_hash, role_id) VALUES (?, ?, ?)',
            [$username, $hash, $roleId],
        );

        return $this->db->lastInsertId();
    }
}
