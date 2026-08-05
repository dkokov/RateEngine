<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/**
 * balance — money, engine-owned. The API/library only READS balances and may
 * create an initial row or toggle the active flag; it never recomputes amounts
 * (rating does that). Guarded status changes.
 */
final class BalanceRepository extends AbstractRepository
{
    /** @return array all balance rows for an account */
    public function findByAccount(int $accountId): array
    {
        return $this->db->all(
            'SELECT * FROM balance WHERE billing_account_id = ? ORDER BY id',
            [$accountId],
        );
    }

    /** Current amount of the active balance, or null if none. */
    public function currentAmount(int $accountId): ?string
    {
        $v = $this->db->value(
            'SELECT amount FROM balance WHERE billing_account_id = ? AND active = true ORDER BY id DESC LIMIT 1',
            [$accountId],
        );

        return $v === null ? null : (string) $v;
    }

    /** Create an initial balance row if the account has none. Returns its id. */
    public function ensure(int $accountId, string $amount, bool $active = true): int
    {
        $existing = $this->db->value(
            'SELECT id FROM balance WHERE billing_account_id = ? ORDER BY id LIMIT 1',
            [$accountId],
        );
        if ($existing !== null) {
            return (int) $existing;
        }

        return $this->db->insertReturningId(
            'INSERT INTO balance (billing_account_id, amount, active, last_update)
             VALUES (?, ?, ?, now()) RETURNING id',
            [$accountId, $amount, $active ? 't' : 'f'],
        );
    }

    /** ChangeBalanceStatus — guarded. */
    public function setActive(int $balanceId, bool $active): void
    {
        $this->gate->assertNotInUse('balance', $balanceId);
        $this->db->execute(
            'UPDATE balance SET active = ?, last_update = now() WHERE id = ?',
            [$active ? 't' : 'f', $balanceId],
        );
    }
}
