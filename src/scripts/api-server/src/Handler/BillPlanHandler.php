<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Repository\BillPlanRepository;

/** Step-by-step bill plan operations on the RE7 billing DB (via re7-lib). */
final class BillPlanHandler
{
    private readonly BillPlanRepository $repo;

    public function __construct(private readonly Db $db)
    {
        $this->repo = new BillPlanRepository($db);
    }

    /** POST /bill-plans { name, type?, start_period?, end_period? } */
    public function create(Request $req, array $params): Response
    {
        $body = $req->json();
        $name = trim((string) ($body['name'] ?? ''));
        if ($name === '') {
            throw new ApiException(422, 'name is required');
        }
        $type = (string) ($body['type'] ?? 'postpaid');
        $start = (int) ($body['start_period'] ?? 0);
        $end = (int) ($body['end_period'] ?? 0);

        $existed = $this->repo->findId($name) !== null;
        $id = $this->db->transaction(
            fn (Db $db) => $this->repo->getOrCreate($name, $type, $start, $end),
        );

        return Response::json(
            ['id' => $id, 'name' => $name, 'created' => !$existed],
            $existed ? 200 : 201,
        );
    }

    /** GET /bill-plans/{name} */
    public function get(Request $req, array $params): Response
    {
        $name = $params['name'] ?? '';
        $id = $this->repo->findId($name);
        if ($id === null) {
            throw new ApiException(404, 'bill_plan not found: ' . $name);
        }

        return Response::json(['id' => $id, 'name' => $name]);
    }
}
