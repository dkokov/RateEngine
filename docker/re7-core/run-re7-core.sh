#!/bin/bash

APP_DIR="/usr/local/RateEngine"
CLI="$APP_DIR/bin/RateEngine"
CONF="$APP_DIR/config/RateEngine7.xml"
SQL="$APP_DIR/scripts/sql/rate_engine_0.6.13.sql"
PSQL=/usr/bin/psql

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

    # wait for db
    echo "waiting for re7-db ..."
    sleep 5

    # init schema if empty
    _RES=$($PSQL -h re7-db -U re_admin rate_engine -c "select date from version;" -A -t 2>/dev/null)
    if [ "$_RES" == "" ]; then
        if [ -f "$SQL" ]; then
            echo "creating db schema ..."
            $PSQL -h re7-db -U re_admin rate_engine -f $SQL
        else
            echo "WARNING: sql schema file not found: $SQL"
        fi
    else
        echo "db already initialized"
    fi
fi

echo "starting RateEngine7 ..."
$CLI -c $CONF -d

# keep container alive, follow the log
exec tail -f $APP_DIR/logs/rate_engine.log
