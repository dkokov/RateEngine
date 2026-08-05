<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/** Read-only lookups for the seeded reference tables (name -> id). */
final class LookupRepository extends AbstractRepository
{
    public function currencyId(string $name): ?int
    {
        return $this->id('SELECT id FROM currency WHERE name = ?', $name);
    }

    public function roundModeId(string $name): ?int
    {
        return $this->id('SELECT id FROM round_mode WHERE name = ?', $name);
    }

    public function ratingModeId(string $name): ?int
    {
        return $this->id('SELECT id FROM rating_mode WHERE name = ?', $name);
    }

    /** pcard_status.status: deactive(0) / active(1) / block(2) */
    public function pcardStatusId(string $status): ?int
    {
        return $this->id('SELECT id FROM pcard_status WHERE status = ?', $status);
    }

    public function pcardTypeId(string $name): ?int
    {
        return $this->id('SELECT id FROM pcard_type WHERE name = ?', $name);
    }

    public function billPlanTypeId(string $name): ?int
    {
        return $this->id('SELECT id FROM bill_plan_type WHERE name = ?', $name);
    }

    private function id(string $sql, string $key): ?int
    {
        $v = $this->db->value($sql, [$key]);

        return $v === null ? null : (int) $v;
    }
}
