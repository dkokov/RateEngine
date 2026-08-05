<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/**
 * Read-only rated-calls reporting: joins the engine-written `rating` rows to
 * their `cdrs` (rating.call_id = cdrs.id). Heavy analytics can later be moved
 * to DuckDB; this covers per-account statements.
 */
final class ReportRepository extends AbstractRepository
{
    /** Individual rated calls for an account within an optional date range. */
    public function ratedCalls(int $accountId, ?string $from, ?string $to, int $limit = 1000): array
    {
        [$where, $params] = $this->range($accountId, $from, $to);
        $params[] = $limit;

        return $this->db->all(
            "SELECT cdrs.start_ts, cdrs.calling_number, cdrs.called_number,
                    rt.call_price, rt.call_billsec, rt.call_ts
             FROM rating rt JOIN cdrs ON cdrs.id = rt.call_id
             WHERE {$where}
             ORDER BY rt.call_ts DESC LIMIT ?",
            $params,
        );
    }

    /** Totals for an account within an optional date range. */
    public function summary(int $accountId, ?string $from, ?string $to): array
    {
        [$where, $params] = $this->range($accountId, $from, $to);
        $row = $this->db->one(
            "SELECT count(*) AS calls,
                    coalesce(sum(rt.call_billsec), 0) AS billsec,
                    coalesce(sum(rt.call_price), 0)  AS amount
             FROM rating rt WHERE {$where}",
            $params,
        );

        return [
            'calls' => (int) ($row['calls'] ?? 0),
            'billsec' => (int) ($row['billsec'] ?? 0),
            'amount' => (string) ($row['amount'] ?? '0'),
        ];
    }

    /** @return array{0:string,1:array} where-clause + bound params */
    private function range(int $accountId, ?string $from, ?string $to): array
    {
        $where = 'rt.billing_account_id = ?';
        $params = [$accountId];
        if ($from !== null && $from !== '') {
            $where .= ' AND rt.call_ts >= ?';
            $params[] = $from;
        }
        if ($to !== null && $to !== '') {
            $where .= ' AND rt.call_ts < ?';
            $params[] = $to;
        }

        return [$where, $params];
    }
}
