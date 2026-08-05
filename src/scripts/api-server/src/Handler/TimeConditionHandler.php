<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Repository\TariffRepository;
use RateEngine\RE7\Repository\TimeConditionRepository;

/** Time conditions (peak/off-peak windows) on a tariff. */
final class TimeConditionHandler
{
    private readonly TimeConditionRepository $repo;

    public function __construct(private readonly Db $db)
    {
        $this->repo = new TimeConditionRepository($db);
    }

    /** POST /tariffs/{name}/time-conditions { tc_name?, hours?, days_week?, tc_date?, year?, month?, day_month?, prior? } */
    public function create(Request $req, array $params): Response
    {
        $tariffId = $this->tariffId($params['name'] ?? '');
        $body = $req->json();

        $deff = [];
        foreach (['hours', 'days_week', 'tc_name', 'tc_date', 'year', 'month', 'day_month'] as $k) {
            if (isset($body[$k])) {
                $deff[$k] = (string) $body[$k];
            }
        }
        $prior = (int) ($body['prior'] ?? 40);

        $result = $this->db->transaction(fn (Db $db) => $this->repo->create($tariffId, $prior, $deff));

        return Response::json(['tariff' => $params['name'], 'prior' => $prior] + $result, 201);
    }

    /** GET /tariffs/{name}/time-conditions */
    public function list(Request $req, array $params): Response
    {
        return Response::json([
            'tariff' => $params['name'] ?? '',
            'time_conditions' => $this->repo->listByTariff($this->tariffId($params['name'] ?? '')),
        ]);
    }

    /** DELETE /tariffs/{name}/time-conditions/{id} */
    public function delete(Request $req, array $params): Response
    {
        $tariffId = $this->tariffId($params['name'] ?? '');
        $id = (int) ($params['id'] ?? 0);
        if ($this->repo->deleteById($tariffId, $id) === 0) {
            throw new ApiException(404, 'time-condition not found: ' . $id);
        }

        return Response::json(['tariff' => $params['name'], 'id' => $id, 'deleted' => true]);
    }

    private function tariffId(string $name): int
    {
        return (new TariffRepository($this->db))->findId($name)
            ?? throw new ApiException(404, 'tariff not found: ' . $name);
    }
}
