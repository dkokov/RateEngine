<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Repository\PrefixRepository;

/** Prefix create/get. */
final class PrefixHandler
{
    private readonly PrefixRepository $repo;

    public function __construct(private readonly Db $db)
    {
        $this->repo = new PrefixRepository($db);
    }

    /** POST /prefixes { prefix, comm? } */
    public function create(Request $req, array $params): Response
    {
        $body = $req->json();
        $prefix = trim((string) ($body['prefix'] ?? ''));
        if ($prefix === '') {
            throw new ApiException(422, 'prefix is required');
        }

        $existed = $this->repo->findId($prefix) !== null;
        $id = $this->db->transaction(
            fn (Db $db) => $this->repo->getOrCreate($prefix, isset($body['comm']) ? (string) $body['comm'] : null),
        );

        return Response::json(['id' => $id, 'prefix' => $prefix, 'created' => !$existed], $existed ? 200 : 201);
    }

    /** GET /prefixes/{prefix} */
    public function get(Request $req, array $params): Response
    {
        $prefix = $params['prefix'] ?? '';
        $id = $this->repo->findId($prefix);
        if ($id === null) {
            throw new ApiException(404, 'prefix not found: ' . $prefix);
        }

        return Response::json(['id' => $id, 'prefix' => $prefix]);
    }
}
