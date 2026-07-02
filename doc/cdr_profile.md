# CDR Profile Example

  A CDR profile describes **one CDR source** - where to read call records from,
and how to map their fields onto the RateEngine `cdrs` table. Working templates
ship under `config/samples/cdr_profiles/`
([cdr_db_profile.xml](../src/config/samples/cdr_profiles/cdr_db_profile.xml),
[cdr_file_profile.xml](../src/config/samples/cdr_profiles/cdr_file_profile.xml)).

  Every CDR profile file has the following parts:

* [General](#General)

* [CDR Format](#CDRFormat)

* [Prefix Filtering](#PrefixFiltering)


The **Prefix Filtering** part is optional and can be removed from the CDR profile file.


### General

Top part:

``` XML
  <General>
    <param name="profile-version" value="1" />
    <param name="profile-name" value="fs" />
    <!-- yes/no , profile active flag -->
    <param name="active" value="no" />
    <!-- Thread parameters -->
    <param name="getCDRsInterval" value="60" />
    <param name="getCDRsReplies" value="2" />
    <!-- Filtering -->
    <param name="CalledNumberFiltering" value="yes" />
    <!-- file,db -->
    <param name="cdr-type" value="db" />
```

Parameters used only for DB sources:

``` XML
    <!-- Get CDR 'db' config -->
    <param name="dbhost" value="localhost" />
    <param name="dbuser" value="global" />
    <param name="dbpass" value="_cfg.access" />
    <param name="dbname" value="fs_cdrs" />
    <param name="dbport" value="5432" />
    <!-- DB type: pgsql, mysql -->
    <param name="dbtype" value="pgsql" />
    <!-- remote CDR table name -->
    <param name="cdr-table" value="cdrs" />
    <!-- 'timestamp' or 'integer(epoch)' type -->
    <param name="sql-col-where" value="ts" />
    <!-- 'ts' or 'epoch' -->
    <param name="sql-col-where-type" value="ts" />
    <param name="sql-where-const" value="billsec > 0" />
    <!-- Start scheduler timestamp (start date) -->
    <param name="SchedTS" value="2018-12-01 00:00:00" />
```

Params used only for CSV files. The key one is `file-field-num` - the number of
columns in each CSV line. Files are read from `src-dir` and moved to `dst-dir`
after mediating. Scheduled fetching (`getCDRsSched`) does not apply to files.

``` XML
    <param name="src-dir" value="/usr/local/RateEngine/src-cdrs/" />
    <param name="dst-dir" value="/usr/local/RateEngine/dst-cdrs/" />
    <param name="cols-separator" value="," />
    <param name="col-delimiter" value='"' />
    <param name="line-end" value="\n" />
    <param name="file-field-num" value="20" />
```

Final part. The `cdr-rec-type` is important for the Rating.
If you don't set `cdr-rec-type` as a param in the profile file, the Rating uses type `unkn`.

``` XML
    <!-- unkn, isup, sms, voip-audio, voip-video ,voip-trunk -->
    <param name="cdr-rec-type" value="voip-trunk" />
  </General>
```


**Key `<General>` parameters**

| Param | Meaning |
|---|---|
| `profile-name` / `profile-version` | profile identity |
| `active` | `yes`/`no` - whether this source is polled by its thread |
| `getCDRsInterval` | seconds between fetches (DB sources) |
| `getCDRsReplies` | number of fetch retries per cycle |
| `CalledNumberFiltering` | `yes`/`no` - apply the `PrefixFiltering` rules |
| `cdr-type` | `db` or `file` |
| `cdr-table` | remote table to read (DB) |
| `sql-col-where` | timestamp column used to page through new CDRs (DB) |
| `sql-col-where-type` | `ts` (timestamp) or `epoch` (integer) |
| `sql-where-const` | extra static WHERE clause, e.g. `billsec > 0` |
| `SchedTS` | start timestamp for the scheduler (DB) |
| `cdr-rec-type` | record type for Rating: `unkn`, `isup`, `sms`, `voip-audio`, `voip-video`, `voip-trunk` (defaults to `unkn`) |

#### CDRFormat

Map each RateEngine CDR field to its source. For a **CSV** profile use the
column number (digits); for a **DB** profile use the column name from the remote
CDR table.

Full list of CDR elements:

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

CSV file variant:
``` XML
    <param name="call_uid" value="1" />
```

DB variant:

``` XML
    <param name="call_uid" value="call-uuid" />
```

If a param has no value (`value=""`), the CDRMediator ignores that field.



#### PrefixFiltering

`PrefixFiltering` normalizes the **called number** before rating - handy for
turning short or internal numbers into full E.164. It works the same for CSV and
DB sources, and is applied only when `CalledNumberFiltering` is `yes`. Each
`<filter>` is one rule:

| Field | Meaning |
|---|---|
| `prefix` | leading digit(s) the called number must start with |
| `len` | rule fires only when the called number has exactly this many digits |
| `num` | how many leading digit(s) to remove from the matched number |
| `replace` | string prepended to the result (e.g. a country/area code) |

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