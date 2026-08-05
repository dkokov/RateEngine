<?php

declare(strict_types=1);

namespace RateEngine\RE7;

/**
 * RE7 database connection config, sourced from environment variables.
 *
 * Recognised vars: RE7_DB_HOST, RE7_DB_PORT, RE7_DB_NAME, RE7_DB_USER,
 * RE7_DB_PASS. See .env.example.
 */
final class Config
{
    public function __construct(
        public readonly string $host,
        public readonly int $port,
        public readonly string $dbname,
        public readonly string $user,
        public readonly string $password,
    ) {
    }

    /**
     * Build from environment. If $envFile is given and exists, it is loaded
     * first (simple KEY=VALUE lines) without overwriting already-set vars.
     */
    public static function fromEnv(?string $envFile = null): self
    {
        if ($envFile !== null && is_file($envFile)) {
            self::loadEnvFile($envFile);
        }

        return new self(
            self::env('RE7_DB_HOST', '127.0.0.1'),
            (int) self::env('RE7_DB_PORT', '5432'),
            self::env('RE7_DB_NAME', 'rate_engine'),
            self::env('RE7_DB_USER', 'global'),
            self::env('RE7_DB_PASS', ''),
        );
    }

    /** PDO DSN for pgsql. */
    public function dsn(): string
    {
        return sprintf('pgsql:host=%s;port=%d;dbname=%s', $this->host, $this->port, $this->dbname);
    }

    private static function env(string $key, string $default): string
    {
        $v = getenv($key);

        return ($v === false || $v === '') ? $default : $v;
    }

    private static function loadEnvFile(string $path): void
    {
        $lines = file($path, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
        if ($lines === false) {
            return;
        }

        foreach ($lines as $line) {
            $line = trim($line);
            if ($line === '' || $line[0] === '#' || !str_contains($line, '=')) {
                continue;
            }

            [$key, $value] = explode('=', $line, 2);
            $key = trim($key);
            $value = trim($value);

            // do not clobber vars already present in the real environment
            if ($key !== '' && getenv($key) === false) {
                putenv($key . '=' . $value);
            }
        }
    }
}
