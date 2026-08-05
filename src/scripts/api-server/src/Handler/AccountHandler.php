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
use RateEngine\RE7\Repository\BillingAccountRepository;
use RateEngine\RE7\Repository\BillPlanRepository;
use RateEngine\RE7\Repository\CallingNumberRepository;
use RateEngine\RE7\Repository\LookupRepository;

/** Granular billing-account operations (create/list/get/update, add number). */
final class AccountHandler
{
    private readonly BillingAccountRepository $accounts;
    private readonly LookupRepository $lookup;

    public function __construct(private readonly Db $db)
    {
        $this->accounts = new BillingAccountRepository($db);
        $this->lookup = new LookupRepository($db);
    }

    /** POST /accounts { username, currency?, leg?, cdr_server_id?, billing_day?, round_mode?, day_of_payment? } */
    public function create(Request $req, array $params): Response
    {
        $body = $req->json();
        $username = trim((string) ($body['username'] ?? ''));
        if ($username === '') {
            throw new ApiException(422, 'username is required');
        }

        $existed = $this->accounts->findId($username) !== null;
        $opts = $this->opts($body);
        $id = $this->db->transaction(fn (Db $db) => $this->accounts->getOrCreate($username, $opts));

        return Response::json(['id' => $id, 'username' => $username, 'created' => !$existed], $existed ? 200 : 201);
    }

    /** GET /accounts */
    public function list(Request $req, array $params): Response
    {
        return Response::json(['accounts' => $this->accounts->list()]);
    }

    /** GET /accounts/{username} */
    public function get(Request $req, array $params): Response
    {
        $row = $this->accounts->get($params['username'] ?? '');
        if ($row === null) {
            throw new ApiException(404, 'account not found');
        }

        return Response::json($row);
    }

    /** PATCH /accounts/{username} — ChangeBillingAccount (guarded). */
    public function update(Request $req, array $params): Response
    {
        $username = $params['username'] ?? '';
        try {
            $ok = $this->db->transaction(fn (Db $db) => $this->accounts->update($username, $this->opts($req->json())));
        } catch (InUseException $e) {
            throw new ApiException(409, $e->getMessage());
        }
        if (!$ok) {
            throw new ApiException(404, 'account not found: ' . $username);
        }

        return Response::json(['username' => $username, 'updated' => true]);
    }

    /** DELETE /accounts/{username} — FK-safe teardown (same as DeleteService). */
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

    /** POST /accounts/{username}/numbers { number, bill_plan, sm_bill_plan? } */
    public function addNumber(Request $req, array $params): Response
    {
        $accountId = $this->accounts->findId($params['username'] ?? '')
            ?? throw new ApiException(404, 'account not found');

        $body = $req->json();
        $number = trim((string) ($body['number'] ?? ''));
        $plan = trim((string) ($body['bill_plan'] ?? ''));
        if ($number === '' || $plan === '') {
            throw new ApiException(422, 'number and bill_plan are required');
        }

        $billPlans = new BillPlanRepository($this->db);
        $billPlanId = $billPlans->findId($plan) ?? throw new ApiException(404, 'unknown bill_plan: ' . $plan);
        $smId = 0;
        if (!empty($body['sm_bill_plan'])) {
            $smId = $billPlans->findId((string) $body['sm_bill_plan'])
                ?? throw new ApiException(404, 'unknown sm_bill_plan: ' . $body['sm_bill_plan']);
        }

        $numbers = new CallingNumberRepository($this->db);
        $result = $this->db->transaction(function (Db $db) use ($numbers, $number, $accountId, $billPlanId, $smId): array {
            $numberId = $numbers->getOrCreate($number, $accountId);

            return [
                'calling_number_id' => $numberId,
                'calling_number_deff_id' => $numbers->deffGetOrCreate($numberId, $billPlanId, $smId),
            ];
        });

        return Response::json($result, 201);
    }

    /** Resolve currency/round_mode names to ids; pass through the rest. */
    private function opts(array $body): array
    {
        $opts = [];
        foreach (['leg', 'cdr_server_id', 'billing_day', 'day_of_payment'] as $k) {
            if (array_key_exists($k, $body)) {
                $opts[$k] = $body[$k];
            }
        }
        if (!empty($body['currency'])) {
            $opts['currency_id'] = $this->lookup->currencyId((string) $body['currency'])
                ?? throw new ApiException(422, 'unknown currency: ' . $body['currency']);
        }
        if (!empty($body['round_mode'])) {
            $opts['round_mode_id'] = $this->lookup->roundModeId((string) $body['round_mode'])
                ?? throw new ApiException(422, 'unknown round_mode: ' . $body['round_mode']);
        }

        return $opts;
    }
}
