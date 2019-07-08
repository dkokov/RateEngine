# CallControl

  Very important possibility in the RateEngine gives a CallControl.
You can be released prepaid or postpaid in your voice platform.In the RateEngine has a CallControl module,
but as feature there is a process,not only module.
In the same CallControl process are using functionalities by entire RateEngine (Rating,CDRMediator,MyCC or JSON-RPC over UDP/TCP/TLS,etc).


![](png/CallControl.png)


  When you are releasing prepaid,the CallControl is following whether a prepaid amount is reaching to defined amount per this subscriber.
When this amount is reached,then the CallControl deny calls per this subscriber.

  For prepaid or postpaid release per Subscriber is using a [PaymentCardManagment](features.md#PaymentCardManagment).


See more information for some CallControl integrations :

* [FreeSWITCH CallControl Integration](fs_cc.md)

* [Asterisk CallControl Integration](ast_cc.md)