# Software architecture

  The **RE** is released as moduler software - every functionality is different module.
The modules are share library(.so) files. Use dynamic load(dl) conception by Linux.
For example: TCP support - tcp module (tcp.so) or pgsql support - pgsql module(pgsql.so).
Main modules are: CDRMediator (cdrm.so),Rating (rt.so) and CallControl (cc.so).
If you want to use PGSQL,then should use 'pgsql.so'.And you want to use MSQL,then should use 'mysql.so'. 

Simple block schem of the RateEngine.

![](png/RE7arch.png)


## RateEngine core (libre7core.so)

In the RateEngine core are defined few system interfaces:

#### **mod**
All is start with this interface,because it has to get defined modules in the main config.
Load every module,check for init function,check for depends and push in the modules list.

After stop,the interface check for destroy function,call it and close the dynamic library (.so),
who is opened before (in starting).

#### **db**

#### **net**

#### **mem**

#### **proc**

#### **config**

#### **misc**

#### **log**
