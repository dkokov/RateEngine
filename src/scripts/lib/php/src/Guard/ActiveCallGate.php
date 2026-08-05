<?php

declare(strict_types=1);

namespace RateEngine\RE7\Guard;

use RateEngine\RE7\Exception\InUseException;

/**
 * Guards in-place edits/deletes of records that a live call may depend on.
 *
 * The portal wires a concrete implementation to the engine's active-call query
 * (CallControl `state`). Pure inserts of new keys never need this — only
 * mutation of an existing, potentially in-use row does.
 */
interface ActiveCallGate
{
    /**
     * @param string $entity logical entity name, e.g. "rate", "tariff", "bill_plan"
     * @param int    $id     primary key of the record about to be mutated
     *
     * @throws InUseException if active calls reference the record
     */
    public function assertNotInUse(string $entity, int $id): void;
}
