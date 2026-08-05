<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/**
 * pcard (prepaid card). Money-adjacent: create/read freely, but status/limit
 * changes are guarded against live calls. The engine owns balance movement.
 */
final class PcardRepository extends AbstractRepository
{
    /**
     * @param array $opts start_date, end_date, call_number, sim
     */
    public function create(int $accountId, string $amount, int $statusId, int $typeId, array $opts = []): int
    {
        return $this->db->insertReturningId(
            "INSERT INTO pcard
                (amount, start_date, end_date, last_update, pcard_status_id,
                 billing_account_id, pcard_type_id, call_number, saved_amount, sim)
             VALUES (?, ?, ?, now(), ?, ?, ?, ?, 0, ?) RETURNING id",
            [
                $amount,
                $opts['start_date'] ?? null,
                $opts['end_date'] ?? null,
                $statusId,
                $accountId,
                $typeId,
                (int) ($opts['call_number'] ?? 1),
                (int) ($opts['sim'] ?? 0),
            ],
        );
    }

    public function get(int $id): ?array
    {
        return $this->db->one('SELECT * FROM pcard WHERE id = ?', [$id]);
    }

    public function findByAccount(int $accountId): array
    {
        return $this->db->all(
            'SELECT p.*, s.status FROM pcard p
             JOIN pcard_status s ON s.id = p.pcard_status_id
             WHERE p.billing_account_id = ? ORDER BY p.id',
            [$accountId],
        );
    }

    /** ChangePCardStatus — guarded. */
    public function updateStatus(int $id, int $statusId): void
    {
        $this->gate->assertNotInUse('pcard', $id);
        $this->db->execute(
            'UPDATE pcard SET pcard_status_id = ?, last_update = now() WHERE id = ?',
            [$statusId, $id],
        );
    }

    /** UpdateCreditLimit — guarded (amount = credit limit). */
    public function updateLimit(int $id, string $amount): void
    {
        $this->gate->assertNotInUse('pcard', $id);
        $this->db->execute(
            'UPDATE pcard SET amount = ?, last_update = now() WHERE id = ?',
            [$amount, $id],
        );
    }
}
