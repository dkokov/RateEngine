<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Repository\BillingAccountRepository;
use RateEngine\RE7\Repository\ReportRepository;

/** Read-only reports (GetRatedCallsReport). */
final class ReportHandler
{
    public function __construct(private readonly Db $db)
    {
    }

    /** GET /reports/rated-calls?account=USERNAME&from=YYYY-MM-DD&to=YYYY-MM-DD&limit=N */
    public function ratedCalls(Request $req, array $params): Response
    {
        $username = trim((string) ($req->query('account') ?? ''));
        if ($username === '') {
            throw new ApiException(422, 'account query parameter is required');
        }
        $accountId = (new BillingAccountRepository($this->db))->findId($username)
            ?? throw new ApiException(404, 'account not found: ' . $username);

        $from = $req->query('from');
        $to = $req->query('to');
        $limit = max(1, min((int) ($req->query('limit') ?? 1000), 5000));

        $repo = new ReportRepository($this->db);

        return Response::json([
            'account' => $username,
            'from' => $from,
            'to' => $to,
            'summary' => $repo->summary($accountId, $from, $to),
            'calls' => $repo->ratedCalls($accountId, $from, $to, $limit),
        ]);
    }
}
