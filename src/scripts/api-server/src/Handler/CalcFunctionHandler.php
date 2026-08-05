<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Db;
use RateEngine\RE7\Repository\CalcFunctionRepository;
use RateEngine\RE7\Repository\TariffRepository;

/** calc_function steps for a tariff (the pricing formula). */
final class CalcFunctionHandler
{
    private readonly CalcFunctionRepository $calc;

    public function __construct(private readonly Db $db)
    {
        $this->calc = new CalcFunctionRepository($db);
    }

    /** POST /tariffs/{name}/calc-functions { pos, delta_time, fee, iterations? } */
    public function create(Request $req, array $params): Response
    {
        $tariffId = $this->tariffId($params['name'] ?? '');
        $body = $req->json();
        foreach (['pos', 'delta_time', 'fee'] as $required) {
            if (!isset($body[$required])) {
                throw new ApiException(422, $required . ' is required');
            }
        }

        $id = $this->db->transaction(fn (Db $db) => $this->calc->setPos(
            $tariffId,
            (int) $body['pos'],
            (int) $body['delta_time'],
            (string) $body['fee'],
            isset($body['iterations']) ? (int) $body['iterations'] : null,
        ));

        return Response::json(['id' => $id, 'tariff' => $params['name'], 'pos' => (int) $body['pos']], 201);
    }

    /** GET /tariffs/{name}/calc-functions */
    public function list(Request $req, array $params): Response
    {
        return Response::json([
            'tariff' => $params['name'] ?? '',
            'calc_functions' => $this->calc->listByTariff($this->tariffId($params['name'] ?? '')),
        ]);
    }

    /** DELETE /tariffs/{name}/calc-functions/{pos} */
    public function delete(Request $req, array $params): Response
    {
        $tariffId = $this->tariffId($params['name'] ?? '');
        $pos = (int) ($params['pos'] ?? -1);
        if ($this->calc->deleteByPos($tariffId, $pos) === 0) {
            throw new ApiException(404, 'no calc-function at pos ' . $pos);
        }

        return Response::json(['tariff' => $params['name'], 'pos' => $pos, 'deleted' => true]);
    }

    private function tariffId(string $name): int
    {
        return (new TariffRepository($this->db))->findId($name)
            ?? throw new ApiException(404, 'tariff not found: ' . $name);
    }
}
