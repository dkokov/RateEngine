<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/** prefix (id, prefix UNIQUE, comm). */
final class PrefixRepository extends AbstractRepository
{
    /** @return int|null prefix.id for $prefix, or null if absent */
    public function findId(string $prefix): ?int
    {
        $id = $this->db->value(
            'SELECT id FROM prefix WHERE prefix = ?',
            [$prefix],
        );

        return $id === null ? null : (int) $id;
    }

    /** Get-or-create a prefix by its unique string; $comm is a free-text note. */
    public function getOrCreate(string $prefix, ?string $comm = null): int
    {
        $existing = $this->findId($prefix);
        if ($existing !== null) {
            return $existing;
        }

        return $this->db->insertReturningId(
            'INSERT INTO prefix (prefix, comm) VALUES (?, ?) RETURNING id',
            [$prefix, $comm],
        );
    }
}
