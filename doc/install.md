# Installation

There are two ways to get a RateEngine running:

* [Docker (quick start)](#docker-quick-start) - the fastest way to try it out
* [Manual build from source](#manual-build-from-source) - full control, production setups

---

## Docker (quick start)

A ready-to-use `docker-compose` setup lives in the `docker/` directory.
It builds two containers: **re7-db** (PostgreSQL, schema preloaded) and
**re7-core** (the RateEngine core + modules).

```
cd docker/
docker-compose up --build -d
```

To stop and remove everything (including the data volumes):

```
docker-compose down -v
```

Sample configuration used by the core container is in
`docker/re7-core/samples/` (RateEngine7.xml, cdr_profiles, cc_int).

---

## Manual build from source

### External libraries

RateEngine uses the following external libraries:

|lib|needed for|
|---|---|
|json-c|always|
|libxml2|always|
|libpq|PGSQL support / `pgsql.so`|
|libmysqlclient|MySQL support / `mysql.so`|
|hiredis|Redis support / `redis.so`|
|mongo-c-driver|MongoDB support / `mongodb.so`|
|libduckdb|DuckDB support / `duckdb.so` (auto-fetched, see below)|

You only need the libs for the modules you actually enable in `config.md`.

**You must install the `-devel` / `-dev` (development) version of these libs!**

RedHat/CentOS/Fedora :
```
dnf install gcc make libxml2-devel json-c-devel \
            libpq-devel mysql-devel hiredis-devel mongo-c-driver-devel
```

Debian/Ubuntu :
```
apt-get install gcc make libxml2-dev libjson-c-dev \
                libpq-dev default-libmysqlclient-dev libhiredis-dev libmongoc-dev
```

> The DuckDB SDK (~70 MB) is **not** kept in git. When you build the
> `db_duckdb` module it is downloaded on demand by `scripts/fetch_duckdb.sh`
> (version pinned in `mod/db_duckdb/DUCKDB_VERSION`). No system package needed.

### Get the source

```
git clone https://github.com/dkokov/RateEngine.git
cd RateEngine/src/
```

### Select modules

Edit `config.md` and comment/uncomment the modules you want to build.
A line starting with `#` is **disabled**.

```
vim config.md
```

A typical `config.md` looks like this:
```
mod/CDRMediator
#mod/Rating
mod/RatingDuckDB
#mod/CallControl
mod/db_pgsql
mod/db_duckdb
#mod/db_mysql
#mod/db_redis
#mod/db_mongodb
#mod/tcp
#mod/udp
#mod/sctp
#mod/tls
#mod/my_cc
#mod/jsonrpc_cc
#mod/jsonrpc_rt
```

> Pick **one** rating module: `mod/Rating` (classic, per-CDR) **or**
> `mod/RatingDuckDB` (analytical batch). When using `RatingDuckDB`, make sure
> `mod/db_duckdb` is enabled too, and note that `db_duckdb` must be built
> **before** `RatingDuckDB` (rt.so binds the DuckDB engine at runtime).

### Build and install

Compile the core + all modules from `config.md` and install into the default
path (`/usr/local/RateEngine/`):

```
make install
```

Other useful targets:

```
make                              - build core only (standard gcc flags)
make DEBUG=1                      - build with debug flags
make modules                      - build all modules listed in config.md
make module name=db_duckdb        - build a single module
make module_install name=db_duckdb- build + install a single module
make install PREFIX=/opt          - install into a custom prefix
make uninstall                    - uninstall from the default path
make clean                        - remove objects, shared libs and binaries
make help                         - full list of targets
```

DuckDB engine specifics:
```
make module name=db_duckdb                       - build duckdb.so (auto-downloads SDK; dynamic)
make module name=db_duckdb DUCKDB_LINK=static    - self-contained duckdb.so (no runtime libduckdb.so)
./scripts/fetch_duckdb.sh mod/db_duckdb/lib      - (re)download the SDK manually
```

### SQL DB init

Create the database first, then load the schema.

The init scripts are installed under:
```
cd /usr/local/RateEngine/scripts/sql/
```

PostgreSQL:
```
psql -h localhost -U youruser yourdbname -f rt_pgsql.sql
```

MySQL:
```
mysql -h localhost -u youruser -p yourdbname < rt_mysql.sql
```

(`re7_test_1.sql` contains optional sample/test data.)

### Start/stop service

**systemd** (recommended):
```
sudo cp /usr/local/RateEngine/scripts/systemd/rate-engine.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now rate-engine.service
```
See `scripts/systemd/README.md` for paths to check before first start.

**SysV init** (legacy):
```
cd /usr/local/RateEngine/scripts/init.d
cp -vi RateEngine /etc/init.d/
```
