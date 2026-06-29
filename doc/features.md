# Features

  This page lists the RateEngine features, grouped by the module that provides
them - CDRMediator, Rating, CallControl - plus the transport and database
engine modules.

There is a list in a table with **'RateEngine features'**:

|Feature:|Release:|
|---|---|
|[UDPSupport](#UDPSupport)|in the **udp** module|
|[TCPSupport](#TCPSupport)|in the **tcp** module|
|[SCTPSupport](#SCTPSupport)|in the **sctp** module|
|[TLSSupport](#TLSSupport)|in the **tls** module|
|[PostgreSQLSupport](#PostgreSQLSupport)|in the **pgsql** module|
|[MySQLSupport](#MySQLSupport)|in the **mysql** module|
|[REDISSupport](#REDISSupport)|in the **redis** module|
|[MongoDBSupport](#MongoDBSupport)|in the **mongodb** module|
|[DuckDBSupport](#DuckDBSupport)|in the **duckdb** module|
|||
|[getCDRsSched](#getCDRsSched)|in the **CDRMediator** module|
|[getCDRsFromDB](#getCDRsFromDB)|in the **CDRMediator** module|
|[getCDRsFromCSVfile](#getCDRsFromCSVfile)|in the **CDRMediator** module|
|[PrefixFilter](#PrefixFilter)|in the **CDRMediator** module|
|[CDRMediating](#CDRMediating)|in the **CDRMediator** module|
|||
|[Prerating](#Prerating)|in the **Rating** module|
|[Payment Card Managment](#PaymentCardManagment)|in the **Rating** module|
|[Rate Searching](#RateSearching)|in the **Rating** module|
|[Calc Functions](#CalcFunctions)|in the **Rating** module|
|[Time Conditions](#TimeConditions)|in the **Rating** module|
|[Free Billsec](#FreeBillsec)|in the **Rating** module|
|[Rating](#Rating)|in the **Rating** module|
|[Batch Rating](#BatchRating)|in the **RatingDuckDB** module|
|||
|[Call Control Server](#Call_Control_Server)|in the **CallControl** module|
|[myCC Support](#myCC_Support)|in the **myCC** module|
|[JSON-RPC Support](#JSON-RPC_Support)|in the **JSON-RPC** modules|




## Features descriptions :

### Transport modules

The transport modules carry CallControl / RPC requests between RateEngine and
your voice platform. You load only the protocols you need.

#### UDPSupport

Connectionless UDP transport for the CallControl / RPC interface. Lightweight,
suited to high request rates where occasional loss is acceptable.

#### TCPSupport

Connection-oriented TCP transport - reliable, ordered delivery for the
CallControl / RPC interface.

#### SCTPSupport

SCTP transport, useful in telecom environments that already standardise on it
(multi-homing, message framing).

#### TLSSupport

TLS-secured transport for the RPC interface, for encrypted CallControl traffic
over untrusted networks.

### Database engine modules

RateEngine talks to storage through a `db_*` abstraction layer, so each backend
is a loadable engine module. Any `dbtype` used in the config must have its
engine module loaded first (see [Configurations](config.md)).

#### PostgreSQLSupport

PostgreSQL backend (`pgsql.so`, via libpq). The primary local store for
`cdrs`, `rating`, `balance` and all dimension tables.

#### MySQLSupport

MySQL/MariaDB backend (`mysql.so`, via libmysqlclient).

#### REDISSupport

Redis backend (`redis.so`, via hiredis) - in-memory store / cache use cases.

#### MongoDBSupport

MongoDB backend (`mongodb.so`, via mongo-c-driver) - document store use cases.

#### DuckDBSupport

Embedded analytical engine (`duckdb.so`). A first-class peer of `pgsql.so`: it
runs in-process, can `ATTACH` PostgreSQL read-only, and is what the
[Batch Rating](#BatchRating) path uses to rate thousands of CDRs per query.

### CDRMediator features

The CDRMediator fetches Call Detail Records from external sources (a switch's
own DB or CSV exports) and normalises them into the RateEngine `cdrs` table.
One CDR profile + one thread per CDR source. See [CDRMediator](cdrm.md).

#### getCDRsSched

Periodic, scheduled fetching. Each CDRMediator thread sleeps for the profile's
interval and then runs `getCDRs` again - e.g. with a 600 s interval the source
is polled every 600 s.

#### getCDRsFromDB

Pull CDRs directly from a remote database (e.g. FreeSWITCH/Asterisk CDR DB)
using the source's connection profile.

#### getCDRsFromCSVfile

Read CDRs from CSV files exported by the switch, parsed per the CDR profile's
field mapping.

#### PrefixFilter

Filter fetched CDRs by called-number prefix, so only the calls you care about
are imported / rated.

#### CDRMediating

The mediation step itself: map the external CDR fields onto RateEngine's `cdrs`
columns (per the CDR profile) before rating. See
[CDR Profile Example](cdr_profile.md).

### Rating features

The Rating module turns finished CDRs into priced `rating` rows and updates
`balance`. Two interchangeable engines exist (both build to `rt.so`): the
classic per-CDR **Rating** and the batch **RatingDuckDB** (see
[Batch Rating](#BatchRating) and [Configurations](config.md)).

#### Prerating

A pre-rating pass that loads the subscriber's balance and validates / blocks
payment cards (expiry, state) before the main pricing runs.

#### PaymentCardManagment

Prepaid and postpaid (credit-limit) handling per subscriber via payment cards
(`pcard`) and their balances - the basis for [CallControl](call_control.md)
allow/deny decisions.

#### RateSearching

Resolves the price path for each call: subscriber/account ->
`bill_plan` -> `bill_plan_tree` (nested plans) -> `rate` -> longest matching
`prefix` -> `tariff`.

#### CalcFunctions

The pricing calculators - notably multi-tier pricing (`calc_cprice_2`:
`delta` / `fee` / `iterations` steps) and `round_billsec` (ceil/floor of the
billed microseconds).

#### TimeConditions

Different prices for different time windows of a tariff (date / day-of-week /
hour ranges, including midnight-wrap), so peak/off-peak pricing is possible.

#### FreeBillsec

Free-seconds allowance per tariff. Boundary calls are split into a free portion
and a paid portion (`double_rating`), and consumed free seconds are tracked in
`free_billsec_balance`.

#### Rating

The offline rating run over the `cdrs` backlog. The classic engine issues
15-20 SQL queries per CDR, applying the features above per call.

#### BatchRating

`RatingDuckDB` rates CDRs **in bulk** with the embedded DuckDB engine instead
of per-CDR queries: it pins a window of unrated CDRs, resolves accounts and
matches rates set-based, prices every call in one recursive query, then writes
`rating` / `cdrs.leg` / `balance` back to PostgreSQL. It reproduces the
`Rating` billing logic (account modes, multi-tier pricing, time conditions,
free seconds, balance) but processes thousands of CDRs per cycle. Unmatched
CDRs in the evaluated window are marked `leg = -1` so they are not re-queried
each cycle. Full details in `src/mod/RatingDuckDB/README.md`.

### CallControl features

#### Call_Control_Server

The online plane: a CallControl process enforces prepaid / credit-limit in real
time. It watches each subscriber's consumption against the allowed amount and
returns a `maxsec` (and can deny new calls when the limit is reached). It reuses
the Rating, CDRMediator and transport functionalities. See
[CallControl](call_control.md).

#### myCC_Support

Native CallControl interface protocol (`my_cc`) - RateEngine's own request /
response format for call-control integrations.

#### JSON-RPC_Support

JSON-RPC interface over the transport modules. `jsonrpc_cc` exposes call control
and `jsonrpc_rt` exposes rating, for integration with external billing systems.
