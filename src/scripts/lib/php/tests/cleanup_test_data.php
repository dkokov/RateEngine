<?php

/**
 * Remove synthetic TEST_* provisioning rows created by run_import.php --commit.
 *
 * DRY-RUN by default (shows counts, rolls back). Pass --commit to delete.
 * FK-safe order: rate -> tariff -> bill_plan. Shared prefixes are left intact
 * (they are reference data; e.g. "3598" may be reused elsewhere).
 *
 *   php tests/cleanup_test_data.php            # preview
 *   php tests/cleanup_test_data.php --commit   # delete
 */

declare(strict_types=1);

$root = dirname(__DIR__);

spl_autoload_register(static function (string $class) use ($root): void {
    $prefix = 'RateEngine\\RE7\\';
    if (str_starts_with($class, $prefix)) {
        require $root . '/src/' . str_replace('\\', '/', substr($class, strlen($prefix))) . '.php';
    }
});

use RateEngine\RE7\Config;
use RateEngine\RE7\Db;

$commit = in_array('--commit', $_SERVER['argv'], true);

$db = new Db(Config::fromEnv(is_file($root . '/.env') ? $root . '/.env' : null));

try {
    $pdo = $db->pdo();
} catch (Throwable $e) {
    fwrite(STDERR, 'DB connect failed: ' . $e->getMessage() . "\n");
    exit(3);
}

echo 'mode: ' . ($commit ? 'COMMIT' : 'DRY-RUN (rollback)') . "\n\n";

$pdo->beginTransaction();
try {
    $rates = $db->execute(
        "DELETE FROM rate WHERE bill_plan_id IN (SELECT id FROM bill_plan WHERE name LIKE 'TEST_%')",
    );
    $tariffs = $db->execute("DELETE FROM tariff WHERE name LIKE 'TEST_TARIFF_%'");
    $plans = $db->execute("DELETE FROM bill_plan WHERE name LIKE 'TEST_%'");

    printf("deleted -> rates: %d, tariffs: %d, bill_plans: %d\n", $rates, $tariffs, $plans);

    if ($commit) {
        $pdo->commit();
        echo "\nCOMMITTED.\n";
    } else {
        $pdo->rollBack();
        echo "\nROLLED BACK (dry-run). Re-run with --commit to delete.\n";
    }
} catch (Throwable $e) {
    $pdo->rollBack();
    fwrite(STDERR, 'cleanup FAILED (rolled back): ' . $e->getMessage() . "\n");
    exit(1);
}
