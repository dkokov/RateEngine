/*
 * Minimal TLS test client for the RateEngine CallControl tls transport.
 *
 * Opens a TLS connection, sends one line (the JSON-RPC request), prints the
 * reply, and exits - mirroring 2cclient.c but over TLS. One request per
 * connection, matching the server (net_parallel_server closes after each reply).
 *
 * Build:
 *   gcc -g -Wall -o tls_client tls_client.c -lssl -lcrypto
 *
 * Usage:
 *   ./tls_client <host> <port> '<request>' [client_cert client_key] [ca_cert]
 *
 *   client_cert/client_key : present a client certificate (needed when the
 *                            server interface has verify-client=yes / mutual TLS)
 *   ca_cert                : verify the server certificate against this CA
 *                            (omit to skip server-cert verification, as before)
 *
 * Examples:
 *   # plain server-side TLS
 *   ./tls_client 127.0.0.1 9092 \
 *     '{"jsonrpc":"2.0","method":"rate","params":{"clg":"359112","cld":"359880001"},"id":611}'
 *
 *   # mutual TLS (present a client cert; also verify the server against the CA)
 *   ./tls_client 127.0.0.1 9092 '<request>' client.crt client.key ca.crt
 *
 * By default this client does NOT verify the server certificate (test tool). Do
 * not copy that choice into a production client without passing a CA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUF_LEN 2048

static void die_ssl(const char *msg)
{
	fprintf(stderr,"%s\n",msg);
	ERR_print_errors_fp(stderr);
	exit(1);
}

static int tcp_connect(const char *host,int port)
{
	int fd;
	struct hostent *server;
	struct sockaddr_in serv_addr;

	fd = socket(AF_INET,SOCK_STREAM,0);
	if(fd < 0) { perror("socket"); return -1; }

	server = gethostbyname(host);
	if(server == NULL) { fprintf(stderr,"no such host: %s\n",host); close(fd); return -1; }

	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);
	serv_addr.sin_port = htons(port);

	if(connect(fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
		perror("connect");
		close(fd);
		return -1;
	}

	return fd;
}

int main(int argc,char *argv[])
{
	int fd,n,port;
	SSL_CTX *ctx;
	SSL *ssl;
	char buf[BUF_LEN];

	if(argc < 4) {
		fprintf(stderr,"usage: %s <host> <port> '<json-rpc-request>'\n",argv[0]);
		return 1;
	}

	port = atoi(argv[2]);

	ctx = SSL_CTX_new(TLS_client_method());
	if(ctx == NULL) die_ssl("SSL_CTX_new");
	SSL_CTX_set_min_proto_version(ctx,TLS1_2_VERSION);

	/* optional: present a client certificate (for a mutual-TLS server) */
	if(argc >= 6) {
		if(SSL_CTX_use_certificate_file(ctx,argv[4],SSL_FILETYPE_PEM) <= 0) die_ssl("client SSL_CTX_use_certificate_file");
		if(SSL_CTX_use_PrivateKey_file(ctx,argv[5],SSL_FILETYPE_PEM) <= 0) die_ssl("client SSL_CTX_use_PrivateKey_file");
		if(SSL_CTX_check_private_key(ctx) <= 0) die_ssl("client SSL_CTX_check_private_key");
	}

	/* optional: verify the server certificate against a CA bundle */
	if(argc >= 7) {
		if(SSL_CTX_load_verify_locations(ctx,argv[6],NULL) <= 0) die_ssl("SSL_CTX_load_verify_locations");
		SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,NULL);
	}

	fd = tcp_connect(argv[1],port);
	if(fd < 0) { SSL_CTX_free(ctx); return 1; }

	ssl = SSL_new(ctx);
	SSL_set_fd(ssl,fd);

	if(SSL_connect(ssl) <= 0) die_ssl("SSL_connect");

	if(argc >= 7 && SSL_get_verify_result(ssl) != X509_V_OK) {
		fprintf(stderr,"server certificate verification FAILED\n");
		SSL_free(ssl); close(fd); SSL_CTX_free(ctx);
		return 1;
	}

	printf("TLS established: %s\n",SSL_get_version(ssl));

	/* send request */
	n = SSL_write(ssl,argv[3],strlen(argv[3]));
	if(n <= 0) die_ssl("SSL_write");
	printf("--> %s\n",argv[3]);

	/* read reply. In TLS 1.3 a server-side handshake rejection (e.g. a required
	 * client cert that was not presented) surfaces HERE, not at SSL_connect: the
	 * server sent a fatal alert and closed, so SSL_read returns <= 0. Report it
	 * instead of printing an empty line. */
	memset(buf,0,sizeof(buf));
	n = SSL_read(ssl,buf,sizeof(buf) - 1);
	if(n > 0) {
		buf[n] = '\0';
		printf("<-- %s\n",buf);
	} else {
		int err = SSL_get_error(ssl,n);
		unsigned long e = ERR_get_error();

		if(e != 0) {
			char ebuf[256];
			ERR_error_string_n(e,ebuf,sizeof(ebuf));
			/* e.g. "tlsv13 alert certificate required" */
			fprintf(stderr,"<-- ERROR: no response - %s\n",ebuf);
		} else if(err == SSL_ERROR_ZERO_RETURN) {
			fprintf(stderr,"<-- ERROR: server closed the TLS session with no data (close_notify)\n");
		} else {
			fprintf(stderr,"<-- ERROR: connection closed by server before a reply "
			               "(likely a TLS alert - check client cert / mTLS), SSL_get_error=%d\n",err);
		}

		SSL_shutdown(ssl);
		SSL_free(ssl);
		close(fd);
		SSL_CTX_free(ctx);
		return 2;
	}

	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(fd);
	SSL_CTX_free(ctx);

	return 0;
}
