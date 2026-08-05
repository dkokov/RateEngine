<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Http;

use RateEngine\RE7\Api\Exception\ApiException;

/** Immutable view of the incoming HTTP request. */
final class Request
{
    /** claims from a validated JWT, populated by AuthMiddleware */
    public array $claims = [];

    /**
     * @param array<string,string> $headers lower-cased header name => value
     */
    public function __construct(
        public readonly string $method,
        public readonly string $path,
        public readonly string $remoteIp,
        private readonly array $headers,
        private readonly string $rawBody,
        private readonly array $query = [],
    ) {
    }

    public static function fromGlobals(): self
    {
        $method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
        $path = parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH) ?: '/';
        $ip = $_SERVER['REMOTE_ADDR'] ?? '';

        return new self(
            strtoupper($method),
            $path,
            $ip,
            self::readHeaders(),
            file_get_contents('php://input') ?: '',
            $_GET,
        );
    }

    /** A query-string parameter (?key=value), or null. */
    public function query(string $key): ?string
    {
        $v = $this->query[$key] ?? null;

        return $v === null ? null : (string) $v;
    }

    public function header(string $name): ?string
    {
        return $this->headers[strtolower($name)] ?? null;
    }

    public function bearerToken(): ?string
    {
        $auth = $this->header('authorization');
        if ($auth !== null && preg_match('/^Bearer\s+(.+)$/i', $auth, $m) === 1) {
            return trim($m[1]);
        }

        return null;
    }

    /** Decode a JSON body into an array; empty body => []. */
    public function json(): array
    {
        if (trim($this->rawBody) === '') {
            return [];
        }

        $data = json_decode($this->rawBody, true);
        if (!is_array($data)) {
            throw new ApiException(400, 'invalid JSON body');
        }

        return $data;
    }

    /** @return array<string,string> */
    private static function readHeaders(): array
    {
        $out = [];
        if (function_exists('getallheaders')) {
            foreach (getallheaders() as $k => $v) {
                $out[strtolower($k)] = $v;
            }

            return $out;
        }

        foreach ($_SERVER as $k => $v) {
            if (str_starts_with($k, 'HTTP_')) {
                $name = strtolower(str_replace('_', '-', substr($k, 5)));
                $out[$name] = $v;
            }
        }

        return $out;
    }
}
