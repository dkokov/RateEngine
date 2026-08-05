<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Repository\ReferenceRepository;

/** Read-only reference-data lists (currencies, pcard types/statuses, …). */
final class ReferenceHandler
{
    public function __construct(private readonly Db $db)
    {
    }

    /** GET /ref/{resource} */
    public function get(Request $req, array $params): Response
    {
        $resource = $params['resource'] ?? '';
        $repo = new ReferenceRepository($this->db);

        $data = match ($resource) {
            'currencies' => $repo->currencies(),
            'bill-plan-types' => $repo->billPlanTypes(),
            'pcard-types' => $repo->pcardTypes(),
            'pcard-statuses' => $repo->pcardStatuses(),
            'round-modes' => $repo->roundModes(),
            'rating-modes' => $repo->ratingModes(),
            default => throw new ApiException(404, 'unknown reference: ' . $resource),
        };

        return Response::json([$resource => $data]);
    }
}
