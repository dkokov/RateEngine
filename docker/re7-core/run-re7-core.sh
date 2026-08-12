#!/bin/bash

APP_DIR="/usr/local/RateEngine"
CLI="$APP_DIR/bin/RateEngine"
CONF="$APP_DIR/config/RateEngine7.xml"
PSQL=/usr/bin/psql
CERT_DIR="$APP_DIR/config/certs"
GEN_CERT="$APP_DIR/scripts/gen_tls_cert.sh"

# How long to wait for re7-db to finish initdb before giving up (seconds).
DB_WAIT_SECS=60

# first run: install + init db
if [ ! -f /root/.pgpass ]; then
    cp -vfR /re7-core/tmp/* $APP_DIR/
    cp -vfR /re7-core/docker/re7-core/samples/* $APP_DIR/config/

    # inject the DB password from the environment (never stored in the image).
    : "${RE7_DB_PASS:?RE7_DB_PASS not set (docker/.env)}"
    printf 're7-db:*:rate_engine:re_admin:%s\n' "$RE7_DB_PASS" > /root/.pgpass
    chmod 0600 /root/.pgpass
    # fill the '__RE7_DB_PASS__' placeholder in the copied config (bash expansion
    # handles any special chars in the password)
    for f in $(find "$APP_DIR/config" -name '*.xml'); do
        tmp=$(mktemp)
        while IFS= read -r line; do printf '%s\n' "${line//__RE7_DB_PASS__/$RE7_DB_PASS}"; done < "$f" > "$tmp"
        mv "$tmp" "$f"
    done

fi

# TLS credentials for the CallControl 'tls' interfaces (cc_int/*_tls.xml).
#
# Those interfaces need cert/key to exist or they fail to bind, and the tls
# module uses ONE process-wide SSL_CTX, so a single server.crt/key pair covers
# every TLS interface. Generated only when missing, so a real cert mounted (or
# dropped into the bind-mounted config/certs/) is never overwritten.
#
# SELF-SIGNED - test/dev only. gen_tls_cert.sh also emits ca.crt + client.crt,
# which is what you need for mutual TLS (verify-client=yes in the interface xml)
# and for src/clients/my_cc/tls_client.c.
if [ ! -f "$CERT_DIR/server.crt" ] || [ ! -f "$CERT_DIR/server.key" ]; then
    if [ -x "$GEN_CERT" ]; then
        echo "no TLS cert in $CERT_DIR - generating a self-signed test pair (CN=re7-core) ..."
        echo "WARNING: self-signed, for testing only. Mount a real cert for production."
        # CN=re7-core so in-network clients can verify by container hostname.
        if ! "$GEN_CERT" "$CERT_DIR" re7-core 825; then
            echo "WARNING: gen_tls_cert.sh failed - the cc_int/*_tls.xml interfaces will not start."
        fi
    else
        echo "WARNING: $GEN_CERT not found/executable - cannot generate TLS certs."
        echo "         The cc_int/*_tls.xml interfaces will fail to bind."
    fi
fi

# Wait for re7-db to be initialized - NOT just reachable.
#
# The schema is loaded exactly once, by re7-db itself: docker-compose mounts
# rt_pgsql_v2.sql into /docker-entrypoint-initdb.d/, which runs on an empty data
# dir and ends with an INSERT into 'version'. re7-core used to psql -f its own
# (older) rt_pgsql.sql here as a fallback, guarded by this same 'version' probe -
# dead code in the compose setup, since the probe always found the row.
#
# What is NOT optional is waiting. The postgres entrypoint runs the initdb
# scripts with the server on the unix socket only, so TCP is refused for the
# whole init window: a fixed 'sleep 5' could expire mid-init, and psql's stderr
# has to be discarded to probe cleanly, which makes "connection refused"
# indistinguishable from "no version row". Previously that ended with the engine
# starting against a schema-less DB. Now it is a hard failure instead - this is a
# billing engine, a missing schema must not look like a healthy start.
#
# This runs on every start, not only the first: on a container restart the
# .pgpass guard above is skipped, but re7-db may still be coming up.
echo "waiting for re7-db schema (up to ${DB_WAIT_SECS}s) ..."

_RES=""
for _i in $(seq 1 "$DB_WAIT_SECS"); do
    _RES=$($PSQL -h re7-db -U re_admin rate_engine -c "select date from version;" -A -t 2>/dev/null)
    [ -n "$_RES" ] && break
    sleep 1
done

if [ -z "$_RES" ]; then
    echo "ERROR: re7-db is not reachable or has no schema after ${DB_WAIT_SECS}s."
    echo "       Check the re7-db container and its /docker-entrypoint-initdb.d mount."
    echo "       Refusing to start the engine against an uninitialized database."
    exit 1
fi

echo "db initialized (version.date = $_RES)"
echo "starting RateEngine7 ..."

exec $CLI -c $CONF -f
