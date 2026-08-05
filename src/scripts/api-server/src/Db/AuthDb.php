<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Db;

use PDO;
use PDOException;
use RateEngine\RE7\Api\Exception\ApiException;

/** SQLite wrapper for the API auth store (users, tokens, audit). */
final class AuthDb
{
    private ?PDO $pdo = null;

    public function __construct(private readonly string $path)
    {
    }

    public function pdo(): PDO
    {
        if ($this->pdo === null) {
            $dir = dirname($this->path);
            if (!is_dir($dir)) {
                @mkdir($dir, 0o750, true);
            }
            try {
                $this->pdo = new PDO('sqlite:' . $this->path, null, null, [
                    PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
                    PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
                ]);
                $this->pdo->exec('PRAGMA journal_mode = WAL');
                $this->pdo->exec('PRAGMA foreign_keys = ON');
            } catch (PDOException $e) {
                throw new ApiException(500, 'auth store open failed: ' . $e->getMessage(), $e);
            }
        }

        return $this->pdo;
    }

    public function all(string $sql, array $params = []): array
    {
        return $this->run($sql, $params)->fetchAll();
    }

    public function one(string $sql, array $params = []): ?array
    {
        $row = $this->run($sql, $params)->fetch();

        return $row === false ? null : $row;
    }

    public function execute(string $sql, array $params = []): int
    {
        return $this->run($sql, $params)->rowCount();
    }

    public function lastInsertId(): int
    {
        return (int) $this->pdo()->lastInsertId();
    }

    /** Apply the schema file (idempotent — CREATE TABLE IF NOT EXISTS). */
    public function initSchema(string $sqlFile): void
    {
        $sql = file_get_contents($sqlFile);
        if ($sql === false) {
            throw new ApiException(500, 'cannot read schema: ' . $sqlFile);
        }
        $this->pdo()->exec($sql);
    }

    private function run(string $sql, array $params): \PDOStatement
    {
        try {
            $st = $this->pdo()->prepare($sql);
            $st->execute($params);

            return $st;
        } catch (PDOException $e) {
            throw new ApiException(500, 'auth query failed: ' . $e->getMessage(), $e);
        }
    }
}
