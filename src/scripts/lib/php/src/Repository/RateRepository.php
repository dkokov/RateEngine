<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/**
 * rate (id, bill_plan_id, tariff_id, prefix_id) — the join row that binds a
 * prefix to a tariff within a bill plan.
 */
final class RateRepository extends AbstractRepository
{
    /** @return int|null rate.id for the (bill_plan, prefix, tariff) triple */
    public function findId(int $billPlanId, int $prefixId, int $tariffId): ?int
    {
        $id = $this->db->value(
            'SELECT id FROM rate WHERE bill_plan_id = ? AND prefix_id = ? AND tariff_id = ?',
            [$billPlanId, $prefixId, $tariffId],
        );

        return $id === null ? null : (int) $id;
    }

    /** List rates under a bill plan, with prefix and tariff names. */
    public function listByBillPlan(string $billPlanName): array
    {
        return $this->db->all(
            'SELECT r.id, pr.prefix, tr.name AS tariff
             FROM rate r
             JOIN bill_plan bp ON bp.id = r.bill_plan_id
             JOIN prefix pr    ON pr.id = r.prefix_id
             JOIN tariff tr    ON tr.id = r.tariff_id
             WHERE bp.name = ? ORDER BY pr.prefix',
            [$billPlanName],
        );
    }

    /** Get-or-create the rate row for a (bill_plan, prefix, tariff) triple. */
    public function getOrCreate(int $billPlanId, int $prefixId, int $tariffId): int
    {
        $existing = $this->findId($billPlanId, $prefixId, $tariffId);
        if ($existing !== null) {
            return $existing;
        }

        return $this->db->insertReturningId(
            'INSERT INTO rate (bill_plan_id, tariff_id, prefix_id)
             VALUES (?, ?, ?) RETURNING id',
            [$billPlanId, $tariffId, $prefixId],
        );
    }
}
