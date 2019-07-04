## CallControl Interface  Profile Example

  Configuration of the CallControl interfaces is very easy.
Before to use every interfaces,you have to load same module.Every interface is released as module.
In the moment,the RateEngine CallControl is supporting two type interfaces: my_cc and json_rpc.
Can be configured the IP version - 4 or 6 version,and transport protocol - udp,tcp,tls,sctp.

``` XML
<Interface>
 <config>
    <!-- CallControl - Session/Application layer protocol : my_cc,json_rpc  -->
    <param name="CC-proto" value="json_rpc" />
    <!-- Network layer protocol: udp,tcp,tls,sctp -->
    <param name="proto" value="tcp" />
    <!-- IP version: IPv4,IPv6 -->
    <param name="ip-version" value="IPv4" />
    <param name="ip" value="" />
    <param name="port" value="9091" />
    <!-- 1024 bytes -->
    <param name="rcv_buffer_size" value="1024" />
 </config>
</Interface>
```