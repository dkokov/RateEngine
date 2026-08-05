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

    public function list(int $limit = 500): array
    {
        return $this->db->all(
            'SELECT id, username, currency_id, leg, billing_day, round_mode_id, day_of_payment
             FROM billing_account ORDER BY username LIMIT ?',
            [$limit],
        );
    }

    /**
     * Update only the provided columns (ChangeBillingAccount). Guarded.
     *
     * @param array $opts any of currency_id, leg, cdr_server_id, billing_day, round_mode_id, day_of_payment
     *
     * @return bool false if the account does not exist
     */
    public function update(string $username, array $opts): bool
    {
        $id = $this->findId($username);
        if ($id === null) {
            return false;
        }
        $this->gate->assertNotInUse('billing_account', $id);

        $cols = [];
        $params = [];
        foreach (['currency_id', 'leg', 'cdr_server_id', 'billing_day', 'round_mode_id', 'day_of_payment'] as $k) {
            if (array_key_exists($k, $opts)) {
                $cols[] = $k . ' = ?';
                $params[] = $opts[$k];
            }
        }
        if ($cols === []) {
            return true;
        }
        $params[] = $id;
        $this->db->execute('UPDATE billing_account SET ' . implode(', ', $cols) . ' WHERE id = ?', $params);

        return true;
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
