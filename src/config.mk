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

PGSQL_INCLUDEDIR = $(shell pg_config --includedir 2>/dev/null)

FLAGS   = -Wall
# pin pre-C23 std: gcc 14 defaults to gnu23, which (with glibc >= 2.38) redirects
# strtol/atof/scanf to __isoc23_* symbols that are missing on older runtime libc
# (e.g. dlopen rt.so -> "undefined symbol: __isoc23_strtol"). gnu17 keeps the GNU
# extensions the code uses but emits the classic, portable libc symbols.
# Optimization level. NOT -Ofast: -Ofast implies -ffast-math, which enables
# -fno-signed-zeros / reassociation and is a correctness hazard in a billing
# engine (prices are double/float; free-billsec tracking relies on -0.0 vs 0.0
# and on 'call_price < 0'). -O2 + strict FP keeps monetary math deterministic.
# Overridable from the command line for A/B builds, e.g. make OPT="-O3".
OPT ?= -O2 -fno-fast-math -ffp-contract=off

# Link-Time Optimization. Must be on BOTH compile and link to trigger the LTO
# recompile; -flto alone in CFLAGS relied on the linker plugin firing implicitly.
# =auto parallelizes the LTO stage across cores. Note: LTO only optimizes within
# the core lib and within each module -- it cannot cross the dlopen/.so + bind-API
# function-pointer boundaries that the rating hot path goes through. Set LTO= to
# disable (e.g. make LTO= for faster debug builds).
LTO ?= -flto=auto

CFLAGS  = -std=gnu17 -fPIC $(OPT) -I/usr/include/libxml2/ -I$(PGSQL_INCLUDEDIR)
LDFLAGS = -shared $(LTO)
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
# RE is always compiled on the host it runs on (binaries are never moved between
# machines), so -march=native tunes each build for its own CPU with full ISA use
# and no portability risk. Overridable, e.g. make MARCH="-march=x86-64-v3".
# (was: -march=nocona -mtune=generic -mavx2 -- an incoherent P4-schedule +
#  bolted-on AVX2 combo. -funroll-loops dropped: -O2/-O3 unroll selectively.)
MARCH ?= -march=native -mtune=native
ifeq ($(HOST_ARCH),x86_64)
	CFLAGS += -m64 $(MARCH) $(LTO)
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

# Extra compile flags, appended last so they win. Empty by default (no effect on
# normal builds). Used by the clang CI job to promote real defects to errors,
# e.g. make CC=clang EXTRA_CFLAGS="-Wall -Werror=return-type".
EXTRA_CFLAGS ?=
CFLAGS += $(EXTRA_CFLAGS)

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


