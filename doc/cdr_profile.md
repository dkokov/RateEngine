## CDR Profile Example

  Every CDR Profile file has follow parts:

* [General](#General) - between XML tags **'<General>'** and **'</General>'**

* [CDR Format](#CDRFormat) - between XML tags **'<CDRFormat>'** and **'</CDRFormat>'**

* [Prefix Filtering](#PrefixFiltering) - between XML tags **'<PrefixFiltering>'** and **'</PrefixFiltering'**


The **Prefix Filtering** is optional.Can be removed by the CDR Profile file.


### General

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
<!--    <param name="PrefixFilterNumber" value="25" /> -->
    <!-- file,db -->
    <param name="cdr-type" value="db" />
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
    <!-- unkn, isup, sms, voip-audio, voip-video ,voip-trunk -->
    <param name="cdr-rec-type" value="voip-audio" />
  </General>
```

#### CDRFormat

``` XML
  <!-- CDR fields in the DB (matching in the DB cols ) -->
  <CDRFormat>
    <param name="call_uid" value="call_uid" />
    <param name="start_ts" value="ts" />
    <param name="answer_ts" value="" />
    <param name="end_ts" value="" />
    <param name="start_epoch" value="" />
    <param name="answer_epoch" value="" />
    <param name="end_epoch" value="" />
    <param name="src" value="" />
    <param name="dst" value="" />
    <param name="calling_number" value="src" />
    <param name="clg_nadi" value="" />
    <param name="called_number" value="dst" />
    <param name="cld_nadi" value="" />
    <param name="rdnis" value="" />
    <param name="rdnis_nadi" value="" />
    <param name="ocn" value="" />
    <param name="ocn_nadi" value="" />
    <param name="account_code" value="account_code" />
    <param name="src_context" value="src_context" />
    <param name="src_tgroup" value="src_tgroup" />
    <param name="dst_context" value="dst_context" />
    <param name="dst_tgroup" value="dst_tgroup" />
    <param name="billsec" value="billsec" />
    <param name="duration" value="duration" />
    <param name="uduration" value="" />
    <param name="billusec" value="" />
  </CDRFormat>
```


#### PrefixFiltering

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