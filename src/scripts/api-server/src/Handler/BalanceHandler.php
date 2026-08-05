<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Repository\BalanceRepository;
use RateEngine\RE7\Repository\BillingAccountRepository;

/** Balance read (CheckUserBalance). Money is engine-owned. */
final class BalanceHandler
{
    public function __construct(private readonly Db $db)
    {
    }

    /** GET /accounts/{username}/balance */
    public function get(Request $req, array $params): Response
    {
        $username = $params['username'] ?? '';
        $accountId = (new BillingAccountRepository($this->db))->findId($username)
            ?? throw new ApiException(404, 'account not found: ' . $username);

        $balances = new BalanceRepository($this->db);

        return Response::json([
            'username' => $username,
            'amount' => $balances->currentAmount($accountId),
            'balances' => $balances->findByAccount($accountId),
        ]);
    }
}
