<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Repository\TariffRepository;

/** Tariff create/get. */
final class TariffHandler
{
    private readonly TariffRepository $repo;

    public function __construct(private readonly Db $db)
    {
        $this->repo = new TariffRepository($db);
    }

    /** POST /tariffs { name, start_period?, end_period?, free_billsec_id? } */
    public function create(Request $req, array $params): Response
    {
        $body = $req->json();
        $name = trim((string) ($body['name'] ?? ''));
        if ($name === '') {
            throw new ApiException(422, 'name is required');
        }

        $existed = $this->repo->findId($name) !== null;
        $id = $this->db->transaction(fn (Db $db) => $this->repo->getOrCreate(
            $name,
            (int) ($body['start_period'] ?? 0),
            (int) ($body['end_period'] ?? 0),
            (int) ($body['free_billsec_id'] ?? 0),
        ));

        return Response::json(['id' => $id, 'name' => $name, 'created' => !$existed], $existed ? 200 : 201);
    }

    /** GET /tariffs/{name} */
    public function get(Request $req, array $params): Response
    {
        $name = $params['name'] ?? '';
        $id = $this->repo->findId($name);
        if ($id === null) {
            throw new ApiException(404, 'tariff not found: ' . $name);
        }

        return Response::json(['id' => $id, 'name' => $name]);
    }
}
