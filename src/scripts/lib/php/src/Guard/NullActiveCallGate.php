<?php

declare(strict_types=1);

namespace RateEngine\RE7\Guard;

/**
 * No-op gate. Safe default for provisioning flows that only insert new keys
 * (effective-dated writes). Do NOT use this for in-place edits of live data in
 * production — supply an engine-backed gate there.
 */
final class NullActiveCallGate implements ActiveCallGate
{
    public function assertNotInUse(string $entity, int $id): void
    {
        // intentionally empty
    }
}
