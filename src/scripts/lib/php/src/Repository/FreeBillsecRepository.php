<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/** free_billsec (id, free_billsec) — free-seconds values a tariff can reference. */
final class FreeBillsecRepository extends AbstractRepository
{
    public function getOrCreate(int $seconds): int
    {
        $id = $this->db->value('SELECT id FROM free_billsec WHERE free_billsec = ?', [$seconds]);
        if ($id !== null) {
            return (int) $id;
        }

        return $this->db->insertReturningId(
            'INSERT INTO free_billsec (free_billsec) VALUES (?) RETURNING id',
            [$seconds],
        );
    }

    public function list(): array
    {
        return $this->db->all('SELECT id, free_billsec FROM free_billsec ORDER BY id');
    }
}
