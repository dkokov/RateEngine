<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Handler;

use RateEngine\RE7\Api\Auth\TokenService;
use RateEngine\RE7\Api\Db\TokenRepository;
use RateEngine\RE7\Api\Db\UserRepository;
use RateEngine\RE7\Api\Exception\ApiException;
use RateEngine\RE7\Api\Http\Request;
use RateEngine\RE7\Api\Http\Response;

/** POST /auth/login, /auth/refresh, /auth/logout. */
final class AuthHandler
{
    public function __construct(
        private readonly UserRepository $users,
        private readonly TokenRepository $tokens,
        private readonly TokenService $jwt,
    ) {
    }

    public function login(Request $req, array $params): Response
    {
        $body = $req->json();
        $username = trim((string) ($body['username'] ?? ''));
        $password = (string) ($body['password'] ?? '');
        if ($username === '' || $password === '') {
            throw new ApiException(422, 'username and password are required');
        }

        $user = $this->users->authenticate($username, $password); // throws 401

        return $this->issue($user);
    }

    public function refresh(Request $req, array $params): Response
    {
        $token = (string) ($req->json()['refresh_token'] ?? '');
        if ($token === '') {
            throw new ApiException(422, 'refresh_token is required');
        }

        $hash = $this->jwt->hashRefresh($token);
        $live = $this->tokens->findLiveRefresh($hash);
        if ($live === null) {
            throw new ApiException(401, 'invalid or expired refresh token');
        }

        // rotate: revoke the presented token, issue a fresh pair
        $this->tokens->revokeByHash($hash);

        $row = $this->users->authenticateById($live['user_id']);

        return $this->issue($row);
    }

    public function logout(Request $req, array $params): Response
    {
        $token = (string) ($req->json()['refresh_token'] ?? '');
        if ($token !== '') {
            $this->tokens->revokeByHash($this->jwt->hashRefresh($token));
        }

        return Response::json(['status' => 'ok']);
    }

    /** @param array{id:int,username:string,scopes:string} $user */
    private function issue(array $user): Response
    {
        $access = $this->jwt->issueAccess($user);
        $refresh = $this->jwt->newRefresh();
        $this->tokens->storeRefresh($user['id'], $refresh['hash'], $refresh['expires_at']);

        return Response::json([
            'access_token' => $access,
            'refresh_token' => $refresh['token'],
            'token_type' => 'Bearer',
            'expires_in' => $this->jwt->accessTtl(),
            'scopes' => $user['scopes'],
        ]);
    }
}
