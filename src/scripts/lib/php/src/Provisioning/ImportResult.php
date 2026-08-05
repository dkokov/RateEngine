<?php

declare(strict_types=1);

namespace RateEngine\RE7\Provisioning;

/** Per-run counters for an import. */
final class ImportResult
{
    public int $billPlans = 0;
    public int $prefixes = 0;
    public int $tariffs = 0;
    public int $rates = 0;

    /** rows whose mode is not yet implemented (3/4/5/*), by mode */
    public array $skipped = [];

    public function skip(string $mode): void
    {
        $this->skipped[$mode] = ($this->skipped[$mode] ?? 0) + 1;
    }

    public function __toString(): string
    {
        $s = sprintf(
            'bill_plans: %d, prefixes: %d, tariffs: %d, rates: %d',
            $this->billPlans,
            $this->prefixes,
            $this->tariffs,
            $this->rates,
        );

        if ($this->skipped !== []) {
            $parts = [];
            foreach ($this->skipped as $mode => $n) {
                $parts[] = sprintf('mode %s: %d', $mode, $n);
            }
            $s .= ' | skipped(pending) => ' . implode(', ', $parts);
        }

        return $s;
    }
}
