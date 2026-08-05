<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Repository\FreeBillsecRepository;

/** free_billsec values (referenced by tariff.free_billsec_id). */
final class FreeBillsecHandler
{
    private readonly FreeBillsecRepository $repo;

    public function __construct(private readonly Db $db)
    {
        $this->repo = new FreeBillsecRepository($db);
    }

    /** POST /free-billsec { free_billsec } */
    public function create(Request $req, array $params): Response
    {
        $body = $req->json();
        if (!isset($body['free_billsec'])) {
            throw new ApiException(422, 'free_billsec is required');
        }
        $seconds = (int) $body['free_billsec'];
        $id = $this->db->transaction(fn (Db $db) => $this->repo->getOrCreate($seconds));

        return Response::json(['id' => $id, 'free_billsec' => $seconds], 201);
    }

    /** GET /free-billsec */
    public function list(Request $req, array $params): Response
    {
        return Response::json(['free_billsec' => $this->repo->list()]);
    }
}
