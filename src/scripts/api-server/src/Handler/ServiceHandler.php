<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Exception\InUseException;
use RateEngine\RE7\Exception\ProvisioningException;
use RateEngine\RE7\Provisioning\ServiceProvisioner;
use RateEngine\RE7\Repository\BalanceRepository;
use RateEngine\RE7\Repository\BillingAccountRepository;
use RateEngine\RE7\Repository\PcardRepository;

/**
 * Composite "service" operations (CreateService / CheckService):
 * one call provisions billing_account + calling_number(+bill plan) + optional
 * prepaid card + optional initial balance.
 */
final class ServiceHandler
{
    public function __construct(private readonly Db $db)
    {
    }

    /**
     * POST /services
     * { username, number, bill_plan, [currency,leg,billing_day,sm_bill_plan,
     *   pcard:{amount,status,type,...}, balance:{amount}] }
     */
    public function create(Request $req, array $params): Response
    {
        try {
            $result = (new ServiceProvisioner($this->db))->createService($req->json());
        } catch (ProvisioningException $e) {
            throw new ApiException(422, $e->getMessage());
        }

        return Response::json($result, 201);
    }

    /** GET /services/{username} — aggregated view (CheckService). */
    public function get(Request $req, array $params): Response
    {
        $username = $params['username'] ?? '';

        $account = (new BillingAccountRepository($this->db))->get($username);
        if ($account === null) {
            throw new ApiException(404, 'service not found: ' . $username);
        }

        $accountId = (int) $account['id'];
        $numbers = $this->db->all(
            'SELECT cn.calling_number, bp.name AS bill_plan
             FROM calling_number cn
             LEFT JOIN calling_number_deff d ON d.calling_number_id = cn.id
             LEFT JOIN bill_plan bp ON bp.id = d.bill_plan_id
             WHERE cn.billing_account_id = ? ORDER BY cn.calling_number',
            [$accountId],
        );

        return Response::json([
            'account' => $account,
            'numbers' => $numbers,
            'balance' => (new BalanceRepository($this->db))->currentAmount($accountId),
            'pcards' => (new PcardRepository($this->db))->findByAccount($accountId),
        ]);
    }

    /** DELETE /services/{username} — guarded FK-safe teardown (DeleteService). */
    public function delete(Request $req, array $params): Response
    {
        $username = $params['username'] ?? '';
        try {
            $deleted = (new ServiceProvisioner($this->db))->deleteService($username);
        } catch (ProvisioningException $e) {
            throw new ApiException(404, $e->getMessage());
        } catch (InUseException $e) {
            throw new ApiException(409, $e->getMessage());
        }

        return Response::json(['deleted' => $deleted]);
    }
}
