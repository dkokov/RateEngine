/*
 * TLS transport module for RateEngine V7.
 *
 * Refs:
 *   https://www.openssl.org/docs/manmaster/man3/
 *   https://wiki.openssl.org/index.php/Simple_TLS_Server
 *
 * This module plugs into the same net_funcs_t vtable as tcp.so and is driven by
 * CallControl's parallel (worker-pool) server, net_parallel_server():
 *
 *   acceptor thread : api->accept(server_conn) -> pushes the raw client fd
 *   worker thread   : api->recv_msg(wc) -> handler() -> api->send_msg(wc) -> api->close(wc)
 *
 * Only the int fd travels from acceptor to worker (via a queue), so the TLS
 * handshake CANNOT be done in accept() - the resulting SSL* would be lost. It is
 * therefore deferred into tls_recv(), which runs on the worker that owns the fd:
 * many clients negotiate in parallel instead of serialising on the acceptor.
 *
 * Object ownership:
 *   - SSL_CTX  : one per interface, created in tls_open() on the single-threaded
 *                setup path and stored on the server conn's eng_ctx. It is
 *                propagated read-only to each worker conn by net_parallel_server()
 *                (workers only SSL_new() from it, which OpenSSL >= 1.1 makes
 *                thread-safe). Freed on server-side tls_close(). Each interface
 *                thus has its own cert/key and its own client-verify setting.
 *   - SSL      : one per connection, created in tls_recv(), stored in that
 *                worker's own net_conn_t->eng_tmp, consumed by tls_send() and
 *                freed by tls_close().
 */

#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../net/net.h"
#include "../mod.h"

#include "tls.h"

/* free a per-interface TLS context (SSL_CTX + wrapper) */
static void tls_ctx_free(tls_t *ptr)
{
	if(ptr == NULL) return;
	if(ptr->ctx != NULL) SSL_CTX_free(ptr->ctx);
	mem_free(ptr);
}

/* drain the OpenSSL error queue into the RateEngine log */
static void tls_log_err(const char *where)
{
	unsigned long e;
	char buf[256];

	while((e = ERR_get_error()) != 0) {
		ERR_error_string_n(e,buf,sizeof(buf));
		LOG("tls","%s: %s",where,buf);
	}
}

/* build the shared SSL_CTX and load the server cert/key. When verify_peer != 0,
 * also load the CA bundle 'ca' and require every client to present a certificate
 * that chains to it (mutual TLS). Returns NULL on any failure (errors logged). */
static tls_t *tls_ctx_init(const char *cert,const char *key,int verify_peer,const char *ca)
{
	tls_t *ptr;

	if((cert == NULL)||(strlen(cert) == 0)) {
		LOG("tls_ctx_init()","no server certificate configured ('cert' is empty)");
		return NULL;
	}
	if((key == NULL)||(strlen(key) == 0)) {
		LOG("tls_ctx_init()","no server private key configured ('key' is empty)");
		return NULL;
	}

	ptr = mem_alloc(sizeof(tls_t));
	if(ptr == NULL) return NULL;

	ptr->method = TLS_server_method();
	ptr->ctx    = SSL_CTX_new(ptr->method);
	if(ptr->ctx == NULL) {
		tls_log_err("SSL_CTX_new");
		mem_free(ptr);
		return NULL;
	}

	/* refuse legacy, broken protocol versions */
	SSL_CTX_set_min_proto_version(ptr->ctx,TLS1_2_VERSION);

	if(SSL_CTX_use_certificate_chain_file(ptr->ctx,cert) <= 0) {
		tls_log_err("SSL_CTX_use_certificate_chain_file");
		SSL_CTX_free(ptr->ctx);
		mem_free(ptr);
		return NULL;
	}

	if(SSL_CTX_use_PrivateKey_file(ptr->ctx,key,SSL_FILETYPE_PEM) <= 0) {
		tls_log_err("SSL_CTX_use_PrivateKey_file");
		SSL_CTX_free(ptr->ctx);
		mem_free(ptr);
		return NULL;
	}

	if(SSL_CTX_check_private_key(ptr->ctx) <= 0) {
		tls_log_err("SSL_CTX_check_private_key");
		SSL_CTX_free(ptr->ctx);
		mem_free(ptr);
		return NULL;
	}

	/* optional mutual TLS: request + verify a client certificate */
	if(verify_peer) {
		if((ca == NULL)||(strlen(ca) == 0)) {
			LOG("tls_ctx_init()","client-cert verification requested but no CA configured ('ca' is empty)");
			SSL_CTX_free(ptr->ctx);
			mem_free(ptr);
			return NULL;
		}

		if(SSL_CTX_load_verify_locations(ptr->ctx,ca,NULL) <= 0) {
			tls_log_err("SSL_CTX_load_verify_locations");
			SSL_CTX_free(ptr->ctx);
			mem_free(ptr);
			return NULL;
		}

		/* SSL_VERIFY_FAIL_IF_NO_PEER_CERT: a client with no cert is rejected at
		 * the handshake (SSL_accept fails in tls_recv), not silently allowed. */
		SSL_CTX_set_verify(ptr->ctx,SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,NULL);
		SSL_CTX_set_verify_depth(ptr->ctx,4);

		LOG("tls_ctx_init()","mutual TLS ENABLED - client cert required (CA: %s)",ca);
	} else {
		LOG("tls_ctx_init()","client cert verification disabled (server-side TLS only)");
	}

	return ptr;
}

/* net_open: create the listening socket for this interface and, once per
 * interface, its own TLS context (cert/key + verify setting). The context is
 * stored on the server conn's eng_ctx and later propagated read-only to each
 * worker conn by net_parallel_server(). Runs on the single setup path per
 * interface (net_open()), so no locking is needed. */
int tls_open(net_conn_t *conn)
{
	int ret;
	tls_t *ctx;

	if(conn == NULL) return NET_ERROR_SOCKET;

	ret = net_open_socket(conn);
	if(ret < 0) return ret;

	ctx = tls_ctx_init(conn->cert_filename,conn->pkey_filename,
	                   conn->tls_verify_peer,conn->ca_filename);
	if(ctx == NULL) {
		LOG("tls_open()","server TLS context init FAILED (cert: %s)",conn->cert_filename);
		net_close_socket(conn);
		return NET_ERROR_SOCKET;
	}

	conn->eng_ctx = (void *)ctx;

	LOG("tls_open()","server TLS context ready (port: %d,cert: %s)",conn->port,conn->cert_filename);

	return NET_OK;
}

/* net_close: a worker conn (t == client) tears down its per-connection SSL
 * (eng_tmp) and closes the fd; the shared per-interface SSL_CTX (eng_ctx) is
 * owned by the server conn and freed only on server-side teardown. */
void tls_close(net_conn_t *conn)
{
	SSL *ssl;

	if(conn == NULL) return;

	ssl = (SSL *)conn->eng_tmp;
	if(ssl != NULL) {
		SSL_shutdown(ssl);
		SSL_free(ssl);
		conn->eng_tmp = NULL;
	}

	if((conn->t == server) && (conn->eng_ctx != NULL)) {
		tls_ctx_free((tls_t *)conn->eng_ctx);
		conn->eng_ctx = NULL;
	}

	net_close_socket(conn);
}

/* net_accept: plain TCP accept only. The TLS handshake is deferred to tls_recv()
 * on the worker that owns the fd (see file header). */
int tls_accept(net_conn_t *conn)
{
	socklen_t clilen;
	struct sockaddr_in cli_addr;

	if(conn == NULL) return NET_ERROR_SOCKET;

	clilen = sizeof(cli_addr);

	conn->newsockfd = accept(conn->sockfd,(struct sockaddr *)&cli_addr,&clilen);
	if(conn->newsockfd < 0) return NET_ERROR_ACCEPT;

	return NET_OK;
}

/* net_recv: runs on a worker with a fresh net_conn_t (eng_tmp == NULL, eng_ctx
 * propagated from the interface's server conn). Mints an SSL from that
 * interface's context, completes the server handshake, reads one request, and
 * hands the live SSL* to tls_send()/tls_close() via eng_tmp. Returns the byte
 * count (> 0) on success, a negative NET_ERROR_* otherwise. */
int tls_recv(net_conn_t *conn)
{
	int n,fd;
	SSL *ssl;
	tls_t *srv;

	if(conn == NULL) return NET_ERROR_SOCKET;
	if(conn->eng_ctx == NULL) return NET_ERROR_RECV;
	if(conn->buffer == NULL || conn->buf_size == 0) return NET_ERROR_RECV;

	srv = (tls_t *)conn->eng_ctx;

	fd = (conn->t == server) ? conn->sockfd : conn->newsockfd;

	ssl = SSL_new(srv->ctx);
	if(ssl == NULL) {
		tls_log_err("SSL_new");
		return NET_ERROR_RECV;
	}

	SSL_set_fd(ssl,fd);

	if(SSL_accept(ssl) <= 0) {
		tls_log_err("SSL_accept");
		SSL_free(ssl);
		return NET_ERROR_RECV;
	}

	/* leave room for a NUL terminator: downstream treats buffer as a C string */
	n = SSL_read(ssl,conn->buffer,conn->buf_size - 1);
	if(n <= 0) {
		tls_log_err("SSL_read");
		SSL_shutdown(ssl);
		SSL_free(ssl);
		return NET_ERROR_RECV;
	}

	conn->buffer[n] = '\0';

	conn->eng_tmp = (void *)ssl;

	return n;
}

/* net_send: reply over the SSL created by tls_recv() for this connection. */
int tls_send(net_conn_t *conn)
{
	int n;
	SSL *ssl;

	if(conn == NULL) return NET_ERROR_SOCKET;

	ssl = (SSL *)conn->eng_tmp;
	if(ssl == NULL) return NET_ERROR_SEND;

	n = SSL_write(ssl,conn->buffer,strlen(conn->buffer));
	if(n <= 0) {
		tls_log_err("SSL_write");
		return NET_ERROR_SEND;
	}

	return n;
}

int tls_bind_api(net_funcs_t *ptr)
{
	if(ptr == NULL) return -1;

	ptr->open     = tls_open;
	ptr->close    = tls_close;
	ptr->accept   = tls_accept;
	ptr->listen   = NULL;      /* core net_listen() binds + listens on the socket */
	ptr->recv_msg = tls_recv;
	ptr->send_msg = tls_send;
	ptr->status   = NULL;
	ptr->connect  = NULL;
	ptr->s_server = NULL;      /* CallControl drives the parallel worker-pool server */

	return 0;
}
