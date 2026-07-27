#ifndef TLS_H
#define TLS_H

#include <openssl/ssl.h>

/* Process-wide server TLS context. One SSL_CTX is shared by every CallControl
 * worker; per-connection SSL objects are minted from it at recv time and kept
 * in net_conn_t->eng_tmp (see tls.c). */
typedef struct tls {
	SSL_CTX *ctx;
	const SSL_METHOD *method;
} tls_t;

#endif
