<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/**
 * bill_plan (id, name UNIQUE, bill_plan_type_id, start_period, end_period)
 * and the seeded lookup bill_plan_type (id, name).
 *
 * This is the reference implementation other repositories should follow:
 * a findId() lookup and an effective-dated getOrCreate().
 */
final class BillPlanRepository extends AbstractRepository
{
    /** @return int|null bill_plan.id for $name, or null if absent */
    public function findId(string $name): ?int
    {
        $id = $this->db->value(
            'SELECT id FROM bill_plan WHERE name = ?',
            [$name],
        );

        return $id === null ? null : (int) $id;
    }

    /** @return int|null bill_plan_type.id for a type name (e.g. postpaid/prepaid) */
    public function typeIdByName(string $typeName): ?int
    {
        $id = $this->db->value(
            'SELECT id FROM bill_plan_type WHERE name = ?',
            [$typeName],
        );

        return $id === null ? null : (int) $id;
    }

    /**
     * Get-or-create a bill plan by unique name.
     *
     * Effective-dated: a new plan is inserted with its validity window; this
     * method never mutates an existing plan's type/periods in place (that would
     * be a versioning operation, guarded and added separately).
     *
     * @param string $type postpaid/prepaid; resolved via bill_plan_type
     */
    public function getOrCreate(
        string $name,
        string $type = 'postpaid',
        int $startPeriod = 0,
        int $endPeriod = 0,
    ): int {
        $existing = $this->findId($name);
        if ($existing !== null) {
            return $existing;
        }

        // default to postpaid (2) if the type name is unknown/unseeded
        $typeId = $this->typeIdByName($type) ?? 2;

        return $this->db->insertReturningId(
            'INSERT INTO bill_plan (name, bill_plan_type_id, start_period, end_period)
             VALUES (?, ?, ?, ?) RETURNING id',
            [$name, $typeId, $startPeriod, $endPeriod],
        );
    }
}
