# Installation

## You can be used Docker containers (re6-core,re6-db)

You should be execute following commands:
```bash
git clone https://github.com/dkokov/RateEngine.git

cd RateEngine/docker

docker-compose up --build -d
```

## Compile and install from source

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
git clone https://github.com/dkokov/RateEngine.git
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

#### Start and stop RateEngine daemon

***Start daemon command by CLI:***
``` bash 
cd /usr/local/RateEngine
./bin/RateEngine -d
```

***Stop daemon command by CLI:***
```bash
cd /usr/local/RateEngine
./bin/RateEngine -k
```
<!--#### Start/stop script:

Default path with start init script:
```
cd /usr/local/RateEngine/scripts/init.d

cp -vi RateEngine /etc/init.d/
```
-->

