<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Http;

/** A JSON HTTP response value object. */
final class Response
{
    public function __construct(
        public readonly int $status = 200,
        public readonly array $body = [],
        public readonly array $headers = [],
    ) {
    }

    public static function json(array $body, int $status = 200): self
    {
        return new self($status, $body);
    }

    /** RFC7807-ish error envelope. */
    public static function problem(int $status, string $title, ?string $detail = null): self
    {
        return new self($status, array_filter([
            'error' => $title,
            'status' => $status,
            'detail' => $detail,
        ], static fn ($v) => $v !== null));
    }

    public function send(): void
    {
        http_response_code($this->status);
        header('Content-Type: application/json');
        header('X-Content-Type-Options: nosniff');
        foreach ($this->headers as $name => $value) {
            header($name . ': ' . $value);
        }
        echo json_encode($this->body, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE) . "\n";
    }
}
