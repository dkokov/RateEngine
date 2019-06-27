# RateEngine installation

A RateEngine use follow libs:

|lib|using|
|---|---|
|json-c|always|
|libxml2|always|
|libpq|if you want to use PGSQL/compile pgsql.so|
|libmysqlc|if you want to use MYSQL/compile mysql.so|

Installation steps are follow:

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

compile core and modules,install in default path(/usr/local/RateEngine/)
```
make install
```

You can be used more options when compile and install a RE.
To see these options,use follow command:

```
make help
```

