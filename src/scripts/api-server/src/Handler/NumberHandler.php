<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Exception\InUseException;
use RateEngine\RE7\Repository\BillPlanRepository;
use RateEngine\RE7\Repository\CallingNumberRepository;

/** Calling-number operations, incl. ChangeBillPlan. */
final class NumberHandler
{
    private readonly CallingNumberRepository $numbers;

    public function __construct(private readonly Db $db)
    {
        $this->numbers = new CallingNumberRepository($db);
    }

    /** GET /numbers/{number} */
    public function get(Request $req, array $params): Response
    {
        $row = $this->numbers->get($params['number'] ?? '');
        if ($row === null) {
            throw new ApiException(404, 'number not found');
        }

        return Response::json($row);
    }

    /** PATCH /numbers/{number}/bill-plan { bill_plan } — ChangeBillPlan (guarded). */
    public function changeBillPlan(Request $req, array $params): Response
    {
        $number = $params['number'] ?? '';
        $plan = trim((string) ($req->json()['bill_plan'] ?? ''));
        if ($plan === '') {
            throw new ApiException(422, 'bill_plan is required');
        }
        if ($this->numbers->findId($number) === null) {
            throw new ApiException(404, 'number not found');
        }
        $billPlanId = (new BillPlanRepository($this->db))->findId($plan)
            ?? throw new ApiException(404, 'unknown bill_plan: ' . $plan);

        try {
            $this->db->transaction(fn (Db $db) => $this->numbers->changeBillPlan($number, $billPlanId));
        } catch (InUseException $e) {
            throw new ApiException(409, $e->getMessage());
        }

        return Response::json(['number' => $number, 'bill_plan' => $plan]);
    }
}
