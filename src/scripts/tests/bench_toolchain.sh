#!/usr/bin/env bash
#
# bench_toolchain.sh - build RateEngine with two toolchains and compare
#                      build time, rating correctness, and (optionally) runtime.
#
# Motivation: config.mk now exposes CC / OPT / MARCH / LTO as overridable knobs.
# This harness builds the whole engine + modules with GCC and with Clang (each
# with its own LTO flavor: gcc -> -flto=auto, clang -> -flto=thin), installs each
# into its OWN throwaway prefix, then for each build:
#   * records the clean-build wall-clock time,
#   * runs run_rating_regression.sh (per-CDR price/billsec vs golden.tsv) so a
#     flag change that alters the MONEY is caught immediately,
#   * optionally runs cc_loadtest.sh for CallControl throughput (opt-in, needs a
#     live listener; see RUN_LOADTEST below).
# Finally it diffs the two builds' rated prices against each other - the two
# toolchains must produce identical billing, not just each pass golden.
#
# This never touches a production install or the engine's real DB: it installs
# into mktemp prefixes and the regression uses its own throwaway test database.
#
# Env (all optional):
#   TOOLCHAINS   space list of "label" builds to run   (default "gcc clang")
#   OPT          optimization flags applied to BOTH     (default = config.mk's)
#   MARCH        -march flags applied to both           (default = config.mk's)
#   RE_MAKE_JOBS -j value for module builds             (default nproc)
#   DBHOST/DBNAME/DBUSER/DBPASS/DBPORT   passed to the rating regression
#   RUN_LOADTEST=1   also run cc_loadtest.sh (requires a running CallControl;
#                    set LOADTEST_ARGS="host port total conc ...")
#   RE_REAL_CONF DB creds are auto-read from this real install config when not
#                passed via env (default /usr/local/RateEngine/config/RateEngine7.xml).
#                Precedence: your DB* env vars > this config > the regression's defaults.
#   KEEP=1       keep the temp prefixes/logs instead of removing them
#
# Exit: 0 = every build compiled AND passed golden AND the builds agree on price.
#       1 = a build failed, a golden mismatch, or a cross-toolchain price diff.
set -u

HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "$HERE/../.." && pwd)                 # tests -> scripts -> src
REGRESSION="$HERE/rating/run_rating_regression.sh"

TOOLCHAINS=${TOOLCHAINS:-"gcc clang"}
RE_MAKE_JOBS=${RE_MAKE_JOBS:-$(nproc 2>/dev/null || echo 4)}
KEEP=${KEEP:-0}
RUN_LOADTEST=${RUN_LOADTEST:-0}

# OPT / MARCH: empty means "let config.mk decide" (its ?= defaults apply).
OPT=${OPT:-}
MARCH=${MARCH:-}

# DB creds fallback: the harness installs each build into a THROWAWAY prefix, so
# the regression would otherwise read that prefix's SAMPLE config (placeholders
# like "your_hostname"). To spare the caller from typing creds every run, read
# any unset DB* var from the REAL install config. Same <DB> extraction the
# regression uses. Explicit env always wins; whatever stays unset falls through
# to the regression's own defaults.
RE_REAL_CONF=${RE_REAL_CONF:-/usr/local/RateEngine/config/RateEngine7.xml}
conf_db() {
	[ -f "$RE_REAL_CONF" ] || return 0
	sed -n '/<DB>/,/<\/DB>/p' "$RE_REAL_CONF" 2>/dev/null |
		sed -nE "s/.*name=\"$1\"[^>]*value=\"([^\"]*)\".*/\1/p" | head -1
}
if [ -f "$RE_REAL_CONF" ]; then
	DBHOST=${DBHOST:-$(conf_db dbhost)}
	DBNAME=${DBNAME:-$(conf_db dbname)}
	DBUSER=${DBUSER:-$(conf_db dbuser)}
	DBPASS=${DBPASS:-$(conf_db dbpass)}
	DBPORT=${DBPORT:-$(conf_db dbport)}
	# Export only the ones we actually resolved, so the regression inherits them
	# (an empty value must NOT shadow the regression's default -> skip blanks).
	for v in DBHOST DBNAME DBUSER DBPASS DBPORT; do
		[ -n "${!v:-}" ] && export "$v"
	done
	echo "bench: DB creds (env > $RE_REAL_CONF): host=${DBHOST:-<default>} db=${DBNAME:-<default>} user=${DBUSER:-<default>}"
else
	echo "bench: no real config at $RE_REAL_CONF - relying on DB* env / regression defaults"
fi

WORK=$(mktemp -d "${TMPDIR:-/tmp}/re7_bench.XXXXXX") || { echo "mktemp failed" >&2; exit 2; }
cleanup() { [ "$KEEP" = "1" ] || rm -rf "$WORK"; }
trap cleanup EXIT

echo "bench: toolchains='$TOOLCHAINS'  jobs=$RE_MAKE_JOBS  work=$WORK"
[ -x "$REGRESSION" ] || { echo "missing $REGRESSION" >&2; exit 2; }

# lto_for CC -> the LTO flavor that CC understands.
lto_for() {
	case "$1" in
	clang*) echo "-flto=thin" ;;
	*) echo "-flto=auto" ;;
	esac
}

# make_flags CC -> the CC=/OPT=/MARCH=/LTO= overrides for this build.
make_flags() {
	local cc=$1
	local -a f=("CC=$cc" "LTO=$(lto_for "$cc")")
	[ -n "$OPT" ] && f+=("OPT=$OPT")
	[ -n "$MARCH" ] && f+=("MARCH=$MARCH")
	printf '%s\n' "${f[@]}"
}

FAILED=0
declare -A BUILD_SECS
PRICEFILES=()

for cc in $TOOLCHAINS; do
	echo
	echo "=============================================================="
	echo " BUILD: $cc"
	echo "=============================================================="
	command -v "$cc" >/dev/null 2>&1 || { echo "  SKIP: $cc not in PATH (install it to include in the comparison)"; continue; }

	mapfile -t MF < <(make_flags "$cc")
	prefix="$WORK/prefix_$cc/"
	log="$WORK/build_$cc.log"
	echo "  flags: ${MF[*]}"
	echo "  prefix: $prefix"

	# Clean build, timed. Built in dependency order with SEPARATE make calls:
	# the aggregate targets `all`/`install` list RE7Core and RateEngine as
	# unordered siblings, and RateEngine links -lre7core, so `make -j all` races
	# (exe links before libre7core.so exists). So: build the core lib first (-j is
	# safe here - its object prereqs are independent, the .so link waits on all of
	# them), THEN the main exe, THEN modules (-j), THEN install.
	( cd "$SRC" && make "${MF[@]}" clean ) >>"$log" 2>&1
	t0=$(date +%s.%N)
	if ! ( cd "$SRC" \
		&& make -j"$RE_MAKE_JOBS" "${MF[@]}" RE7Core >>"$log" 2>&1 \
		&& make "${MF[@]}" RateEngine >>"$log" 2>&1 \
		&& make -j"$RE_MAKE_JOBS" "${MF[@]}" modules >>"$log" 2>&1 \
		&& make "${MF[@]}" PREFIX="$prefix" install >>"$log" 2>&1 \
		&& make "${MF[@]}" PREFIX="$prefix" modules_install >>"$log" 2>&1 ); then
		echo "  FAIL: build/install ($cc) - tail of $log:"
		tail -n 25 "$log" | sed 's/^/    /'
		FAILED=1
		continue
	fi
	t1=$(date +%s.%N)
	BUILD_SECS[$cc]=$(awk "BEGIN{printf \"%.1f\", $t1-$t0}")
	echo "  build+install OK in ${BUILD_SECS[$cc]}s"

	# Rating correctness vs golden. Capture stdout; extract per-CDR prices so we
	# can also diff toolchains against each other.
	reglog="$WORK/regression_$cc.log"
	if RE_PREFIX="${prefix%/}" bash "$REGRESSION" >"$reglog" 2>&1; then
		echo "  rating regression: PASS (vs golden.tsv)"
	else
		echo "  rating regression: FAIL - tail of $reglog:"
		tail -n 20 "$reglog" | sed 's/^/    /'
		FAILED=1
	fi

	# Normalize "<label> <uid>: price <got> ..." lines -> "uid got" for parity.
	pf="$WORK/prices_$cc.txt"
	awk '/rt\.so [0-9]+: price /{u=$3; sub(/:$/,"",u); print u, $5}' "$reglog" \
		| sort >"$pf"
	PRICEFILES+=("$pf")

	if [ "$RUN_LOADTEST" = "1" ]; then
		echo "  cc_loadtest: (RUN_LOADTEST=1) - needs a running CallControl listener"
		# shellcheck disable=SC2086
		bash "$HERE/cc_loadtest.sh" ${LOADTEST_ARGS:-} || echo "  cc_loadtest returned nonzero"
	fi
done

echo
echo "==================== toolchain comparison ===================="
for cc in $TOOLCHAINS; do
	printf "  %-8s build=%ss\n" "$cc" "${BUILD_SECS[$cc]:-n/a}"
done

# Cross-toolchain price equality: every pair of price files must be identical.
if [ "${#PRICEFILES[@]}" -ge 2 ]; then
	ref="${PRICEFILES[0]}"
	for pf in "${PRICEFILES[@]:1}"; do
		if diff -q "$ref" "$pf" >/dev/null 2>&1; then
			echo "  price parity: $(basename "$ref") == $(basename "$pf")  OK"
		else
			echo "  price parity: MISMATCH between $(basename "$ref") and $(basename "$pf"):"
			diff "$ref" "$pf" | sed 's/^/    /'
			FAILED=1
		fi
	done
else
	echo "  price parity: only one build produced results - nothing to compare"
fi
echo "=============================================================="

[ "$FAILED" -eq 0 ] && { echo "RESULT: OK"; exit 0; } || { echo "RESULT: FAIL"; exit 1; }
