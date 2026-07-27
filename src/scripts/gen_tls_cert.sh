#!/usr/bin/env bash
#
# Generate a small test PKI for the RateEngine CallControl tls transport
# (src/mod/tls). For testing / internal deployments; use a proper CA in
# production.
#
# Produces, in OUT_DIR:
#   ca.crt      / ca.key       - local test CA (signs the two certs below)
#   server.crt  / server.key   - server cert (SAN=CN,127.0.0.1) -> interface cert/key
#   client.crt  / client.key   - client cert (only needed for mutual TLS)
#
# The server presents server.crt. For mutual TLS the interface sets
# verify-client=yes + ca=<.../ca.crt>; a client must then present client.crt
# (signed by ca.crt) or the handshake is rejected.
#
# Usage:
#   ./gen_tls_cert.sh [OUT_DIR] [CN] [DAYS]
#     OUT_DIR  output directory                       (default: ./certs)
#     CN       server Common Name / SAN               (default: localhost)
#     DAYS     validity in days                       (default: 825)

set -euo pipefail

OUT_DIR="${1:-./certs}"
CN="${2:-localhost}"
DAYS="${3:-825}"

mkdir -p "$OUT_DIR"
cd "$OUT_DIR"

# --- local test CA ---------------------------------------------------------
openssl req -x509 -newkey rsa:2048 -sha256 -days "$DAYS" -nodes \
    -keyout ca.key -out ca.crt -subj "/CN=RateEngine Test CA"

# --- server cert (signed by the CA, with SAN) ------------------------------
openssl req -newkey rsa:2048 -nodes -keyout server.key -out server.csr \
    -subj "/CN=$CN"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -days "$DAYS" -sha256 -out server.crt \
    -extfile <(printf "subjectAltName=DNS:%s,IP:127.0.0.1\nextendedKeyUsage=serverAuth\n" "$CN")

# --- client cert (signed by the CA; for mutual TLS) ------------------------
openssl req -newkey rsa:2048 -nodes -keyout client.key -out client.csr \
    -subj "/CN=re7-test-client"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -days "$DAYS" -sha256 -out client.crt \
    -extfile <(printf "extendedKeyUsage=clientAuth\n")

rm -f server.csr client.csr ca.srl
chmod 600 ca.key server.key client.key

echo
echo "Wrote in $OUT_DIR:"
echo "  ca.crt / ca.key          (test CA)"
echo "  server.crt / server.key  (interface 'cert'/'key')"
echo "  client.crt / client.key  (client, for mutual TLS)"
echo
echo "Server-side TLS : cert=server.crt key=server.key"
echo "Mutual TLS      : + verify-client=yes ca=ca.crt ; client presents client.crt/client.key"
