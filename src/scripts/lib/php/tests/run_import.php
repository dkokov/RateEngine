<?php

/**
 * Manual import runner for rateengine/re7-lib.
 *
 * DRY-RUN by default: imports inside a transaction and ROLLS BACK, so it is
 * safe to run repeatedly against a real RE7 database without persisting.
 * Pass --commit to actually keep the rows.
 *
 * Usage (run from anywhere):
 *   cp .env.example .env   # edit RE7_DB_* to point at a TEST database
 *   php tests/run_import.php                         # dry-run, default fixture
 *   php tests/run_import.php tests/fixtures/settings_sample.csv
 *   php tests/run_import.php --commit path/to/settings.csv
 */

declare(strict_types=1);

$root = dirname(__DIR__);

// minimal PSR-4 autoloader (no composer install required)
spl_autoload_register(static function (string $class) use ($root): void {
    $prefix = 'RateEngine\\RE7\\';
    if (str_starts_with($class, $prefix)) {
        require $root . '/src/' . str_replace('\\', '/', substr($class, strlen($prefix))) . '.php';
    }
});

use RateEngine\RE7\Config;
use RateEngine\RE7\Db;
use RateEngine\RE7\Provisioning\Importer;

$argv = $_SERVER['argv'];
array_shift($argv);

$commit = false;
$csv = $root . '/tests/fixtures/settings_sample.csv';
foreach ($argv as $arg) {
    if ($arg === '--commit') {
        $commit = true;
    } else {
        $csv = $arg;
    }
}

if (!is_file($csv)) {
    fwrite(STDERR, "fixture not found: {$csv}\n");
    exit(2);
}

$envFile = is_file($root . '/.env') ? $root . '/.env' : null;
$db = new Db(Config::fromEnv($envFile));

try {
    $pdo = $db->pdo(); // connect now so a bad config fails loudly
} catch (Throwable $e) {
    fwrite(STDERR, 'DB connect failed: ' . $e->getMessage() . "\n");
    fwrite(STDERR, "Set RE7_DB_* in " . $root . "/.env (copy from .env.example).\n");
    exit(3);
}

echo "mode: " . ($commit ? 'COMMIT' : 'DRY-RUN (rollback)') . "\n";
echo "csv : {$csv}\n\n";

$pdo->beginTransaction();
try {
    $result = (new Importer($db))->import($csv);
    echo "import result -> {$result}\n\n";

    // verification: show what the fixture created (visible inside this tx)
    $names = ['TEST_PLAN_A', 'TEST_PLAN_B'];
    foreach ($names as $n) {
        $id = $db->value('SELECT id FROM bill_plan WHERE name = ?', [$n]);
        printf("  bill_plan %-14s id=%s\n", $n, $id ?? '(none)');
    }
    foreach (['TEST_TARIFF_STD', 'TEST_TARIFF_MOB'] as $t) {
        $id = $db->value('SELECT id FROM tariff WHERE name = ?', [$t]);
        printf("  tariff    %-14s id=%s\n", $t, $id ?? '(none)');
    }
    $rateCnt = $db->value(
        "SELECT count(*) FROM rate r
         JOIN bill_plan bp ON bp.id = r.bill_plan_id
         WHERE bp.name IN ('TEST_PLAN_A','TEST_PLAN_B')",
    );
    printf("  rate rows for test plans: %s\n", $rateCnt);

    if ($commit) {
        $pdo->commit();
        echo "\nCOMMITTED.\n";
    } else {
        $pdo->rollBack();
        echo "\nROLLED BACK (dry-run). Re-run with --commit to persist.\n";
    }
} catch (Throwable $e) {
    $pdo->rollBack();
    fwrite(STDERR, "\nimport FAILED (rolled back): " . $e->getMessage() . "\n");
    exit(1);
}
