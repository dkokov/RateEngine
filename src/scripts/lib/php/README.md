# RE7 PHP library (`rateengine/re7-lib`)

Shared, **front-end-agnostic** PHP library for reading and provisioning the
**RE7** database (schema: `src/scripts/sql/rt_pgsql_v2.sql`). It is the single
**production provisioning funnel** — the web-API portal (and any other PHP
consumer) sits on top of it; it contains no HTTP, no GUI and no softswitch glue.

It is a clean, RE7-native rewrite. The old PHP (`src/scripts/cli/legacy_php`,
VoIPManager `lib.re5.php`) is used only as a *reference* for which operations and
business rules exist — none of its string-concatenated SQL is carried over.

## Ground rules (do not violate)

1. **Parameterized SQL only.** All queries go through `Db` with bound
   parameters (`PDO`, prepared statements). Never build SQL by concatenation.
2. **RE7 schema only.** Columns/tables follow `rt_pgsql_v2.sql`. No RE5/6 code.
3. **Effective-dated writes.** Provisioning is append/forward, never an in-place
   edit of a row that live calls depend on. `bill_plan`/`tariff` carry
   `start_period`/`end_period`; a "change" is a new version, not a mutation.
4. **No rating math.** This library never computes a call price. Rating is the
   engine's job — the portal delegates it (stage → engine rates → read back).
5. **Guard in-use edits.** Any in-place update/delete of a record that active
   calls may reference must pass an `ActiveCallGate` (wired by the portal to the
   engine's active-call query). Pure inserts of new keys are always safe.

## Layout

```
src/
  Config.php                     DB config (env-driven)
  Db.php                         PDO wrapper: one/all/value/insertReturningId/transaction
  Exception/                     typed exceptions
  Guard/
    ActiveCallGate.php           interface: assertNotInUse(entity, id)
    NullActiveCallGate.php       no-op default (safe for pure inserts)
  Repository/
    AbstractRepository.php       shared base (Db + gate)
    BillPlanRepository.php       ── implemented (template) ──┐
    PrefixRepository.php         ── implemented             │ import modes 1 & 2
    TariffRepository.php         ── implemented             │ bill_plan -> prefix/
    RateRepository.php           ── implemented ────────────┘ tariff/rate
  Provisioning/
    Importer.php                 CSV importer; modes 1 & 2 done, 3/4/5/* stubbed
    ImportResult.php             per-run counters
```

## Usage

```php
require __DIR__ . '/vendor/autoload.php';

use RateEngine\RE7\Config;
use RateEngine\RE7\Db;
use RateEngine\RE7\Provisioning\Importer;

$db  = new Db(Config::fromEnv(__DIR__ . '/.env'));
$res = $db->transaction(fn (Db $db) => (new Importer($db))->import('settings.csv'));

echo $res; // bill_plans: N, prefixes: N, tariffs: N, rates: N ...
```

## Extending

Add one `*Repository` per RE7 entity, following `BillPlanRepository` as the
template: a `findId(...)` and a `getOrCreate(...)` built on `Db`, using bound
parameters. Then wire it into `Importer` for the relevant mode. Still to add:
`BillingAccountRepository`, `PcardRepository`, `CallingNumberRepository`,
`CalcFunctionRepository`, `BillPlanTreeRepository`, `BalanceRepository`
(read-only for provisioning), plus report queries mined from the legacy libs.

## Relationship to the Python CLI

`src/scripts/cli/py_cli` (`re_cli`) stays as the **dev / test / CI tooling**
path. This PHP library is the **production** write funnel. Keep one production
write path — the CLI should not write provisioning in production.
