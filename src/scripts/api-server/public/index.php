<?php

declare(strict_types=1);

require dirname(__DIR__) . '/vendor/autoload.php';

use RateEngine\RE7\Api\Bootstrap;

Bootstrap::create(dirname(__DIR__))->run();
