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

### TLS transport

When `proto` is `tls`, add the server certificate and private key (PEM). The
transport is independent of the wire protocol, so `tls` works with either
`my_cc` or `jsonrpc_cc`. Each interface has its own TLS context, so different
interfaces may use different certificates and verify policies.

``` XML
<Interface>
 <config>
    <param name="CC-proto" value="jsonrpc_cc" />
    <!-- Network layer protocol: udp,tcp,tls,sctp -->
    <param name="proto" value="tls" />
    <param name="ip-version" value="IPv4" />
    <param name="ip" value="" />
    <param name="port" value="9092" />
    <!-- TLS server credentials (PEM), required when proto="tls" -->
    <param name="cert" value="/usr/local/RateEngine/config/certs/server.crt" />
    <param name="key"  value="/usr/local/RateEngine/config/certs/server.key" />
    <!-- Mutual TLS (optional, default off): verify-client="yes" requires every
         client to present a certificate that chains to the CA bundle 'ca';
         clients without a valid cert are rejected at the TLS handshake. -->
    <param name="verify-client" value="no" />
    <param name="ca" value="/usr/local/RateEngine/config/certs/ca.crt" />
 </config>
</Interface>
```

Generate a self-signed test PKI (CA + server + client certs) with
`src/scripts/gen_tls_cert.sh`. Notes:

* The TLS handshake is performed by the worker (in `recv`), so many clients
  negotiate concurrently.
* Changing `verify-client` (or any TLS credential) takes effect on the next
  RateEngine restart, since the TLS context is built once when the interface
  opens.