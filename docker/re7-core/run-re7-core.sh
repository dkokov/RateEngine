#!/bin/bash

APP_DIR="/usr/local/RateEngine"
CLI="$APP_DIR/bin/RateEngine"
CONF="$APP_DIR/config/RateEngine7.xml"
SQL="$APP_DIR/scripts/sql/rate_engine_0.6.13.sql"
PSQL=/usr/bin/psql

# first run: install + init db
if [ ! -f /root/.pgpass ]; then
    cp -vfR /re7-core/tmp/* $APP_DIR/
    cp -vf /re7-core/docker/re7-core/samples/RateEngine7.xml $APP_DIR/config/
    cp -vf /re7-core/docker/re7-core/.pgpass /root/.pgpass
    chmod 0600 /root/.pgpass

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
exec $CLI -c $CONF -d
