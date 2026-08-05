<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api;

use Dotenv\Dotenv;
use RateEngine\RE7\Api\Auth\AuthMiddleware;
use RateEngine\RE7\Api\Auth\TokenService;
use RateEngine\RE7\Api\Db\AuthDb;
use RateEngine\RE7\Api\Db\TokenRepository;
use RateEngine\RE7\Api\Db\UserRepository;
use RateEngine\RE7\Api\Handler\AccountHandler;
use RateEngine\RE7\Api\Handler\AuthHandler;
use RateEngine\RE7\Api\Handler\BalanceHandler;
use RateEngine\RE7\Api\Handler\BillPlanHandler;
use RateEngine\RE7\Api\Handler\CalcFunctionHandler;
use RateEngine\RE7\Api\Handler\NumberHandler;
use RateEngine\RE7\Api\Handler\PcardHandler;
use RateEngine\RE7\Api\Handler\PrefixHandler;
use RateEngine\RE7\Api\Handler\RateHandler;
use RateEngine\RE7\Api\Handler\ReferenceHandler;
use RateEngine\RE7\Api\Handler\ReportHandler;
use RateEngine\RE7\Api\Handler\ServiceHandler;
use RateEngine\RE7\Api\Handler\TariffHandler;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;
use RateEngine\RE7\Api\Http\Router;
use RateEngine\RE7\Config;
use RateEngine\RE7\Db;

/** Wires config, stores, auth and routes, then dispatches a request. */
final class Bootstrap
{
    private function __construct(
        private readonly Router $router,
        private readonly TokenRepository $tokens,
    ) {
    }

    public static function create(string $root): self
    {
        if (is_file($root . '/.env')) {
            Dotenv::createUnsafeImmutable($root)->safeLoad();
        }

        // auth store (SQLite, local)
        $authDb = new AuthDb(self::resolvePath($root, getenv('API_AUTH_DB') ?: './data/auth.sqlite'));
        $users = new UserRepository($authDb);
        $tokenRepo = new TokenRepository($authDb);

        // RE7 billing DB (remote, via re7-lib)
        $re7 = new Db(Config::fromEnv());

        $jwt = new TokenService(
            (string) (getenv('JWT_SECRET') ?: ''),
            (string) (getenv('JWT_ALG') ?: 'HS256'),
            (int) (getenv('JWT_ACCESS_TTL') ?: 900),
            (int) (getenv('JWT_REFRESH_TTL') ?: 28800),
        );

        $router = new Router(new AuthMiddleware($jwt));

        $auth = new AuthHandler($users, $tokenRepo, $jwt);
        $billPlan = new BillPlanHandler($re7);
        $tariff = new TariffHandler($re7);
        $prefix = new PrefixHandler($re7);
        $rate = new RateHandler($re7);
        $service = new ServiceHandler($re7);
        $number = new NumberHandler($re7);
        $pcard = new PcardHandler($re7);
        $balance = new BalanceHandler($re7);
        $report = new ReportHandler($re7);
        $account = new AccountHandler($re7);
        $calc = new CalcFunctionHandler($re7);
        $ref = new ReferenceHandler($re7);

        // public
        $router->add('GET', '/health', static fn (Request $r, array $p) => Response::json(['status' => 'ok']));
        $router->add('POST', '/auth/login', [$auth, 'login']);
        $router->add('POST', '/auth/refresh', [$auth, 'refresh']);
        $router->add('POST', '/auth/logout', [$auth, 'logout']);

        // provisioning (scope-guarded)
        $router->add('POST', '/bill-plans', [$billPlan, 'create'], 'provisioning:write');
        $router->add('GET', '/bill-plans/{name}', [$billPlan, 'get'], 'provisioning:read');

        // rate-plan definition
        $router->add('POST', '/tariffs', [$tariff, 'create'], 'provisioning:write');
        $router->add('GET', '/tariffs/{name}', [$tariff, 'get'], 'provisioning:read');
        $router->add('POST', '/tariffs/{name}/calc-functions', [$calc, 'create'], 'provisioning:write');
        $router->add('GET', '/tariffs/{name}/calc-functions', [$calc, 'list'], 'provisioning:read');
        $router->add('DELETE', '/tariffs/{name}/calc-functions/{pos}', [$calc, 'delete'], 'provisioning:write');
        $router->add('POST', '/prefixes', [$prefix, 'create'], 'provisioning:write');
        $router->add('GET', '/prefixes/{prefix}', [$prefix, 'get'], 'provisioning:read');
        $router->add('POST', '/rates', [$rate, 'create'], 'provisioning:write');
        $router->add('GET', '/rates', [$rate, 'list'], 'provisioning:read');

        // billing accounts (granular)
        $router->add('POST', '/accounts', [$account, 'create'], 'provisioning:write');
        $router->add('GET', '/accounts', [$account, 'list'], 'provisioning:read');
        $router->add('GET', '/accounts/{username}', [$account, 'get'], 'provisioning:read');
        $router->add('PATCH', '/accounts/{username}', [$account, 'update'], 'provisioning:write');
        $router->add('POST', '/accounts/{username}/numbers', [$account, 'addNumber'], 'provisioning:write');

        // reference data
        $router->add('GET', '/ref/{resource}', [$ref, 'get'], 'provisioning:read');

        // composite service (CreateService / CheckService / DeleteService)
        $router->add('POST', '/services', [$service, 'create'], 'provisioning:write');
        $router->add('GET', '/services/{username}', [$service, 'get'], 'provisioning:read');
        $router->add('DELETE', '/services/{username}', [$service, 'delete'], 'provisioning:write');

        // calling numbers
        $router->add('GET', '/numbers/{number}', [$number, 'get'], 'provisioning:read');
        $router->add('PATCH', '/numbers/{number}/bill-plan', [$number, 'changeBillPlan'], 'provisioning:write');

        // prepaid cards
        $router->add('POST', '/accounts/{username}/pcards', [$pcard, 'create'], 'provisioning:write');
        $router->add('GET', '/accounts/{username}/pcards', [$pcard, 'list'], 'provisioning:read');
        $router->add('PATCH', '/pcards/{id}/status', [$pcard, 'status'], 'provisioning:write');
        $router->add('PATCH', '/pcards/{id}/limit', [$pcard, 'limit'], 'provisioning:write');

        // balance (read)
        $router->add('GET', '/accounts/{username}/balance', [$balance, 'get'], 'provisioning:read');

        // reports (read-only)
        $router->add('GET', '/reports/rated-calls', [$report, 'ratedCalls'], 'rating:read');

        // audit every write
        $router->onHandled(static function (Request $req, Response $res) use ($tokenRepo): void {
            if ($req->method === 'GET') {
                return;
            }
            $tokenRepo->audit(
                isset($req->claims['sub']) ? (int) $req->claims['sub'] : null,
                $req->claims['username'] ?? null,
                $req->method,
                $req->path,
                $res->status,
                $req->remoteIp,
            );
        });

        return new self($router, $tokenRepo);
    }

    public function run(): void
    {
        $this->router->dispatch(Request::fromGlobals())->send();
    }

    private static function resolvePath(string $root, string $path): string
    {
        return str_starts_with($path, '/') ? $path : $root . '/' . ltrim($path, './');
    }
}
