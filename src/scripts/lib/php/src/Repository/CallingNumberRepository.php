<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/**
 * calling_number (id, calling_number UNIQUE, billing_account_id) and its
 * calling_number_deff (id, calling_number_id, bill_plan_id, sm_bill_plan_id).
 */
final class CallingNumberRepository extends AbstractRepository
{
    public function findId(string $number): ?int
    {
        $v = $this->db->value('SELECT id FROM calling_number WHERE calling_number = ?', [$number]);

        return $v === null ? null : (int) $v;
    }

    public function getOrCreate(string $number, int $accountId): int
    {
        $existing = $this->findId($number);
        if ($existing !== null) {
            return $existing;
        }

        return $this->db->insertReturningId(
            'INSERT INTO calling_number (calling_number, billing_account_id) VALUES (?, ?) RETURNING id',
            [$number, $accountId],
        );
    }

    public function deffFindId(int $callingNumberId): ?int
    {
        $v = $this->db->value(
            'SELECT id FROM calling_number_deff WHERE calling_number_id = ?',
            [$callingNumberId],
        );

        return $v === null ? null : (int) $v;
    }

    public function deffGetOrCreate(int $callingNumberId, int $billPlanId, int $smBillPlanId = 0): int
    {
        $existing = $this->deffFindId($callingNumberId);
        if ($existing !== null) {
            return $existing;
        }

        return $this->db->insertReturningId(
            'INSERT INTO calling_number_deff (calling_number_id, bill_plan_id, sm_bill_plan_id)
             VALUES (?, ?, ?) RETURNING id',
            [$callingNumberId, $billPlanId, $smBillPlanId],
        );
    }

    /** Full view: number + account + assigned bill plan(s). */
    public function get(string $number): ?array
    {
        return $this->db->one(
            'SELECT cn.id, cn.calling_number, ba.username,
                    d.bill_plan_id, bp.name AS bill_plan,
                    d.sm_bill_plan_id, sbp.name AS sm_bill_plan
             FROM calling_number cn
             JOIN billing_account ba ON ba.id = cn.billing_account_id
             LEFT JOIN calling_number_deff d ON d.calling_number_id = cn.id
             LEFT JOIN bill_plan bp  ON bp.id  = d.bill_plan_id
             LEFT JOIN bill_plan sbp ON sbp.id = d.sm_bill_plan_id
             WHERE cn.calling_number = ?',
            [$number],
        );
    }

    /** ChangeBillPlan — effective-dated in spirit; guarded against live calls. */
    public function changeBillPlan(string $number, int $billPlanId): void
    {
        $id = $this->findId($number);
        if ($id === null) {
            return;
        }
        $this->gate->assertNotInUse('calling_number', $id);

        $this->db->execute(
            'UPDATE calling_number_deff SET bill_plan_id = ? WHERE calling_number_id = ?',
            [$billPlanId, $id],
        );
    }
}
