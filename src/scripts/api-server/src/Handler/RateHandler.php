<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Repository\BillPlanRepository;
use RateEngine\RE7\Repository\PrefixRepository;
use RateEngine\RE7\Repository\RateRepository;
use RateEngine\RE7\Repository\TariffRepository;

/**
 * Rate rows — the link binding a prefix to a tariff within a bill plan.
 * Step-by-step: the referenced bill_plan, prefix and tariff must already exist.
 */
final class RateHandler
{
    private readonly RateRepository $rates;

    public function __construct(private readonly Db $db)
    {
        $this->rates = new RateRepository($db);
    }

    /** POST /rates { bill_plan, prefix, tariff } */
    public function create(Request $req, array $params): Response
    {
        $body = $req->json();
        $billPlan = trim((string) ($body['bill_plan'] ?? ''));
        $prefix = trim((string) ($body['prefix'] ?? ''));
        $tariff = trim((string) ($body['tariff'] ?? ''));
        if ($billPlan === '' || $prefix === '' || $tariff === '') {
            throw new ApiException(422, 'bill_plan, prefix and tariff are required');
        }

        $billPlanId = (new BillPlanRepository($this->db))->findId($billPlan)
            ?? throw new ApiException(404, 'unknown bill_plan: ' . $billPlan);
        $prefixId = (new PrefixRepository($this->db))->findId($prefix)
            ?? throw new ApiException(404, 'unknown prefix: ' . $prefix);
        $tariffId = (new TariffRepository($this->db))->findId($tariff)
            ?? throw new ApiException(404, 'unknown tariff: ' . $tariff);

        $existed = $this->rates->findId($billPlanId, $prefixId, $tariffId) !== null;
        $id = $this->db->transaction(fn (Db $db) => $this->rates->getOrCreate($billPlanId, $prefixId, $tariffId));

        return Response::json(
            ['id' => $id, 'bill_plan' => $billPlan, 'prefix' => $prefix, 'tariff' => $tariff, 'created' => !$existed],
            $existed ? 200 : 201,
        );
    }

    /** GET /rates?bill_plan=NAME */
    public function list(Request $req, array $params): Response
    {
        $billPlan = trim((string) ($req->query('bill_plan') ?? ''));
        if ($billPlan === '') {
            throw new ApiException(422, 'bill_plan query parameter is required');
        }

        return Response::json(['bill_plan' => $billPlan, 'rates' => $this->rates->listByBillPlan($billPlan)]);
    }
}
