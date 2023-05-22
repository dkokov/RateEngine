# Installation

#### A RateEngine use follow external libs:

|lib|using|
|---|---|
|libxml2|always|
|libpq|if you want to use PGSQL/compile pgsql.so|


**You have to install devel version of these libs!!!**

RedHat/CentOS/Fedora :
```
yum install libxml2-devel
or
dnf install libxml2-devel
```

Debian :
```
apt-get install libxml2-dev libpq-dev
```


#### Installation steps are follow:

``` 
git clone -b 0.6.14 https://github.com/dkokov/RateEngine.git
```

main source directory:
```
cd RateEngine/src/
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
psql -h localhost -U youruser yourdbname -f rate_engine_0.6.10.sql
```

#### Start/stop script:

Default path with start init script:
```
cd /usr/local/RateEngine/scripts/init.d

cp -vi RateEngine /etc/init.d/
```


