# RateEngine

  A **RateEngine(RE)** is engine for calls/messages calculate and online call control.
Can be started as server(daemon) or to use from the console.


* [Introduction](#Introduction)

* [RateEngine software architecture](doc/arch.md)

* [RateEngine installation](doc/install.md)

* [RateEngine configurations](doc/config.md)

* [FreeSWITCH CallControl Integration](doc/fs_cc.md)

* [Asterisk CallControl Integration](doc/ast_cc.md)




## Introduction



  Намира кой разговор на кой потребител е. 

Определя неговият билинг(тарифен) план и по коя от тарифите е този разговор
. 
Допълнително към тарифирането има възможност за времево зониране(сегментиране),
както и гъвкъв механизъм за пресмятане (пресмятащи функции). 
Към всяка тарифа или група от тарифи може да има безплатни минути.Плановете могат да имат период на действие. 
Същата възможност имат и тарифите. 
Това дава възможност за промоции с определен период на действие,след който влиза в действие друг план или тарифа .

  A **RE** get need records from definited servers as files or from databases.
Determine her **BillPlan** and **Tariff** is current call.

 In more cases,the rating engine is part by billing system.
Sometimes is released as module,sometimes is released as different application.
But the rating engine is part of entire billing system architecture.
 

  Why **RE** is different apllication ?


The last years appear different billing sistems.

 Последните години се появиха различни билинг системи. 

Много от фирмите в телекомуникациите започнаха сами да пишат или надграждат съществуващи системи. 
В самите билинг системи вече са включени много услуги - не само гласови, като освен това тези приложения вече са интегрирани със самите услуги. 
Те предлагат нещо повече от плащания и справки.Билинг приложенията вече управляват услугите. 
Пускат, спират и правят настройки по тях. На практика те се превърнаха в интерфейс към много други приложения, 
работещи за различни услуги и функционалностти . 

Например Internet, IPTV, VoIP са различни по характеристики услуги.Обикновенно са реализирани върху различни платформи. 

For example: Internet,IPTV,VOIP are different services. Usualy are released over different platforms.

Не пречи обаче да се управляват от една система, единен интерфейс. И в много от случайте  това е точно билинга. 
В цялата тази схема, рейтването е една част, една функционалност. Рейтването на разговори обаче е специфична дейност . 
Нужни са познания и опит . Тъй като тази фукционалност е замислена като самостоятелно приложение са добавени възможностти за интегриране към съществуващите билинг системи . 
RateEngine ще работи на заден план като част от цялата билинг система. 
В зависимост от използваният softswitch ще може да осигури и Call Control за да могат да се реализират prepaid  или credit control на postpaid услуги . 
С предоставените API-та може да  се интегрират настройките към конкретен билинг. 
Ако това не е необходимо може да се използва собственият интерфейс на машината и тя да се настройва като самостоятелно приложение  извън дадената билинг система .


RE може да обслужва няколко сървъра едновременно, но не може един акаунт(потребител) да се използва на няколко сървъра т.е. потребителят е обвързан с конкретен CDR сървър!!!


You can see example topology for RateEngine using in follow picture:

![](doc/png/RateEngine_v2.png)


## RateEngine components

The **RE** is released as moduler software - every functionality is different module.
For example: TCP support - tcp module (tcp.so) or pgsql support - pgsql module(pgsql.so).
Main modules are: CDRMediator (cdrm.so),Rating (rt.so) and CallControl (cc.so).
If you want to use PGSQL,then should use 'pgsql.so'.And you want to use MSQL,then should use 'mysql.so'. 

### RateEngine core (libre7core.so)

In the RateEngine core are defined few system interfaces:

#### **mod**

#### **db**

#### **net**

#### **mem**

#### **config**

#### **misc**

#### **log**


### CDRMediator module (cdrm.so)

### Rating module (rt.so)

### CallControl (cc.so)

## RateEngine installation

Installation steps are follow:

``` 
git clone https://github.com/dkokov/RateEngine.git
```

main source directory:
```
cd src/
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

## RateEngine configurations

#### Main config file

**The main RateEngine config file is RateEngine.xml :**

``` XML
<RateEngine version="0.7.0" >
 <System>
    <param name="DIR" value="/usr/local/RateEngine/" />
    <param name="PIDFile" value="logs/rate_engine.pid" />
 </System>
 <LoadModules>
    <param name="module" value="pgsql.so" />
    <param name="module" value="mysql.so" />
    <param name="module" value="redis.so" />
    <param name="module" value="cdrm.so" />
    <param name="module" value="rt.so" />
 </LoadModules>
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
 <CDRMediator>
    <param name="CDRProfilesDIR" value="/usr/local/RateEngine/config/cdr_profiles/" />
 </CDRMediator>
 <Logs>
    <param name="LogFile" value="logs/rate_engine.log" />
    <param name="LogMaxFileSize" value="40960000" />
    <param name="LogSeparator" value="|" />
    <param name="LogDateFormat" value="" />
    <!-- 1,INFO ; 2,WARN ; 3,DEBUG ; 4,DEBUG + TIMING ;-->
    <param name="LogDebugLevel" value="3" />
 </Logs>
</RateEngine>
```

## RateEngine commands:

Go to default directory:
```
cd /use/local/RateEngine
```

List commands and arguments:
```
./bin/RateEngine -h
```

Start as daemon(in background):
```
./bin/RateEngine -d
```

Start CDR gettings,without daemonization:
```
./bin/RateEngine -g
```

Start Rating,Leg a(incomming calls),without daemonization:
```
./bin/RateEngine -r a
```

Start CallControl,without daemonization:
```
./bin/RateEngine -2c
```

## 
