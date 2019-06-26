# RateEngine software architecture

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

