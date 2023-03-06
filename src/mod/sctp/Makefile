include ../../config.mk

MOD_NAME = sctp

# TLS
MOD_LIB = $(MOD_NAME).so
MOD_SRC = $(shell echo *.c)
MOD_OBJS = $(MOD_SRC:.c=.o)

MOD: $(MOD_OBJS) 
	$(CC) $(LDFLAGS) -o $(MOD_LIB) $(MOD_OBJS) -lsctp -L../../ $(CORE_LIB_LD)

all: MOD

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
