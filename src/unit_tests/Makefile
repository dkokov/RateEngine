# RateEngine unit tests
#   make check   - build and run all tests (non-zero exit on failure)
#   make clean   - remove built test binaries

CC     = cc
CFLAGS = -O2 -Wall
LDLIBS = -lm

TESTS = tariff_pricing

.PHONY: check
check: $(TESTS)
	@rc=0; for t in $(TESTS); do echo "== running $$t =="; ./$$t || rc=1; done; exit $$rc

tariff_pricing: tariff_pricing.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

.PHONY: clean
clean:
	@rm -fv $(TESTS)
