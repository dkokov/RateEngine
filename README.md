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

  A **RE** get need records from definited servers as files or from databases.
Determine her **BillPlan** and **Tariff** is current call.

 In more cases,the rating engine is part by billing system.
Sometimes is released as module,sometimes is released as different application.
But the rating engine is part of entire billing system architecture.
 

  Why **RE** is different apllication ?


The last years appear different billing sistems.


For example: Internet,IPTV,VOIP are different services. Usualy are released over different platforms.


You can see example topology for RateEngine using in follow picture:

![](doc/png/RateEngine_v2.png)


