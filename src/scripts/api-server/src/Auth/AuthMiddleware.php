<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Auth;

use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;

/** Validates the Bearer access token and enforces per-route scopes. */
final class AuthMiddleware
{
    public function __construct(private readonly TokenService $tokens)
    {
    }

    /** @return array<string,mixed> validated claims */
    public function authenticate(Request $req): array
    {
        $jwt = $req->bearerToken();
        if ($jwt === null) {
            throw new ApiException(401, 'missing bearer token');
        }

        return $this->tokens->validateAccess($jwt);
    }

    /** @param array<string,mixed> $claims */
    public function requireScope(array $claims, string $scope): void
    {
        $scopes = $claims['scopes'] ?? '';
        $list = is_array($scopes) ? $scopes : explode(' ', (string) $scopes);

        if (!in_array($scope, $list, true)) {
            throw new ApiException(403, 'missing scope: ' . $scope);
        }
    }
}
