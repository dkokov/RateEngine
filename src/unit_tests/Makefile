# RateEngine tests
#
#   make              - build + run the standalone unit tests (alias of `check`)
#   make check        - build + run self-asserting unit tests; non-zero exit on failure
#   make integration  - build the known-good core-linked tests (needs ../libre7core.so)
#   make integration-extra - build the manual (argv-driven) core-linked tests
#   make all          - run `check` and build `integration`
#   make clean        - remove all built test binaries
#
# Two classes of test live here:
#   UNIT        - self-contained, link only libc/libm, assert via exit code.
#                 These are what `make check` runs in CI.
#   INTEGRATION - link the built core lib (../libre7core.so) + json-c; they
#                 exercise real engine code and are usually run by hand
#                 (most take argv / print output rather than self-asserting).
#

CC       = cc
CFLAGS   = -O2 -Wall

# --- core lib used by the integration tests ---
CORE     = ..
CORE_LIB = $(CORE)/libre7core.so
XML2_CFLAGS = $(shell xml2-config --cflags 2>/dev/null)
# -I.. lets sources include "misc/...", "Rating/..." etc. without a ../ prefix;
# -rpath lets the binaries find libre7core.so when run from this directory.
INT_CFLAGS = -g -Wall -I$(CORE) $(XML2_CFLAGS)
INT_LDLIBS = -L$(CORE) -Wl,-rpath,$(CORE) -lre7core -ljson-c

# ---- self-asserting unit tests (run by `make check`) ----
UNIT  = tariff_pricing cdr_ingest_safety

# ---- core-linked tests, known-good (built by `make integration`) ----
# This is the set the old src/tests/Makefile built by default.
INTEGRATION = \
	json_ext_test json_ext_test_2 json_ext_test_2b json_ext_cdrm \
	mem_test mem_test_arr mem_test_stack

# ---- core-linked tests, NOT built by default (`make integration-extra`) ----
# These compile against the current core lib but were never in the old default
# build: they need manual argv and don't self-assert, so they're driven by hand.
# (Dead/uncompilable tests live in _old/ — gitignored, not part of any target.)
INTEGRATION_EXTRA = \
	json_rpc_test_2 \
	xml_cfg_test xml_cfg_child_test

.PHONY: check
check: $(UNIT)
	@rc=0; for t in $(UNIT); do echo "== running $$t =="; ./$$t || rc=1; done; exit $$rc

# Self-asserting unit tests: standalone, libm only.
$(UNIT): %: %.c
	$(CC) $(CFLAGS) -o $@ $< -lm

.PHONY: integration
integration: $(INTEGRATION)

.PHONY: integration-extra
integration-extra: $(INTEGRATION_EXTRA)

# Core-linked tests: require the built core library.
$(INTEGRATION) $(INTEGRATION_EXTRA): %: %.c $(CORE_LIB)
	$(CC) $(INT_CFLAGS) -o $@ $< $(INT_LDLIBS)

$(CORE_LIB):
	@echo "error: $(CORE_LIB) not built — run 'make' in src/ first" >&2; exit 1

.PHONY: all
all: check integration

.PHONY: clean
clean:
	@rm -fv $(UNIT) $(INTEGRATION) $(INTEGRATION_EXTRA)
