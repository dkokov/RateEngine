include ../../config.mk

MOD_NAME = duckdb

# db_duckdb - DuckDB as a db engine (peer of pgsql/mysql/mongodb).
# Produces duckdb.so; the loader binds it via db_duckdb_bind_api().
MOD_LIB = $(MOD_NAME).so

# DuckDB SDK is shared from the RatingDuckDB module for now (single copy of
# the ~70MB lib). rpath=$ORIGIN finds libduckdb.so next to duckdb.so in modules/.
DUCKDB_DIR = ../RatingDuckDB/lib
DUCKDB_LIB = $(DUCKDB_DIR)/libduckdb.so
DUCKDB_INC = -I$(DUCKDB_DIR)

MOD_SRC = $(shell echo *.c)
MOD_OBJS = $(MOD_SRC:.c=.o)

CFLAGS += $(DUCKDB_INC)

MOD: $(MOD_OBJS)
	$(CC) $(LDFLAGS) -o $(MOD_LIB) $(MOD_OBJS) -L../../ $(CORE_LIB_LD) -L$(DUCKDB_DIR) -lduckdb -Wl,-rpath,'$$ORIGIN'

all: MOD

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	@rm -fv *.o *.so *.d

.PHONY: install
install: MOD
	@if [ ! -d $(MOD) ]; then \
	    mkdir -p $(MOD); \
	fi
	@if [ -f $(MOD_LIB) ]; then \
	    cp -fv $(MOD_LIB) $(MOD); \
	    cp -fv $(DUCKDB_LIB) $(MOD); \
	else \
	    echo ""; \
	    echo "Installation ERROR! The file "$(MOD_LIB)" is not found!"; \
	    echo ""; \
	    exit 1; \
	fi

.PHONY: uninstall
uninstall:
	@if [ -f $(MOD)/$(MOD_LIB) ]; then \
	    rm -fv $(MOD)/$(MOD_LIB); \
	fi
