#!/usr/bin/env bash
#
# gen_bench_db.sh - build a SYNTHETIC benchmark database for offline rating.
#
# Creates a persistent database (default: re7_bench) from the committed schema
# rt_pgsql.sql and fills it with generated, non-customer data at scale:
#   * 5 tariff plans (bill_plan/tariff/rate/prefix)
#   * N_ACCOUNTS subscribers (billing_account + calling_number + pcard)
#   * N_CDRS unrated leg-A CDRs (random src from the accounts, dst from prefixes)
#
# The result is a stand-in for a real rating DB so the benchmark/parity tools
# (bench_rating_replay.sh, parity_real.sh) can run WITHOUT the real database.
# Those tools take the SOURCE db in $SRCDB (NOT $DBNAME):
#   SRCDB=re7_bench ./bench_rating_replay.sh
#   SRCDB=re7_bench ./parity_real.sh
#
# It NEVER writes to the real DB - the configured DB is used only as the
# maintenance connection to CREATE/DROP the bench DB, and the script refuses to
# use the real DB name as the target.
#
# Env (optional): RE_PREFIX/RE_CONF (to inherit DB creds), DBHOST DBPORT DBUSER
#   DBPASS DBNAME (maintenance/real db), BENCH_DB (target, default re7_bench),
#   N_ACCOUNTS (default 50000), N_CDRS (default 1000000).
#
# Exit: 0 ok, 2 prerequisites/safety failure.

set -u

HERE=$(cd "$(dirname "$0")" && pwd)
REPO_SRC=$(cd "$HERE/../../.." && pwd)           # rating -> tests -> scripts -> src
SCHEMA_SQL="$REPO_SRC/scripts/sql/rt_pgsql.sql"
GEN_SQL="$HERE/gen_bench_db.sql"

RE_PREFIX=${RE_PREFIX:-/usr/local/RateEngine}
RE_CONF=${RE_CONF:-$RE_PREFIX/config/RateEngine7.xml}

db_from_conf() {
	[ -f "$RE_CONF" ] || return 0
	sed -n '/<DB>/,/<\/DB>/p' "$RE_CONF" 2>/dev/null |
		sed -nE "s/.*name=\"$1\"[^>]*value=\"([^\"]*)\".*/\1/p" | head -1
}
DBHOST=${DBHOST:-$(db_from_conf dbhost)}; DBHOST=${DBHOST:-127.0.0.1}
DBNAME=${DBNAME:-$(db_from_conf dbname)}; DBNAME=${DBNAME:-rate_engine}
DBUSER=${DBUSER:-$(db_from_conf dbuser)}; DBUSER=${DBUSER:-re_admin}
DBPASS=${DBPASS:-$(db_from_conf dbpass)}; DBPASS=${DBPASS:-_cfg.access}
DBPORT=${DBPORT:-$(db_from_conf dbport)}; DBPORT=${DBPORT:-5432}

BENCH_DB=${BENCH_DB:-re7_bench}
N_ACCOUNTS=${N_ACCOUNTS:-50000}
N_CDRS=${N_CDRS:-1000000}
export PGPASSWORD="$DBPASS"

die() { echo "gen_bench_db: $*" >&2; exit 2; }

[ -f "$SCHEMA_SQL" ] || die "schema not found: $SCHEMA_SQL"
[ -f "$GEN_SQL" ] || die "generator not found: $GEN_SQL"
command -v psql >/dev/null || die "psql not found in PATH"

# Safety: never target the real/maintenance DB.
if [ "$BENCH_DB" = "$DBNAME" ]; then
	die "refusing to use the real DB name '$DBNAME' as the bench target (set BENCH_DB=...)"
fi

psql_maint() { psql -v ON_ERROR_STOP=1 -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$DBNAME" "$@"; }
psql_bench() { psql -v ON_ERROR_STOP=1 -h "$DBHOST" -p "$DBPORT" -U "$DBUSER" -d "$BENCH_DB" "$@"; }

echo "gen_bench_db: host=$DBHOST:$DBPORT user=$DBUSER  bench_db=$BENCH_DB"
echo "gen_bench_db: scale accounts=$N_ACCOUNTS cdrs=$N_CDRS"
echo "gen_bench_db: WARNING - (re)creating '$BENCH_DB' (existing one is dropped)"

psql_maint -c "DROP DATABASE IF EXISTS $BENCH_DB;" >/dev/null 2>&1 || true
psql_maint -c "CREATE DATABASE $BENCH_DB;" ||
	die "could not CREATE DATABASE $BENCH_DB (does $DBUSER have CREATEDB? is $DBHOST reachable?)"

echo "gen_bench_db: loading schema (rt_pgsql.sql)..."
psql_bench -q -f "$SCHEMA_SQL" >/dev/null || die "failed to load schema"

echo "gen_bench_db: generating data (this can take a minute for 1e6 CDRs)..."
start=$(date +%s)
psql_bench -v n_accounts="$N_ACCOUNTS" -v n_cdrs="$N_CDRS" -f "$GEN_SQL" ||
	die "data generation failed"
gen_s=$(( $(date +%s) - start ))

echo "gen_bench_db: VACUUM ANALYZE (planner stats for representative timings)..."
psql_bench -q -c "VACUUM ANALYZE;" >/dev/null || die "VACUUM ANALYZE failed"

echo
echo "gen_bench_db: done in ${gen_s}s. Bench DB '$BENCH_DB' ready on $DBHOST:$DBPORT."
echo "  use it as the SOURCE db (SRCDB) for the perf/parity tools, e.g.:"
echo "    SRCDB=$BENCH_DB ./bench_rating_replay.sh"
echo "    SRCDB=$BENCH_DB MODULE=rt_duckdb.so ./bench_rating_replay.sh"
echo "    SRCDB=$BENCH_DB ./parity_real.sh"
