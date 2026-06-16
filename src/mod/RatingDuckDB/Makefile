include ../../config.mk

MOD_NAME = rt

# RatingDuckDB - DuckDB analytical batch rating (produces rt.so, drop-in)
MOD_LIB = $(MOD_NAME).so

DUCKDB_DIR = lib
DUCKDB_LIB = $(DUCKDB_DIR)/libduckdb.so
DUCKDB_INC = -I$(DUCKDB_DIR)

MOD_SRC = rating_duckdb.c rt_duckdb.c
MOD_OBJS = $(MOD_SRC:.c=.o)

CFLAGS += $(DUCKDB_INC)

MOD: $(MOD_OBJS)
	$(CC) $(LDFLAGS) -o $(MOD_LIB) $(MOD_OBJS) -L../../ $(CORE_LIB_LD) -L$(DUCKDB_DIR) -lduckdb -Wl,-rpath,'$$ORIGIN/lib'

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
