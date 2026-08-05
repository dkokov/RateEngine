<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Exception\InUseException;
use RateEngine\RE7\Repository\BillingAccountRepository;
use RateEngine\RE7\Repository\LookupRepository;
use RateEngine\RE7\Repository\PcardRepository;

/** Prepaid-card operations: create/list + ChangePCardStatus / UpdateCreditLimit. */
final class PcardHandler
{
    private readonly PcardRepository $pcards;
    private readonly LookupRepository $lookup;

    public function __construct(private readonly Db $db)
    {
        $this->pcards = new PcardRepository($db);
        $this->lookup = new LookupRepository($db);
    }

    /** POST /accounts/{username}/pcards { amount, status?, type?, start_date?, end_date?, call_number?, sim? } */
    public function create(Request $req, array $params): Response
    {
        $accountId = $this->accountId($params['username'] ?? '');
        $body = $req->json();
        if (!isset($body['amount'])) {
            throw new ApiException(422, 'amount is required');
        }

        $statusId = $this->lookup->pcardStatusId((string) ($body['status'] ?? 'active')) ?? 1;
        $typeId = 2;
        if (!empty($body['type'])) {
            $typeId = $this->lookup->pcardTypeId((string) $body['type'])
                ?? throw new ApiException(422, 'unknown pcard type: ' . $body['type']);
        }

        $id = $this->pcards->create($accountId, (string) $body['amount'], $statusId, $typeId, [
            'start_date' => $body['start_date'] ?? null,
            'end_date' => $body['end_date'] ?? null,
            'call_number' => $body['call_number'] ?? 1,
            'sim' => $body['sim'] ?? 0,
        ]);

        return Response::json(['id' => $id], 201);
    }

    /** GET /accounts/{username}/pcards */
    public function list(Request $req, array $params): Response
    {
        return Response::json(['pcards' => $this->pcards->findByAccount($this->accountId($params['username'] ?? ''))]);
    }

    /** PATCH /pcards/{id}/status { status } */
    public function status(Request $req, array $params): Response
    {
        $id = $this->existingPcardId($params);
        $status = (string) ($req->json()['status'] ?? '');
        $statusId = $this->lookup->pcardStatusId($status)
            ?? throw new ApiException(422, 'unknown status: ' . $status);

        try {
            $this->pcards->updateStatus($id, $statusId);
        } catch (InUseException $e) {
            throw new ApiException(409, $e->getMessage());
        }

        return Response::json(['id' => $id, 'status' => $status]);
    }

    /** PATCH /pcards/{id}/limit { amount } — UpdateCreditLimit */
    public function limit(Request $req, array $params): Response
    {
        $id = $this->existingPcardId($params);
        if (!isset($req->json()['amount'])) {
            throw new ApiException(422, 'amount is required');
        }
        $amount = (string) $req->json()['amount'];

        try {
            $this->pcards->updateLimit($id, $amount);
        } catch (InUseException $e) {
            throw new ApiException(409, $e->getMessage());
        }

        return Response::json(['id' => $id, 'amount' => $amount]);
    }

    private function accountId(string $username): int
    {
        return (new BillingAccountRepository($this->db))->findId($username)
            ?? throw new ApiException(404, 'account not found: ' . $username);
    }

    private function existingPcardId(array $params): int
    {
        $id = (int) ($params['id'] ?? 0);
        if ($this->pcards->get($id) === null) {
            throw new ApiException(404, 'pcard not found: ' . $id);
        }

        return $id;
    }
}
