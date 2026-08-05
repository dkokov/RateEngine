<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/**
 * billing_account (id, username UNIQUE, currency_id, leg, cdr_server_id,
 * billing_day, round_mode_id, day_of_payment).
 */
final class BillingAccountRepository extends AbstractRepository
{
    public function findId(string $username): ?int
    {
        $v = $this->db->value('SELECT id FROM billing_account WHERE username = ?', [$username]);

        return $v === null ? null : (int) $v;
    }

    public function get(string $username): ?array
    {
        return $this->db->one('SELECT * FROM billing_account WHERE username = ?', [$username]);
    }

    /**
     * Get-or-create a billing account by unique username.
     *
     * @param array $opts currency_id, leg, cdr_server_id, billing_day, round_mode_id, day_of_payment
     */
    public function getOrCreate(string $username, array $opts = []): int
    {
        $existing = $this->findId($username);
        if ($existing !== null) {
            return $existing;
        }

        return $this->db->insertReturningId(
            'INSERT INTO billing_account
                (username, currency_id, leg, cdr_server_id, billing_day, round_mode_id, day_of_payment)
             VALUES (?, ?, ?, ?, ?, ?, ?) RETURNING id',
            [
                $username,
                (int) ($opts['currency_id'] ?? 1),
                (string) ($opts['leg'] ?? 'a'),
                (int) ($opts['cdr_server_id'] ?? 0),
                (string) ($opts['billing_day'] ?? '01'),
                (int) ($opts['round_mode_id'] ?? 0),
                (int) ($opts['day_of_payment'] ?? 0),
            ],
        );
    }
}
