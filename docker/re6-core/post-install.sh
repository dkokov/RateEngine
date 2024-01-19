#!/bin/bash

_SQL="/usr/local/RateEngine/scripts/sql/rate_engine_0.6.10.sql"
CLI=/usr/bin/psql

cd /re6-core/

if [ -f /root/.pgpass ]; then
    echo "...keep on..."
else
#	mv -v /re6-core/tmp/ /usr/local/RateEngine
	cp -vfR tmp/* /usr/local/RateEngine/
    cp -vf RateEngine6.xml /usr/local/RateEngine/config
    rm -fv /usr/local/RateEngine/config/cc_int/*
    cp -vf cc_int/* /usr/local/RateEngine/config/cc_int
    cp -vf .pgpass /root
    chmod 0600 /root/.pgpass
    sleep 5
    _RES_SQL=`$CLI -h 172.20.0.2 -U re_admin rate_engine -c "select date from version;" -A -t`
    if [ "$_RES_SQL" == "" ]; then 
		$CLI -h 172.20.0.2 -U re_admin rate_engine -f $_SQL
	else
		echo "the db was created before"
	fi
fi
