## CDR Profile Example

  Every CDR Profile file has follow parts:

* [General](#General)

* [CDR Format](#CDRFormat)

* [Prefix Filtering](#PrefixFiltering)


The **Prefix Filtering** is optional.Can be removed by the CDR Profile file.


### General

There is a part top.

``` XML
  <General>
    <param name="profile-version" value="1" />
    <param name="profile-name" value="fs" />
    <param name="active" value="no" />
    <!-- Thread parameters -->
    <param name="getCDRsInterval" value="60" />
    <param name="getCDRsReplies" value="2" />
    <!-- Filtering -->
    <param name="CalledNumberFiltering" value="yes" />
    <!-- file,db -->
    <param name="cdr-type" value="db" />
```

There are params only for DB.

``` XML
    <!-- Get CDR 'db' config -->
    <param name="dbhost" value="localhost" />
    <param name="dbuser" value="global" />
    <param name="dbpass" value="_cfg.access" />
    <param name="dbname" value="fs_cdrs" />
    <param name="dbport" value="5432" />
    <!-- DB type (pgsql,mysql,oracle) -->
    <param name="dbtype" value="pgsql" />
    <!-- SQL Query into the remote db server -->
    <param name="cdr-table" value="cdrs" />
    <!-- 'timestamp' or 'integer(epoch)' type -->
    <param name="sql-col-where" value="ts" />
    <!-- 'ts' or 'epoch' -->
    <param name="sql-col-where-type" value="ts" />
    <param name="sql-where-const" value="billsec > 0" />
    <!-- Start scheduler timestamp (start date) -->
    <param name="SchedTS" value="2018-12-01 00:00:00" />
```

There are params only for CSV file.

``` XML
    <param name="src-dir" value="/usr/local/RateEngine/src-cdrs/" />
    <param name="dst-dir" value="/usr/local/RateEngine/dst-cdrs/" />
    <param name="cols-separator" value="," />
    <param name="col-delimiter" value='"' />
    <param name="line-end" value="\n" />
    <param name="file-field-num" value="20" />
```

There is a part end.The 'cdr-rec-type' is importment for the Rating.
If you don't use 'cdr-rec-type' as param in a profile file then the Rating will use type 'unkn'.

``` XML
    <!-- unkn, isup, sms, voip-audio, voip-video ,voip-trunk -->
    <param name="cdr-rec-type" value="voip-trunk" />
  </General>
```


#### CDRFormat

If you create a CDR profile for CSV file,use for 'value' digits.
When you want to creaete CDR profile for DB,then use the column names from remote CDR DB table.

Full CDR elements listing:

``` XML
  <CDRFormat>
    <param name="call_uid" value="" />
    <param name="start_ts" value="" />
    <param name="answer_ts" value="" />
    <param name="end_ts" value="" />
    <param name="start_epoch" value="" />
    <param name="answer_epoch" value="" />
    <param name="end_epoch" value="" />
    <param name="src" value="" />
    <param name="dst" value="" />
    <param name="calling_number" value="" />
    <param name="clg_nadi" value="" />
    <param name="called_number" value="" />
    <param name="cld_nadi" value="" />
    <param name="rdnis" value="" />
    <param name="rdnis_nadi" value="" />
    <param name="ocn" value="" />
    <param name="ocn_nadi" value="" />
    <param name="account_code" value="" />
    <param name="src_context" value="" />
    <param name="src_tgroup" value="" />
    <param name="dst_context" value="" />
    <param name="dst_tgroup" value="" />
    <param name="billsec" value="" />
    <param name="duration" value="" />
    <param name="uduration" value="" />
    <param name="billusec" value="" />
  </CDRFormat>
```

There is CSV file variant:
``` XML
    <param name="call_uid" value="1" />
```

There is DB variant:

``` XML
    <param name="duration" value="duration" />
```

If don't have a value in tag 'value' - that mean 'value=""',then this param is not use by the CDRMediator.



#### PrefixFiltering

Don't have different in the 'PrefixFiltering' for CSV file and for DB.
The 'filter' is table as XML release.

``` XML
  <PrefixFiltering>
    <filter>
    <param name="prefix" value="2" />
    <param name="num" value="1" />
    <param name="replace" value="359429372" />
    <param name="len" value="3" />
    </filter>
    <filter>
    <param name="prefix" value="4" />
    <param name="num" value="1" />
    <param name="replace" value="359241194" />
    <param name="len" value="3" />
    </filter>
    <filter>
    <param name="prefix" value="6" />
    <param name="num" value="1" />
    <param name="replace" value="359241196" />
    <param name="len" value="3" />
    </filter>
    <filter>
    <param name="prefix" value="7" />
    <param name="num" value="1" />
    <param name="replace" value="359241197" />
    <param name="len" value="3" />
    </filter>
    <filter>
    <param name="prefix" value="9" />
    <param name="num" value="1" />
    <param name="replace" value="359241199" />
    <param name="len" value="3" />
    </filter>
  </PrefixFiltering>
```