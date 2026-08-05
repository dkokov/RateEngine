<?php

/**
 * Manage RE7 API users in the SQLite auth store.
 *
 *   php bin/re7-api-user.php init                          # create/upgrade schema
 *   php bin/re7-api-user.php add <username> [--role admin]  # prompts for password
 *
 * Roles: admin | operator | readonly (seeded by sql/api_auth.sql).
 */

declare(strict_types=1);

require dirname(__DIR__) . '/vendor/autoload.php';

use Dotenv\Dotenv;
use RateEngine\RE7\Api\Db\AuthDb;
use RateEngine\RE7\Api\Db\UserRepository;

$root = dirname(__DIR__);
if (is_file($root . '/.env')) {
    Dotenv::createUnsafeImmutable($root)->safeLoad();
}

$argv = $_SERVER['argv'];
$cmd = $argv[1] ?? '';

$dbPath = getenv('API_AUTH_DB') ?: './data/auth.sqlite';
if (!str_starts_with($dbPath, '/')) {
    $dbPath = $root . '/' . ltrim($dbPath, './');
}
$db = new AuthDb($dbPath);

$roles = ['admin' => 1, 'operator' => 2, 'readonly' => 3];

switch ($cmd) {
    case 'init':
        $db->initSchema($root . '/sql/api_auth.sql');
        fwrite(STDOUT, "auth schema ready: {$dbPath}\n");
        break;

    case 'add':
        $username = $argv[2] ?? '';
        if ($username === '') {
            fwrite(STDERR, "usage: re7-api-user.php add <username> [--role admin|operator|readonly]\n");
            exit(2);
        }
        $role = 'operator';
        foreach ($argv as $i => $a) {
            if ($a === '--role' && isset($argv[$i + 1])) {
                $role = $argv[$i + 1];
            }
        }
        if (!isset($roles[$role])) {
            fwrite(STDERR, "unknown role: {$role}\n");
            exit(2);
        }

        $db->initSchema($root . '/sql/api_auth.sql'); // ensure schema exists
        $password = promptPassword("password for {$username}: ");
        if (strlen($password) < 8) {
            fwrite(STDERR, "password too short (min 8)\n");
            exit(2);
        }

        $id = (new UserRepository($db))->create($username, $password, $roles[$role]);
        fwrite(STDOUT, "created user '{$username}' (id={$id}, role={$role})\n");
        break;

    default:
        fwrite(STDERR, "commands: init | add <username> [--role ...]\n");
        exit(2);
}

function promptPassword(string $prompt): string
{
    fwrite(STDOUT, $prompt);
    if (function_exists('shell_exec') && stripos(PHP_OS, 'WIN') === false) {
        @shell_exec('stty -echo 2>/dev/null');
        $pw = rtrim((string) fgets(STDIN), "\n");
        @shell_exec('stty echo 2>/dev/null');
        fwrite(STDOUT, "\n");

        return $pw;
    }

    return rtrim((string) fgets(STDIN), "\n");
}
