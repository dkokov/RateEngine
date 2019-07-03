## RateEngine configurations

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


The **'Rating'** part is defined all params for this module.

``` XML 
 <Rating>
    <param name="active" value="no" />
    <param name="leg" value="a" />
    <!-- <param name="NoPrefixRating" value="&" /> -->
    <!-- RatingInterval, seconds -->
    <param name="RatingInterval" value="120" />
    <!-- WaitRatingInterval , microseconds -->
    <param name="WaitRatingInterval" value="5" />
    <param name="RatingJSONConfigDIR" value="/home/dkokov/VQuality/RateEngine/v7/src/config/samples/rt_json/" />
    <!-- When you want to use pcard for all subscribers,
         set this param with 'yes' -->
    <param name="UsePCard" value="no" />
    <!-- 'start_date' or 'end_date' -->
    <param name="PCardSortKey" value="start_date" />
    <!-- 'desc' or 'asc' -->
    <param name="PCardSortMode" value="desc" />
    <!-- When you want to use only one billing day(one billing cycle),
         insert a billing day here.If it's empty,
         then will use 'billing_day' from the 'billing_account' table -->
    <param name="BillingDay" value="01" />
    <param name="DayOfPayment" value="10" />
    <!-- k limit min = (limit/amount) when have more from 1 rating account per billing account -->
    <param name="KLimitMin" value="0.05" />
 </Rating>
```

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
