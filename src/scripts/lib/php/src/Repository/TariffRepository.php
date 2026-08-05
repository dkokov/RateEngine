<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/**
 * tariff (id, name UNIQUE, temp_id, start_period, end_period, free_billsec_id).
 */
final class TariffRepository extends AbstractRepository
{
    /** @return int|null tariff.id for $name, or null if absent */
    public function findId(string $name): ?int
    {
        $id = $this->db->value(
            'SELECT id FROM tariff WHERE name = ?',
            [$name],
        );

        return $id === null ? null : (int) $id;
    }

    /**
     * Get-or-create a tariff by unique name. Effective-dated via
     * start_period/end_period; free_billsec_id links optional free seconds.
     */
    public function getOrCreate(
        string $name,
        int $startPeriod = 0,
        int $endPeriod = 0,
        int $freeBillsecId = 0,
    ): int {
        $existing = $this->findId($name);
        if ($existing !== null) {
            return $existing;
        }

        return $this->db->insertReturningId(
            'INSERT INTO tariff (name, start_period, end_period, free_billsec_id)
             VALUES (?, ?, ?, ?) RETURNING id',
            [$name, $startPeriod, $endPeriod, $freeBillsecId],
        );
    }
}
