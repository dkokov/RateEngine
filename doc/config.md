# Configurations

**The main RateEngine config file is 'RateEngine.xml'**

Examine different parts by this file.Step by step.

The **'System'** part is defined main application path and pid file name.

``` XML
 <System>
    <param name="DIR" value="/usr/local/RateEngine/" />
    <param name="PIDFile" value="logs/rate_engine.pid" />
 </System>
```

The **'LoadModules'** part is defined modules list for loading after app start.
If you want to use same module,should be defined first here!

``` XML
 <LoadModules>
    <param name="module" value="pgsql.so" />
    <!-- DuckDB engine for the RatingDuckDB module; load before rt.so -->
    <param name="module" value="duckdb.so" />
    <param name="module" value="mysql.so" />
    <param name="module" value="redis.so" />
    <param name="module" value="cdrm.so" />
    <param name="module" value="rt.so" />
 </LoadModules>
```

The **'DB'** part is defined a local storage params(pgsql,mysql,redis,etc).
Every 'dbtype' who is used here,have to load first as module - pgsql,mysql or redis.

``` XML
 <DB>
    <param name="dbtype" value="pgsql" />
    <param name="dbhost" value="127.0.0.1" />
    <param name="dbname" value="dbname" />
    <param name="dbuser" value="dbuser" />
    <param name="dbpass" value="dbpassword" />
    <!-- 3306,5432,6379 -->
    <param name="dbport" value="5432" />

    <param name="NumberRetries" value="10" />
    <param name="IntervalRetries" value="2" />
 </DB>
```

The **'CallControl'** part is defined all params for this module.
Very importment params are 'SimCalls','CallMaxsecLimit','IntConfigDIR'.
First (sim calls number) define reserved dymanic memory for this task.
Second (maximum maxsec) define when auto stop a current call(different for every call).
Last is the path to CallControl interface profiles.

``` XML
 <CallControl>
    <param name="active" value="yes" />

    <!-- server sleeping in microseconds -->
    <param name="CCServerMicroSleep" value="60000" />

    <!-- maxsec per call in seconds -->
    <param name="CallMaxsecLimit" value="3600" />

    <!-- Sim calls -->
    <param name="SimCalls" value="30000" />

    <!-- Interface Configuration Directory - config per protocol/interface -->
    <param name="IntConfigDIR" value="/usr/local/RateEngine/config/cc_int/" />
 </CallControl>
```

* [CallControl Interface Profile Example](cc_int_prof.md) 


The **'Rating'** section configures the rating engine.

RateEngine ships two interchangeable rating modules - **both build to `rt.so`**,
so you load `rt.so` either way and choose which one to build in `config.md`:

* **RatingDuckDB** (`mod/RatingDuckDB`) - analytical *batch* rating with an
  embedded DuckDB engine. The current default. Rates thousands of CDRs per
  cycle in one set of analytical queries; needs the `duckdb.so` engine module.
* **Rating** (`mod/Rating`) - the classic per-CDR engine (15-20 SQL queries
  per call). Its parameters are kept further below for reference.

**RatingDuckDB parameters:**

``` XML
 <Rating>
    <param name="active" value="yes" />
    <!-- rating leg: 'a' or 'b' -->
    <param name="leg" value="a" />
    <!-- idle poll interval, seconds (used when the CDR queue is empty) -->
    <param name="RatingInterval" value="300" />
    <!-- throttle between back-to-back drain cycles, microseconds -->
    <param name="WaitRatingInterval" value="1500" />
    <!-- CDRs rated per batch window (default 5000) -->
    <param name="BatchLimit" value="5000" />
    <!-- DuckDB worker threads; 0 = all cores -->
    <param name="RatingThreads" value="4" />
    <!-- dimension cache mode:
         'all'    (default) - cache every lookup table; fastest, most RAM
         'static'           - cache small config tables, account tables stay live
         'none'             - all dimensions live as views over PostgreSQL -->
    <param name="CacheDimensions" value="all" />
    <!-- billing cycle day; if empty, uses 'billing_day' from 'billing_account' -->
    <param name="BillingDay" value="01" />
    <param name="DayOfPayment" value="10" />
 </Rating>
```

**Legacy `Rating` module parameters** (only relevant when you build
`mod/Rating` instead of `mod/RatingDuckDB`):

``` XML
 <Rating>
    <param name="active" value="no" />
    <param name="leg" value="a" />
    <!-- RatingInterval, seconds -->
    <param name="RatingInterval" value="120" />
    <!-- WaitRatingInterval, microseconds -->
    <param name="WaitRatingInterval" value="5" />
    <param name="RatingJSONConfigDIR" value="/usr/local/RateEngine/config/samples/rt_json/" />
    <!-- single billing cycle day; if empty, uses 'billing_day' from 'billing_account' -->
    <param name="BillingDay" value="01" />
    <param name="DayOfPayment" value="10" />
    <!-- k limit min = (limit/amount) when more than 1 rating account per billing account -->
    <param name="KLimitMin" value="0.05" />
 </Rating>
```

> Note: `UsePCard`, `PCardSortKey` and `PCardSortMode` appeared in older
> configs but are no longer used by the engine.

In **'CDRMediator'** part has only path to cdr profiles.Same module will read and load all profiles.

``` XML
 <CDRMediator>
    <param name="CDRProfilesDIR" value="/usr/local/RateEngine/config/cdr_profiles/" />
 </CDRMediator>
```

* [CDR Profile Example](cdr_profile.md) 

The 'Logs' part is defined all params for logging.
Can be defined log file name,log file size - after that rolling log files,
level for logging(NONE,INFO,DEBUG,...),separator in the file,etc.

``` XML
 <Logs>
    <param name="LogFile" value="logs/rate_engine.log" />
    <param name="LogMaxFileSize" value="40960000" />
    <param name="LogSeparator" value="|" />
    <param name="LogDateFormat" value="" />
    <!-- 1,INFO ; 2,WARN ; 3,DEBUG ; 4,DEBUG + TIMING ;-->
    <param name="LogDebugLevel" value="3" />
 </Logs>
```
