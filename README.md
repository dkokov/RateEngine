# RateEngine

  A **RateEngine(RE)** is engine for calls/messages calculate and 
online call control - prepaid and postpaid credit limit.
Can be started as server(daemon) or to use from the console.


* [Introduction](#Introduction)

* [RateEngine software architecture](doc/arch.md)

* [RateEngine installation](doc/install.md)

* [RateEngine configurations](doc/config.md)

* [FreeSWITCH CallControl Integration](doc/fs_cc.md)

* [Asterisk CallControl Integration](doc/ast_cc.md)



## Introduction

  In more cases,the rating engine is part by billing system.
Sometimes is released as module,sometimes is released as different application.
But the rating engine is part of entire billing system architecture.
 
  Why **RE** is different apllication ?

  The last years appear different billing sistems.


For example: Internet,IPTV,VOIP are different services. Usualy are released over different platforms.



  A **RE** get need records from definited servers as files or from databases.
Determine her **BillPlan** and **Tariff** is current call.
Calculate and save in balance.Can be used **FreeBillsec** per difined tariff.
Can be used **TimeConditions** per tariff - different prices in different time zone.
The **RE** to be managed by external **BillingSystem** with APIs or other external interface.

You can see example topology for RateEngine using in follow picture:

![](doc/png/RateEngine_v2.png)


