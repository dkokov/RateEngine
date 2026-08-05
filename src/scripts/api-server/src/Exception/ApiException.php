<?php

declare(strict_types=1);

namespace RateEngine\RE7\Api\Exception;

use RuntimeException;
use Throwable;

/** An error carrying the HTTP status to return. */
final class ApiException extends RuntimeException
{
    public function __construct(
        public readonly int $status,
        string $message,
        ?Throwable $previous = null,
    ) {
        parent::__construct($message, 0, $previous);
    }
}
