#
# * RateEngine7 Makefile config *
#
#   Created by Dimitar Kokov ,dkokov75 at gmail dot com  (2018-11-08)
#
#   History:
#	2018-11-08, create file - separate parts from the Makefile!
#	2018-12-11, use 'xml2-config' 
#

SHELL = /usr/bin/sh
CC    = gcc

XML2_LIB_LD = $(shell xml2-config --libs)
XML2_CFLAGS = $(shell xml2-config --cflags)

FLAGS   = -Wall
CFLAGS  = -fPIC -Ofast 
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
ifeq ($(GCC_VERSION),8)
	
endif

# CPU ARCH
ifeq ($(HOST_ARCH),x86_64)
	CFLAGS += -m64 -march=nocona -mtune=generic -mfpmath=sse -flto -funroll-loops
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

RT_RATES_NOCACHE ?= 0
ifeq ($(RT_RATES_NOCACHE),1)
	CFLAGS += -DRATES_NOCACHE
endif

CORE_DIR = ./

# RE7 Core
CORE_LIB    = $(CORE_DIR)libre7core.so
CORE_LIB_LD = -lre7core


# RateEngine, main()
MAIN_SRC    = RateEngine.c
MAIN_TARGET = RateEngine

PREFIX ?= $(OS_PREFIX)

LIBS_CFG = /etc/ld.so.conf.d/RateEngine.conf

BIN   = $(PREFIX)bin/
LIBS  = $(PREFIX)libs/
MOD   = $(PREFIX)modules/
CFG   = $(PREFIX)config/
SMP   = $(CORE_DIR)config/samples/*
_SMP  = $(CORE_DIR)config/samples/
LOG   = $(PREFIX)logs/
SS    = $(PREFIX)scripts/

# modules with '#' are disabled for compilitaion !
MOD_CONF_FILE = config.md
MODULES = $$(grep -v "\#" $(MOD_CONF_FILE) | sed ":a;N;$!ba;s/\n/ /g")


