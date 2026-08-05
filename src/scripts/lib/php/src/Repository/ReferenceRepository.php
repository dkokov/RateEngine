<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/** Read-only lists of the seeded reference tables (for GUI dropdowns etc.). */
final class ReferenceRepository extends AbstractRepository
{
    public function currencies(): array
    {
        return $this->db->all('SELECT id, name, to_bg FROM currency ORDER BY id');
    }

    public function billPlanTypes(): array
    {
        return $this->db->all('SELECT id, name FROM bill_plan_type ORDER BY id');
    }

    public function pcardTypes(): array
    {
        return $this->db->all('SELECT id, name, "desc" FROM pcard_type ORDER BY id');
    }

    public function pcardStatuses(): array
    {
        return $this->db->all('SELECT id, status FROM pcard_status ORDER BY id');
    }

    public function roundModes(): array
    {
        return $this->db->all('SELECT id, name FROM round_mode ORDER BY id');
    }

    public function ratingModes(): array
    {
        return $this->db->all('SELECT id, name FROM rating_mode ORDER BY id');
    }
}
