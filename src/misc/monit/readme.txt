* send to e-mail notification (use libcurl)

	https://curl.haxx.se/libcurl/c/smtp-tls.html

* send to snmp trap(notification,event) 

https://serverfault.com/questions/534127/sending-e-mail-when-snmp-trap-is-received
https://stackoverflow.com/questions/30050542/how-to-send-v2-traps-in-net-snmp-using-c

http://www.net-snmp.org/

https://github.com/vlm/asn1c

RateEngine OID:
https://www.iana.org/assignments/enterprise-numbers/enterprise-numbers

# yum install net-snmp
# yum install net-snmp-devel
# yum install net-snmp-utils ... snmpget,...
# yum install net-snmp-perl .... mib2c
# yum install net-snmp-gui ... tkmib

#smilint mibs/RATE-ENGINE-MIB

mib2c -- generate template code for extending the agent


* RE6 SNMP statistics & events(notifications):

- send trap notifications
1.alarms by the manager

- write stat in SNMP counters:

1.RE6 state - starting processes and their PIDs,ts,...
2.sim CC
3.rating counters
4.cdrm counters

# net-snmp-config --cflags
-I. -I/usr/include

# net-snmp-config --libs
-Wl,-z,relro -Wl,--as-needed -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -L/usr/lib64 -lnetsnmp -lssl -lssl -lcrypto -lm


snmpget -v1 -c public hpvp.bulsat.com  system.sysUpTime system.sysContact.0

snmptranslate -m +RATE-ENGINE-MIB -IR -On rateEngine

[root@localhost mibs]# env MIBS="+RATE-ENGINE-MIB" mib2c rateEngineCDRMediatorTable

https://stackoverflow.com/questions/54630567/how-to-send-snmptrap-with-net-snmp-in-c

https://tools.ietf.org/html/rfc2578 , Structure of Management Information Version 2 (SMIv2)
