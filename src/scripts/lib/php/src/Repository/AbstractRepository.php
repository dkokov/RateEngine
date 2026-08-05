<?php

declare(strict_types=1);

namespace RateEngine\RE7\Repository;

use RateEngine\RE7\Db;
use RateEngine\RE7\Guard\ActiveCallGate;
use RateEngine\RE7\Guard\NullActiveCallGate;

/**
 * Base for entity repositories. Holds the Db and the ActiveCallGate used to
 * guard in-place edits. Subclasses expose findId()/getOrCreate() style methods
 * built on parameterized queries.
 */
abstract class AbstractRepository
{
    protected readonly ActiveCallGate $gate;

    public function __construct(
        protected readonly Db $db,
        ?ActiveCallGate $gate = null,
    ) {
        $this->gate = $gate ?? new NullActiveCallGate();
    }
}
