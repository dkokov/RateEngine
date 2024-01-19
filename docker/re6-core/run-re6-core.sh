#!/bin/bash

#APP_DIR="/usr/local/RateEngine/"
#CLI=$APP_DIR"bin/RateEngine"
#CONF=$APP_DIR"config/RateEngine6.xml"

/re6-core/post-install.sh

#$CLI -c $CONF -d 2>&1 > /dev/null
/usr/local/RateEngine/bin/RateEngine -f

#while true; do
#  echo "Daemon is running..."
#  sleep 2
#done
