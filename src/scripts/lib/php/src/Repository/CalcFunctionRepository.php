<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

/**
 * calc_function (id, tariff_id, pos, delta_time, fee, iterations) — the pricing
 * formula steps for a tariff. Keyed logically by (tariff_id, pos).
 */
final class CalcFunctionRepository extends AbstractRepository
{
    public function listByTariff(int $tariffId): array
    {
        return $this->db->all(
            'SELECT id, pos, delta_time, fee, iterations FROM calc_function WHERE tariff_id = ? ORDER BY pos',
            [$tariffId],
        );
    }

    /** Upsert the calc-function step at (tariff_id, pos). Returns its id. */
    public function setPos(int $tariffId, int $pos, int $deltaTime, string $fee, ?int $iterations): int
    {
        $id = $this->db->value(
            'SELECT id FROM calc_function WHERE tariff_id = ? AND pos = ?',
            [$tariffId, $pos],
        );

        if ($id !== null) {
            $this->db->execute(
                'UPDATE calc_function SET delta_time = ?, fee = ?, iterations = ? WHERE id = ?',
                [$deltaTime, $fee, $iterations, (int) $id],
            );

            return (int) $id;
        }

        return $this->db->insertReturningId(
            'INSERT INTO calc_function (tariff_id, pos, delta_time, fee, iterations)
             VALUES (?, ?, ?, ?, ?) RETURNING id',
            [$tariffId, $pos, $deltaTime, $fee, $iterations],
        );
    }

    public function deleteByPos(int $tariffId, int $pos): int
    {
        return $this->db->execute(
            'DELETE FROM calc_function WHERE tariff_id = ? AND pos = ?',
            [$tariffId, $pos],
        );
    }
}
