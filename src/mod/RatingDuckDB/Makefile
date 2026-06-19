include ../../config.mk

MOD_NAME = rt

# RatingDuckDB - analytical batch rating (produces rt.so, drop-in for Rating).
# DuckDB is now reached through the db_duckdb engine (duckdb.so), so this
# module no longer links libduckdb directly. It requires duckdb.so to be
# built/installed and loaded at runtime.
MOD_LIB = $(MOD_NAME).so

MOD_SRC = rating_duckdb.c rt_duckdb.c
MOD_OBJS = $(MOD_SRC:.c=.o)

MOD: $(MOD_OBJS)
	$(CC) $(LDFLAGS) -o $(MOD_LIB) $(MOD_OBJS) -L../../ $(CORE_LIB_LD)

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
