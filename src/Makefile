#
# * RateEngine6 Makefile *
#
#   Created by Dimitar Kokov ,dkokov75 at gmail dot com  (2017-11-22)
#
#   History:
#	2018-01-17, edit 'RATING_LIB,CC_LIB,CDRM_LIB,CODE_LIB' to use main directory ;
#		  , add '-L.' when use '-lrating -lre6cc -lre6core -lcdrmediator'
#
#       2018-02-13, add '@' in the 'uninstall' part ;
#       2018-02-13, add '@' in the 'clean' part ;
#       2018-02-13, add '@' in the 'install' part ;
#       2018-02-16, improve Makefile 
#       		(add 'OS','HOST_ARCH','LDLIBS','OS_PREFIX','LDFLAGS',new gcc options in 'CFLAGS'
#       		)
#       2018-02-19,
#
#       2018-03-24, add make option 'CC_LOOP=1/0' for GCC option '-DCC_NOLOOP' 
#
#	2023-05-20, add 'xml-config' and 'pg_config' to get 'xml2' and 'pq' pathes for 'include' files

SHELL = /bin/sh
CC    = gcc

XML2_CFLAGS = $(shell xml2-config --cflags)

#PGSQL_DIR="/usr/include/"
PGSQL_DIR=$(shell pg_config --includedir)
PGSQL_H= $(PGSQL_DIR)

FLAGS   = -Wall
CFLAGS  = -fPIC -Ofast $(XML2_CFLAGS) -I$(PGSQL_H)
LDFLAGS = -shared
LDLIBS  = -lpq

# uname -p , print the processor type (non-portable)
# uname -s , print the kernel name
# uname -i , print the hardware platform (non-portable) 
# uname -m , print the machine hardware name

OS = $(shell uname -s)
ifeq ($(OS),Linux)
	OS_PREFIX = /usr/local/RateEngine/
	HOST_ARCH = $(shell uname -m)
	LDLIBS += -lpthread
endif

# GCC version
GCC_VERSION = $(shell gcc -dumpversion)
ifeq ($(GCC_VERSION),7)

endif

# CPU ARCH
ifeq ($(HOST_ARCH),x86_64)
	CFLAGS += -m64 -march=nocona -mtune=generic -mfpmath=sse -flto -funroll-loops
#	CFLAGS += -m64 -mfpmath=sse -flto -march=native -funroll-loops
# 2019-10-16,
#	CFLAGS += -m64 -mtune=generic -flto -funroll-loops -mavx2
else
ifeq ($(HOST_ARCH),i386)
	CFLAGS += -m32
endif
endif

DEBUG ?= 0
ifeq ($(DEBUG),1)
    FLAGS     += -g
    CFLAGS    += $(FLAGS) -DDEBUG_MEM
    LDFLAGS   += $(FLAGS)
endif

CC_NOLOOP ?= 0
ifeq ($(CC_NOLOOP),1)
	CFLAGS += -DCC_NOLOOP
endif


#ifeq ($(PGSQL_H),)
#    echo "DEBUG TEST"
#endif

all: Rating CDRMediator CallControl RE6Core RateEngine

CORE_DIR = ./

# Rating
RATING_DIR = Rating/
RATING_LIB  = $(CORE_DIR)librating.so
RT_SRC = $(shell echo $(RATING_DIR)*.c)
RT_OBJS = $(RT_SRC:.c=.o)
depends = $(RT_SRC:.c=.d)

Rating: $(RT_OBJS) 
	$(CC) $(LDFLAGS) -o $(RATING_LIB) $(RT_OBJS)

# CDRMediator
CDRM_DIR = CDRMediator/
CDRM_LIB  = $(CORE_DIR)libcdrmediator.so
CDRM_SRC = $(shell echo $(CDRM_DIR)*.c)
#CDRM_SRC = $(wildcars $(CDRM_DIR)*.c)
CDRM_OBJS = $(CDRM_SRC:.c=.o)
depends  += $(CDRM_SRC:.c=.d)

CDRMediator: $(CDRM_OBJS) 
	$(CC) $(LDFLAGS) -o $(CDRM_LIB) $(CDRM_OBJS)

# CallControl
CC_DIR = CallControl/
CC_LIB  = $(CORE_DIR)libre6cc.so
CC_SRC = $(shell echo $(CC_DIR)*.c)
CC_OBJS = $(CC_SRC:.c=.o)

# # MyCCv1
MY_CC_SRC = $(shell echo $(CC_DIR)my_cc/*.c)
MY_CC_OBJS = $(MY_CC_SRC:.c=.o)

CallControl: $(MY_CC_OBJS) $(CC_OBJS) 
	$(CC) $(LDFLAGS) -o $(CC_LIB) $(MY_CC_OBJS) $(CC_OBJS)

# RE6 Core
CORE_LIB  = $(CORE_DIR)libre6core.so

# # Config
CFG_SRC = $(shell echo $(CORE_DIR)config/*.c)
CFG_OBJS = $(CFG_SRC:.c=.o)

# # Syslog
LOG_SRC = $(shell echo $(CORE_DIR)syslog/*.c)
LOG_OBJS = $(LOG_SRC:.c=.o)

# # DB
DB_SRC = $(shell echo $(CORE_DIR)DB/*.c)
DB_OBJS = $(DB_SRC:.c=.o)

# # Misc
MISC_SRC = $(shell echo $(CORE_DIR)misc/*.c)
MISC_OBJS = $(MISC_SRC:.c=.o)

# # # Misc/TCP
TCP_SRC = $(shell echo $(CORE_DIR)misc/tcp/*.c)
TCP_OBJS = $(TCP_SRC:.c=.o)

# # # Misc/Exten
EXTEN_SRC = $(shell echo $(CORE_DIR)misc/exten/*.c)
EXTEN_OBJS = $(EXTEN_SRC:.c=.o)

# # # Misc/Mem
MEM_SRC = $(shell echo $(CORE_DIR)misc/mem/*.c)
MEM_OBJS = $(MEM_SRC:.c=.o)
#MEM_OBJS = $(CORE_DIR)misc/mem/mem.c
#	$(CC) -c $(CORE_DIR)misc/mem/mem.c

# # # Misc/Proc
PROC_SRC = $(shell echo $(CORE_DIR)misc/proc/*.c)
PROC_OBJS = $(PROC_SRC:.c=.o)

# # # Misc/Stat
STAT_SRC = $(shell echo $(CORE_DIR)misc/stat/*.c)
STAT_OBJS = $(STAT_SRC:.c=.o)

RE6Core: $(CFG_OBJS) $(LOG_OBJS) $(DB_OBJS) $(MISC_OBJS) $(TCP_OBJS) $(EXTEN_OBJS) $(PROC_OBJS) $(STAT_OBJS) $(MEM_OBJS)
	$(CC) $(LDFLAGS) -o $(CORE_LIB) $(CFG_OBJS) $(LOG_OBJS) $(DB_OBJS) $(MISC_OBJS) $(TCP_OBJS) $(EXTEN_OBJS) $(MEM_OBJS) \
	$(PROC_OBJS) $(STAT_OBJS) -lxml2

# RateEngine6,main()
MAIN_SRC = RateEngine.c
MAIN_OBJ = $(MAIN_SRC:.c=.o)
MAIN_TARGET = RateEngine

RateEngine: $(MAIN_OBJ)
	$(CC) $(FLAGS) -o $(MAIN_TARGET) $(MAIN_OBJ) $(LDLIBS) -L. -lre6core -lre6cc -lrating -lcdrmediator


.PHONY: clean
clean:
	@rm -fv $(RATING_LIB) $(RT_OBJS) $(CDRM_LIB) $(CDRM_OBJS) $(CC_LIB) $(CC_OBJS) $(MY_CC_OBJS) \
	$(CORE_LIB) $(CFG_OBJS) $(LOG_OBJS) $(DB_OBJS) $(MISC_OBJS) $(TCP_OBJS) $(EXTEN_OBJS) $(MEM_OBJS) \
	$(PROC_OBJS) $(STAT_OBJS) $(MAIN_OBJ) $(MAIN_TARGET)

.PHONY: help
help:
	-@echo ""
	-@echo " Help for RateEngine make commands:"
	-@echo "  make                             - compiletion with gcc standart flags"
	-@echo "  make DEBUG=1                     - compiletion with gcc debug flags"
	-@echo "  make clean                       - remove all object,share libs and exe files from the src dir"
	-@echo "  make install                     - instalation with default PREFIX=/usr/local/RateEngine/"
	-@echo "  make install PREFIX=/usr/local   - instalation in define path in PREFIX="
	-@echo "  make uninstall                   - uninstalation from default path,PREFIX=/usr/local/RateEngine/"
	-@echo "  make uninstall PREFIX=/usr/local - uninstalation from define path in PREFIX="
	-@echo ""

PREFIX ?= $(OS_PREFIX)

LIBS_CFG = /etc/ld.so.conf.d/RateEngine.conf

BIN  = $(PREFIX)bin/
LIBS = $(PREFIX)libs/
CFG  = $(PREFIX)config/
SMP  = $(CORE_DIR)config/samples/*
_SMP  = $(CORE_DIR)config/samples/
LOG  = $(PREFIX)logs/
SS   = $(PREFIX)scripts/

.PHONY: install
install: RateEngine
	@if [ ! -d $(BIN) ]; then \
	    mkdir -p $(BIN); \
	fi
	@if [ -f $(MAIN_TARGET) ]; then \
	    cp -fv $(MAIN_TARGET) $(BIN); \
	else \
	    echo ""; \
	    echo "Installation ERROR! The file "$(MAIN_TARGET)" is not found!"; \
	    echo ""; \
	    exit 1; \
	fi
	@if [ ! -d $(LIBS) ]; then \
	    mkdir -p $(LIBS); \
	fi
	@if [ -f $(RATING_LIB) ]; then \
	    cp -fv $(RATING_LIB) $(LIBS); \
	else \
	    echo ""; \
	    echo "Installation ERROR! The file "$(RATING_LIB)" is not found!"; \
	    echo ""; \
	    exit 1; \
	fi
	@if [ -f $(CORE_LIB) ]; then \
	    cp -fv $(CORE_LIB) $(LIBS); \
	else \
	    echo ""; \
	    echo "Installation ERROR! The file "$(CORE_LIB)" is not found!"; \
	    echo ""; \
	    exit 1; \
	fi
	@if [ -f $(CC_LIB) ]; then \
	    cp -fv $(CC_LIB) $(LIBS); \
	else \
	    echo ""; \
	    echo "Installation ERROR! The file "$(CC_LIB)" is not found!"; \
	    echo ""; \
	    exit 1; \
	fi
	@if [ -f $(CDRM_LIB) ]; then \
	    cp -fv $(CDRM_LIB) $(LIBS); \
	else \
	    echo ""; \
	    echo "Installation ERROR! The file "$(CDRM_LIB)" is not found!"; \
	    echo ""; \
	    exit 1; \
	fi
	@if [ ! -f $(LIBS_CFG) ]; then \
	    echo $(LIBS) > $(LIBS_CFG); \
	    ldconfig; \
	fi
	@if [ ! -d $(CFG) ]; then \
	    mkdir -p $(CFG); \
	    cp -vR $(SMP) $(CFG);\
	else \
	    if [ -z "$(ls -A $CFG)" ]; then \
		cp -vR $(SMP) $(CFG); \
	    else \
		cp -vfR $(_SMP) $(CFG); \
	    fi \
	fi
	@if [ ! -d $(LOG) ]; then \
	    mkdir -p $(LOG); \
	fi
	@if [ ! -d $(SS) ]; then \
	    mkdir -p $(SS); \
	    cp -vR $(CORE_DIR)scripts/* $(SS); \
	fi


.PHONY: uninstall
uninstall:
	@if [ -d $(PREFIX) ]; then \
	    rm -fvR $(PREFIX); \
	fi
	@if [ -f $(LIBS_CFG) ]; then \
	    rm -fv $(LIBS_CFG); \
	fi


