include ../../config.mk

MOD_NAME = duckdb

# db_duckdb - DuckDB as a db engine (peer of pgsql/mysql/mongodb).
# Produces duckdb.so; the loader binds it via db_duckdb_bind_api().
MOD_LIB = $(MOD_NAME).so

# DuckDB SDK is fetched on demand into lib/ (never committed; see .gitignore).
# Version pinned in ./DUCKDB_VERSION.
DUCKDB_DIR     = lib
DUCKDB_VERSION = $(shell cat DUCKDB_VERSION 2>/dev/null)
DUCKDB_INC     = -I$(DUCKDB_DIR)

# Link mode: dynamic (default) or static.
#   make                     -> dynamic: links libduckdb.so, installs it to modules/
#   make DUCKDB_LINK=static  -> static:  bakes libduckdb_static.a into duckdb.so,
#                                        nothing extra to install at runtime
DUCKDB_LINK ?= dynamic
ifeq ($(DUCKDB_LINK),static)
    DUCKDB_LDLIBS = $(DUCKDB_DIR)/libduckdb_static.a -lstdc++ -lm -ldl -lpthread
    DUCKDB_RPATH  =
else
    DUCKDB_LDLIBS = -L$(DUCKDB_DIR) -lduckdb
    DUCKDB_RPATH  = -Wl,-rpath,'$$ORIGIN'
endif

MOD_SRC = $(shell echo *.c)
MOD_OBJS = $(MOD_SRC:.c=.o)

CFLAGS += $(DUCKDB_INC)

# Fetch the SDK if the header is missing (the zip carries headers + both libs).
$(DUCKDB_DIR)/duckdb.h:
	../../scripts/fetch_duckdb.sh $(DUCKDB_DIR) $(DUCKDB_VERSION)

.PHONY: duckdb-fetch
duckdb-fetch: $(DUCKDB_DIR)/duckdb.h

MOD: $(DUCKDB_DIR)/duckdb.h $(MOD_OBJS)
	$(CC) $(LDFLAGS) -o $(MOD_LIB) $(MOD_OBJS) -L../../ $(CORE_LIB_LD) $(DUCKDB_LDLIBS) $(DUCKDB_RPATH)

all: MOD

%.o: %.c $(DUCKDB_DIR)/duckdb.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	@rm -fv *.o *.so *.d

.PHONY: install
install: MOD
	@if [ ! -d $(MOD) ]; then \
	    mkdir -p $(MOD); \
	fi
	cp -fv $(MOD_LIB) $(MOD)
ifeq ($(DUCKDB_LINK),dynamic)
	cp -fv $(DUCKDB_DIR)/libduckdb.so $(MOD)
endif

.PHONY: uninstall
uninstall:
	@if [ -f $(MOD)/$(MOD_LIB) ]; then \
	    rm -fv $(MOD)/$(MOD_LIB); \
	fi
