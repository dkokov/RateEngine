<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Http;

use RateEngine\RE7\Api\Auth\AuthMiddleware;
use RateEngine\RE7\Api\Exception\ApiException;
use Throwable;

/**
 * Tiny method+path router. Routes with a required scope are authenticated and
 * scope-checked via AuthMiddleware before the handler runs.
 */
final class Router
{
    /** @var list<array{method:string,regex:string,handler:callable,scope:?string}> */
    private array $routes = [];

    /** @var null|callable(Request,Response):void */
    private $auditor = null;

    public function __construct(private readonly AuthMiddleware $auth)
    {
    }

    /**
     * @param callable(Request,array<string,string>):Response $handler
     */
    public function add(string $method, string $pattern, callable $handler, ?string $scope = null): void
    {
        $path = rtrim($pattern, '/');
        if ($path === '') {
            $path = '/';
        }
        $regex = '#^' . preg_replace('#\{(\w+)\}#', '(?<$1>[^/]+)', $path) . '$#';

        $this->routes[] = [
            'method' => strtoupper($method),
            'regex' => $regex,
            'handler' => $handler,
            'scope' => $scope,
        ];
    }

    /** Optional hook run after a handled request (for audit logging). */
    public function onHandled(callable $auditor): void
    {
        $this->auditor = $auditor;
    }

    public function dispatch(Request $req): Response
    {
        $path = rtrim($req->path, '/') ?: '/';
        $methodMismatch = false;

        foreach ($this->routes as $route) {
            if (preg_match($route['regex'], $path, $m) !== 1) {
                continue;
            }
            if ($route['method'] !== $req->method) {
                $methodMismatch = true;
                continue;
            }

            $params = array_filter($m, 'is_string', ARRAY_FILTER_USE_KEY);
            $response = $this->handle($route, $req, $params);

            if ($this->auditor !== null) {
                ($this->auditor)($req, $response);
            }

            return $response;
        }

        return $methodMismatch
            ? Response::problem(405, 'method not allowed')
            : Response::problem(404, 'not found');
    }

    private function handle(array $route, Request $req, array $params): Response
    {
        try {
            if ($route['scope'] !== null) {
                $req->claims = $this->auth->authenticate($req);
                $this->auth->requireScope($req->claims, $route['scope']);
            }

            return ($route['handler'])($req, $params);
        } catch (ApiException $e) {
            return Response::problem($e->status, $e->getMessage());
        } catch (Throwable $e) {
            error_log('re7-api: ' . $e->getMessage());

            return Response::problem(500, 'internal error');
        }
    }
}
