# Installation

#### A RateEngine use follow external libs:

|lib|using|
|---|---|
|json-c|always|
|libxml2|always|
|libpq|if you want to use PGSQL/compile pgsql.so|
|libmysqlclient|if you want to use MYSQL/compile mysql.so|

**You have to install devel version of these libs!!!**

RedHat/CentOS/Fedora :
```
yum install libxml2-devel libjson-c-devel
or
dnf install libxml2-devel libjson-c-devel
```

Debian :
```
apt-get install libxml2-dev libjson-c-dev
```


#### Installation steps are follow:

``` 
git clone https://github.com/dkokov/RateEngine.git
```

main source directory:
```
cd RateEngine/src/
```

edit 'config.md' file - comment/uncomment names by modules:
```
vim config.md
```

The same file examines such:
```
mod/CDRMediator
mod/Rating
#mod/CallControl
mod/db_pgsql
mod/db_mysql
mod/db_redis
#mod/tcp
#mod/udp
#mod/sctp
#mod/tls
mod/my_cc
#mod/json_rpc[
```

compile core and modules,install in default path(/usr/local/RateEngine/)
```
make install
```

You can be used more options when compile and install a RE.
To see these options,use follow command:

```
make help
```

#### SQL DB init:

You should create database before to insert tables from the file(.sql)!

Default path with SQL init scripts:

```
cd /usr/local/RateEngine/scripts/sql/
```

If you want to use PGSQL,should start follow:
```
psql -h localhost -U youruser yourdbname -f rate_engine.sql
```

#### Start/stop script:

Default path with start init script:
```
cd /usr/local/RateEngine/scripts/init.d

cp -vi RateEngine /etc/init.d/
```


