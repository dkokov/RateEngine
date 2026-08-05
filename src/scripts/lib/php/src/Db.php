<?php

declare(strict_types=1);

namespace RateEngine\RE7;

use PDO;
use PDOException;
use RateEngine\RE7\Exception\DbException;
use Throwable;

/**
 * Thin PDO wrapper. Every query is a prepared statement with bound parameters —
 * this is the single place SQL is executed, and there is no string-concat path.
 */
final class Db
{
    private ?PDO $pdo = null;

    public function __construct(private readonly Config $config)
    {
    }

    public function pdo(): PDO
    {
        if ($this->pdo === null) {
            try {
                $this->pdo = new PDO(
                    $this->config->dsn(),
                    $this->config->user,
                    $this->config->password,
                    [
                        PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
                        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
                        PDO::ATTR_EMULATE_PREPARES => false,
                    ],
                );
            } catch (PDOException $e) {
                throw new DbException('database connect failed: ' . $e->getMessage(), 0, $e);
            }
        }

        return $this->pdo;
    }

    /** All rows as associative arrays. */
    public function all(string $sql, array $params = []): array
    {
        return $this->run($sql, $params)->fetchAll();
    }

    /** First row as an associative array, or null. */
    public function one(string $sql, array $params = []): ?array
    {
        $row = $this->run($sql, $params)->fetch();

        return $row === false ? null : $row;
    }

    /** First column of the first row, or null. */
    public function value(string $sql, array $params = []): mixed
    {
        $row = $this->run($sql, $params)->fetch(PDO::FETCH_NUM);

        return $row === false ? null : $row[0];
    }

    /** Run an INSERT ... RETURNING id and return the new id. */
    public function insertReturningId(string $sql, array $params = []): int
    {
        return (int) $this->value($sql, $params);
    }

    /** Execute a write; returns affected row count. */
    public function execute(string $sql, array $params = []): int
    {
        return $this->run($sql, $params)->rowCount();
    }

    /**
     * Run $fn inside a transaction. Commits on success, rolls back on any
     * throwable, then re-throws. Nested calls reuse the active transaction.
     *
     * @template T
     * @param callable(self):T $fn
     * @return T
     */
    public function transaction(callable $fn): mixed
    {
        $pdo = $this->pdo();

        if ($pdo->inTransaction()) {
            return $fn($this);
        }

        $pdo->beginTransaction();
        try {
            $result = $fn($this);
            $pdo->commit();

            return $result;
        } catch (Throwable $e) {
            $pdo->rollBack();
            throw $e;
        }
    }

    private function run(string $sql, array $params): \PDOStatement
    {
        try {
            $st = $this->pdo()->prepare($sql);
            $st->execute($params);

            return $st;
        } catch (PDOException $e) {
            throw new DbException('query failed: ' . $e->getMessage(), 0, $e);
        }
    }
}
