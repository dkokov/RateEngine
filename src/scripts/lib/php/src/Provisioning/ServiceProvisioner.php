<?php

declare(strict_types=1);

namespace RateEngine\RE7\Provisioning;

use RateEngine\RE7\Db;
use RateEngine\RE7\Exception\ProvisioningException;
use RateEngine\RE7\Guard\ActiveCallGate;
use RateEngine\RE7\Guard\NullActiveCallGate;
use RateEngine\RE7\Repository\BalanceRepository;
use RateEngine\RE7\Repository\BillingAccountRepository;
use RateEngine\RE7\Repository\BillPlanRepository;
use RateEngine\RE7\Repository\CallingNumberRepository;
use RateEngine\RE7\Repository\LookupRepository;
use RateEngine\RE7\Repository\PcardRepository;

/**
 * Composite provisioning — the composite CreateService as one transaction:
 * billing_account + calling_number(+deff / bill plan) + optional prepaid card
 * + optional initial balance. Idempotent on account/number (get-or-create).
 */
final class ServiceProvisioner
{
    private readonly BillingAccountRepository $accounts;
    private readonly CallingNumberRepository $numbers;
    private readonly BillPlanRepository $billPlans;
    private readonly PcardRepository $pcards;
    private readonly BalanceRepository $balances;
    private readonly LookupRepository $lookup;
    private readonly ActiveCallGate $gate;

    public function __construct(
        private readonly Db $db,
        ?ActiveCallGate $gate = null,
    ) {
        $gate ??= new NullActiveCallGate();
        $this->gate = $gate;
        $this->accounts = new BillingAccountRepository($db, $gate);
        $this->numbers = new CallingNumberRepository($db, $gate);
        $this->billPlans = new BillPlanRepository($db, $gate);
        $this->pcards = new PcardRepository($db, $gate);
        $this->balances = new BalanceRepository($db, $gate);
        $this->lookup = new LookupRepository($db, $gate);
    }

    /**
     * @param array $spec required: username, number, bill_plan
     *                    optional: currency, leg, cdr_server_id, billing_day,
     *                    round_mode, day_of_payment, sm_bill_plan,
     *                    pcard{amount,type,status,start_date,end_date,call_number,sim},
     *                    balance{amount}
     *
     * @return array ids of everything created/reused
     */
    public function createService(array $spec): array
    {
        $username = trim((string) ($spec['username'] ?? ''));
        $number = trim((string) ($spec['number'] ?? ''));
        $billPlan = trim((string) ($spec['bill_plan'] ?? ''));
        if ($username === '' || $number === '' || $billPlan === '') {
            throw new ProvisioningException('username, number and bill_plan are required');
        }

        return $this->db->transaction(function (Db $db) use ($spec, $username, $number, $billPlan): array {
            $billPlanId = $this->billPlans->findId($billPlan)
                ?? throw new ProvisioningException("unknown bill_plan: {$billPlan}");

            $smBillPlanId = 0;
            if (!empty($spec['sm_bill_plan'])) {
                $smBillPlanId = $this->billPlans->findId((string) $spec['sm_bill_plan'])
                    ?? throw new ProvisioningException('unknown sm_bill_plan: ' . $spec['sm_bill_plan']);
            }

            $accountId = $this->accounts->getOrCreate($username, $this->accountOpts($spec));
            $numberId = $this->numbers->getOrCreate($number, $accountId);
            $deffId = $this->numbers->deffGetOrCreate($numberId, $billPlanId, $smBillPlanId);

            $out = [
                'billing_account_id' => $accountId,
                'calling_number_id' => $numberId,
                'calling_number_deff_id' => $deffId,
                'bill_plan_id' => $billPlanId,
            ];

            if (isset($spec['pcard']) && is_array($spec['pcard'])) {
                $out['pcard_id'] = $this->createPcard($accountId, $spec['pcard']);
            }

            if (isset($spec['balance']['amount'])) {
                $out['balance_id'] = $this->balances->ensure(
                    $accountId,
                    (string) $spec['balance']['amount'],
                    (bool) ($spec['balance']['active'] ?? true),
                );
            }

            return $out;
        });
    }

    /**
     * DeleteService — FK-safe teardown of an account's provisioning in one
     * transaction: balance + pcard + calling_number(_deff) + billing_account.
     * Shared reference data (bill_plan, prefix, tariff) is NOT touched.
     * Guarded: refuses while any of the account's numbers has live calls.
     *
     * @return array<string,int> rows deleted per table
     */
    public function deleteService(string $username): array
    {
        return $this->db->transaction(function (Db $db) use ($username): array {
            $accountId = $this->accounts->findId($username)
                ?? throw new ProvisioningException("service not found: {$username}");

            foreach ($db->all('SELECT id FROM calling_number WHERE billing_account_id = ?', [$accountId]) as $row) {
                $this->gate->assertNotInUse('calling_number', (int) $row['id']);
            }

            return [
                'balance' => $db->execute('DELETE FROM balance WHERE billing_account_id = ?', [$accountId]),
                'pcard' => $db->execute('DELETE FROM pcard WHERE billing_account_id = ?', [$accountId]),
                'calling_number_deff' => $db->execute(
                    'DELETE FROM calling_number_deff WHERE calling_number_id IN
                        (SELECT id FROM calling_number WHERE billing_account_id = ?)',
                    [$accountId],
                ),
                'calling_number' => $db->execute('DELETE FROM calling_number WHERE billing_account_id = ?', [$accountId]),
                'billing_account' => $db->execute('DELETE FROM billing_account WHERE id = ?', [$accountId]),
            ];
        });
    }

    private function accountOpts(array $spec): array
    {
        $opts = [
            'leg' => $spec['leg'] ?? 'a',
            'cdr_server_id' => $spec['cdr_server_id'] ?? 0,
            'billing_day' => $spec['billing_day'] ?? '01',
            'day_of_payment' => $spec['day_of_payment'] ?? 0,
        ];

        if (!empty($spec['currency'])) {
            $opts['currency_id'] = $this->lookup->currencyId((string) $spec['currency'])
                ?? throw new ProvisioningException('unknown currency: ' . $spec['currency']);
        }
        if (!empty($spec['round_mode'])) {
            $opts['round_mode_id'] = $this->lookup->roundModeId((string) $spec['round_mode'])
                ?? throw new ProvisioningException('unknown round_mode: ' . $spec['round_mode']);
        }

        return $opts;
    }

    private function createPcard(int $accountId, array $pcard): int
    {
        if (!isset($pcard['amount'])) {
            throw new ProvisioningException('pcard.amount is required');
        }
        $statusId = $this->lookup->pcardStatusId((string) ($pcard['status'] ?? 'active')) ?? 1;
        $typeId = 2;
        if (!empty($pcard['type'])) {
            $typeId = $this->lookup->pcardTypeId((string) $pcard['type'])
                ?? throw new ProvisioningException('unknown pcard type: ' . $pcard['type']);
        }

        return $this->pcards->create($accountId, (string) $pcard['amount'], $statusId, $typeId, [
            'start_date' => $pcard['start_date'] ?? null,
            'end_date' => $pcard['end_date'] ?? null,
            'call_number' => $pcard['call_number'] ?? 1,
            'sim' => $pcard['sim'] ?? 0,
        ]);
    }
}
