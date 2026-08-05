<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/**
 * Time conditions for a tariff: a time_condition row links a tariff to a
 * time_condition_deff (the hours/days window) with a priority. Rating selects
 * them as `df.id = tc.time_condition_id AND tc.tariff_id = ? ORDER BY tc.prior DESC`.
 */
final class TimeConditionRepository extends AbstractRepository
{
    /**
     * Create the window definition and link it to the tariff.
     *
     * @param array $deff hours, days_week, tc_name, tc_date, year, month, day_month
     *
     * @return array{time_condition_id:int,time_condition_deff_id:int}
     */
    public function create(int $tariffId, int $prior, array $deff): array
    {
        $deffId = $this->db->insertReturningId(
            'INSERT INTO time_condition_deff (hours, days_week, tc_name, tc_date, year, month, day_month)
             VALUES (?, ?, ?, ?, ?, ?, ?) RETURNING id',
            [
                $deff['hours'] ?? null,
                $deff['days_week'] ?? null,
                $deff['tc_name'] ?? null,
                $deff['tc_date'] ?? null,
                $deff['year'] ?? null,
                $deff['month'] ?? null,
                $deff['day_month'] ?? null,
            ],
        );

        $tcId = $this->db->insertReturningId(
            'INSERT INTO time_condition (tariff_id, time_condition_id, prior) VALUES (?, ?, ?) RETURNING id',
            [$tariffId, $deffId, $prior],
        );

        return ['time_condition_id' => $tcId, 'time_condition_deff_id' => $deffId];
    }

    public function listByTariff(int $tariffId): array
    {
        return $this->db->all(
            'SELECT tc.id, tc.prior, df.id AS deff_id, df.tc_name, df.hours, df.days_week, df.tc_date
             FROM time_condition tc
             JOIN time_condition_deff df ON df.id = tc.time_condition_id
             WHERE tc.tariff_id = ? ORDER BY tc.prior DESC',
            [$tariffId],
        );
    }

    /** Remove a tariff's time_condition (and the 1:1 deff it points at). */
    public function deleteById(int $tariffId, int $id): int
    {
        $deffId = $this->db->value(
            'SELECT time_condition_id FROM time_condition WHERE id = ? AND tariff_id = ?',
            [$id, $tariffId],
        );
        $n = $this->db->execute('DELETE FROM time_condition WHERE id = ? AND tariff_id = ?', [$id, $tariffId]);
        if ($n > 0 && $deffId !== null) {
            $this->db->execute('DELETE FROM time_condition_deff WHERE id = ?', [(int) $deffId]);
        }

        return $n;
    }
}
