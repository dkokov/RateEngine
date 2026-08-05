<?php

declare(strict_types=1);

namespace RateEngine\RE7\Exception;

/**
 * Thrown when an in-place edit/delete is attempted on a record that active
 * calls may reference. Raised by an ActiveCallGate implementation.
 */
final class InUseException extends RE7Exception
{
}
