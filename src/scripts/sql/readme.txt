systemctl start postgresql

su - postgres

createuser -s -r global -P
createdb --owner=global rate_engine
psql -h 127.0.0.1 -U global rate_engine -f rate_engine-0.6.16.sql


createdb --owner=global hpvp_cdr
psql -h 127.0.0.1 -U global hpvp_cdr -f fs_cdr.sql
