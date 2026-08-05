<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Auth;

use Firebase\JWT\JWT;
use Firebase\JWT\Key;
use RateEngine\RE7\Api\Exception\ApiException;
use Throwable;

/**
 * Issues/validates short-lived access JWTs and generates opaque refresh tokens.
 * The refresh token itself is random; only its sha256 is stored (in api_token).
 */
final class TokenService
{
    public function __construct(
        private readonly string $secret,
        private readonly string $alg,
        private readonly int $accessTtl,
        private readonly int $refreshTtl,
    ) {
    }

    /** @param array<string,mixed> $user  keys: id, username, scopes */
    public function issueAccess(array $user): string
    {
        $now = time();
        $payload = [
            'iat' => $now,
            'exp' => $now + $this->accessTtl,
            'sub' => $user['id'],
            'username' => $user['username'],
            'scopes' => $user['scopes'],
        ];

        return JWT::encode($payload, $this->secret, $this->alg);
    }

    /** @return array{token:string,hash:string,expires_at:int} */
    public function newRefresh(): array
    {
        $token = bin2hex(random_bytes(32));

        return [
            'token' => $token,
            'hash' => hash('sha256', $token),
            'expires_at' => time() + $this->refreshTtl,
        ];
    }

    public function hashRefresh(string $token): string
    {
        return hash('sha256', $token);
    }

    public function accessTtl(): int
    {
        return $this->accessTtl;
    }

    /** @return array<string,mixed> decoded claims */
    public function validateAccess(string $jwt): array
    {
        try {
            $decoded = JWT::decode($jwt, new Key($this->secret, $this->alg));
        } catch (Throwable $e) {
            throw new ApiException(401, 'invalid or expired token');
        }

        return json_decode(json_encode($decoded), true);
    }
}
