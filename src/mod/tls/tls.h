#ifndef TLS_H
#define TLS_H

typedef struct tls {
	SSL *ssl;
	SSL_CTX *ctx;
	const SSL_METHOD *method;
} tls_t;

#endif 
