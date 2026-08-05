<?php

declare(strict_types=1);

namespace RateEngine\RE7\Provisioning;

use RateEngine\RE7\Db;
use RateEngine\RE7\Exception\ImportException;
use RateEngine\RE7\Repository\BillPlanRepository;
use RateEngine\RE7\Repository\PrefixRepository;
use RateEngine\RE7\Repository\RateRepository;
use RateEngine\RE7\Repository\TariffRepository;

/**
 * CSV settings importer — RE7-native port of the mode-based format used by the
 * legacy PHP (`lib.importing.php`) and the Python `re_cli` importer.
 *
 * Each row's first field is a mode:
 *   1  bill_plan                         (name, type, [start_period], [end_period])
 *   2  prefix + tariff + rate            (prefix, prefix_desc, tariff, [start], [end], [free_billsec_id])
 *   *  calc_function (current tariff)    -- pending
 *   3  billing_account + rating account  -- pending
 *   4  prepaid card (current account)    -- pending
 *   5  bill_plan_tree                     -- pending
 *
 * State (current bill plan / tariff / prefix) carries across rows, exactly like
 * the legacy importer. Run inside Db::transaction() so a bad file rolls back.
 */
final class Importer
{
    private readonly BillPlanRepository $billPlans;
    private readonly PrefixRepository $prefixes;
    private readonly TariffRepository $tariffs;
    private readonly RateRepository $rates;

    private ?int $curBillPlanId = null;
    private ?int $curTariffId = null;
    private ?int $curPrefixId = null;

    public function __construct(
        private readonly Db $db,
        private readonly string $delimiter = ',',
    ) {
        $this->billPlans = new BillPlanRepository($db);
        $this->prefixes = new PrefixRepository($db);
        $this->tariffs = new TariffRepository($db);
        $this->rates = new RateRepository($db);
    }

    public function import(string $path): ImportResult
    {
        $handle = @fopen($path, 'rb');
        if ($handle === false) {
            throw new ImportException("cannot open import file: {$path}");
        }

        $result = new ImportResult();

        try {
            $lineNo = 0;
            while (($line = fgets($handle)) !== false) {
                ++$lineNo;
                $line = trim($line);
                if ($line === '' || $line[0] === '#') {
                    continue;
                }

                $row = str_getcsv($line, $this->delimiter);
                $mode = trim((string) ($row[0] ?? ''));

                match ($mode) {
                    '1' => $this->modeBillPlan($row, $result),
                    '2' => $this->modePrefixTariffRate($row, $result),
                    '*', '3', '4', '5' => $result->skip($mode),
                    default => throw new ImportException("unknown mode '{$mode}' at line {$lineNo}"),
                };
            }
        } finally {
            fclose($handle);
        }

        return $result;
    }

    /** Mode 1: bill_plan (name, type, [start_period], [end_period]). */
    private function modeBillPlan(array $row, ImportResult $result): void
    {
        $name = self::str($row, 1);
        if ($name === '') {
            throw new ImportException('mode 1: empty bill_plan name');
        }

        $type = self::str($row, 2, 'postpaid');
        $startPeriod = self::int($row, 3);
        $endPeriod = self::int($row, 4);

        $before = $this->billPlans->findId($name);
        $this->curBillPlanId = $this->billPlans->getOrCreate($name, $type, $startPeriod, $endPeriod);
        if ($before === null) {
            ++$result->billPlans;
        }

        // a new bill plan resets the tariff/prefix cursor
        $this->curTariffId = null;
        $this->curPrefixId = null;
    }

    /** Mode 2: prefix + tariff + rate under the current bill plan. */
    private function modePrefixTariffRate(array $row, ImportResult $result): void
    {
        if ($this->curBillPlanId === null) {
            throw new ImportException('mode 2 seen before any bill_plan (mode 1)');
        }

        $prefix = self::str($row, 1);
        $prefixDesc = self::str($row, 2);
        $tariff = self::str($row, 3);
        if ($prefix === '' || $tariff === '') {
            throw new ImportException('mode 2: prefix and tariff are required');
        }

        $startPeriod = self::int($row, 4);
        $endPeriod = self::int($row, 5);
        $freeBillsecId = self::int($row, 6);

        $prefixBefore = $this->prefixes->findId($prefix);
        $this->curPrefixId = $this->prefixes->getOrCreate($prefix, $prefixDesc !== '' ? $prefixDesc : null);
        if ($prefixBefore === null) {
            ++$result->prefixes;
        }

        $tariffBefore = $this->tariffs->findId($tariff);
        $this->curTariffId = $this->tariffs->getOrCreate($tariff, $startPeriod, $endPeriod, $freeBillsecId);
        if ($tariffBefore === null) {
            ++$result->tariffs;
        }

        $rateBefore = $this->rates->findId($this->curBillPlanId, $this->curPrefixId, $this->curTariffId);
        $this->rates->getOrCreate($this->curBillPlanId, $this->curPrefixId, $this->curTariffId);
        if ($rateBefore === null) {
            ++$result->rates;
        }
    }

    private static function str(array $row, int $idx, string $default = ''): string
    {
        $v = $row[$idx] ?? null;
        if ($v === null) {
            return $default;
        }
        $v = trim((string) $v, " \t\n\r\0\x0B\"'");

        return $v === '' ? $default : $v;
    }

    private static function int(array $row, int $idx, int $default = 0): int
    {
        $v = self::str($row, $idx);

        return $v === '' ? $default : (int) $v;
    }
}
