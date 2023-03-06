#
# https://www.tutorialspoint.com/makefile
#
#
# * RateEngine7 Makefile *
#
#   Created by Dimitar Kokov ,dkokov75 at gmail dot com  (2018-11-07)
#
#   History:
#	2018-11-07, create file, RE6 based ! 
#	2018-11-09, include and use 'config.mk' file
#	2018-11-16, include and use 'config.md' file (modules config/compile/install)
#	2018-12-11, use '$(XML2_LIB_LD)'

include config.mk

all: RE7Core RateEngine

# Mem
MEM_SRC = $(shell echo $(CORE_DIR)mem/*.c)
MEM_OBJS = $(MEM_SRC:.c=.o)

# Proc
PROC_SRC = $(shell echo $(CORE_DIR)proc/*.c)
PROC_OBJS = $(PROC_SRC:.c=.o)

# DB
DB_SRC = $(shell echo $(CORE_DIR)db/*.c)
DB_OBJS = $(DB_SRC:.c=.o)

# MOD
MOD_SRC = $(shell echo $(CORE_DIR)mod/*.c)
MOD_OBJS = $(MOD_SRC:.c=.o)

# NET
NET_SRC = $(shell echo $(CORE_DIR)net/*.c)
NET_OBJS = $(NET_SRC:.c=.o)

# Config
CFG_SRC = $(shell echo $(CORE_DIR)config/*.c)
CFG_OBJS = $(CFG_SRC:.c=.o)

# log
LOG_SRC = $(shell echo $(CORE_DIR)log/*.c)
LOG_OBJS = $(LOG_SRC:.c=.o)

# Misc
MISC_SRC = $(shell echo $(CORE_DIR)misc/*.c)
MISC_OBJS = $(MISC_SRC:.c=.o)

# # Misc/Exten
EXTEN_SRC = $(shell echo $(CORE_DIR)misc/exten/*.c)
EXTEN_OBJS = $(EXTEN_SRC:.c=.o)

# # Misc/Stat
STAT_SRC = $(shell echo $(CORE_DIR)misc/stat/*.c)
STAT_OBJS = $(STAT_SRC:.c=.o)

RE7Core: $(CFG_OBJS) $(LOG_OBJS) $(DB_OBJS) $(MISC_OBJS) $(MOD_OBJS) $(EXTEN_OBJS) $(PROC_OBJS) $(STAT_OBJS) $(MEM_OBJS) $(NET_OBJS)
	$(CC) $(LDFLAGS) -o $(CORE_LIB) $(CFG_OBJS) $(LOG_OBJS) $(DB_OBJS) $(MISC_OBJS) $(MOD_OBJS) $(EXTEN_OBJS) $(MEM_OBJS) $(NET_OBJS) \
	$(PROC_OBJS) $(STAT_OBJS) $(XML2_LIB_LD) -ljson-c

# RateEngine6,main()
MAIN_OBJ = $(MAIN_SRC:.c=.o)

RateEngine: $(MAIN_OBJ)
	$(CC) $(FLAGS) -o $(MAIN_TARGET) $(MAIN_OBJ) -L. -lre7core

# RateEngine's modules
name ?= 
mod_name = mod/$(name)

module: RE7Core
	@set -e; \
	if [ -n "$(name)" ]; then \
	    if [ -d "$(mod_name)" ]; then \
		echo  "Compiling $(mod_name) ..."; \
		echo  "" ; \
		$(MAKE) -j -C $(mod_name) ; \
		echo "" ; \
	    else \
		echo "" ; \
		echo "Module dir name isn't exist!"; \
		echo "" ; exit 20; \
	    fi ; \
	else \
	    echo "" ; \
	    echo "Module name is empty!"; \
	    echo "" ; exit 19; \
	fi ; \

module_install: 
	@set -e; \
	if [ -n "$(name)" ]; then \
	    if [ -d "$(mod_name)" ]; then \
		echo  "Instaling $(mod_name) ..."; \
		echo  "" ; \
		$(MAKE) -j -C $(mod_name) install ; \
		echo "" ; \
	    else \
		echo "" ; \
		echo "Module dir name isn't exist!"; \
		echo "" ; exit 20; \
	    fi ; \
	else \
	    echo "" ; \
	    echo "Module name is empty!"; \
	    echo "" ; exit 19; \
	fi ; \

module_uninstall: 
	@set -e; \
	if [ -n "$(name)" ]; then \
	    if [ -d "$(mod_name)" ]; then \
		echo  "Uninstaling $(mod_name) ..."; \
		echo  "" ; \
		$(MAKE) -j -C $(mod_name) uninstall ; \
		echo "" ; \
	    else \
		echo "" ; \
		echo "Module dir name isn't exist!"; \
		echo "" ; exit 20; \
	    fi ; \
	else \
	    echo "" ; \
	    echo "Module name is empty!"; \
	    echo "" ; exit 19; \
	fi ; \

modules: RE7Core
	@set -e; \
	for tmp in $(MODULES) "" ; do \
	if [ -n "$$tmp" ]; then \
	    if [ -d "$$tmp" ]; then \
		echo  "Compiling $$tmp ..."; \
		echo  "" ; \
		$(MAKE) -j -C $$tmp ; \
		echo "" ; \
	    fi ; \
	fi ; \
	done

modules_install:
	@set -e; \
	for tmp in $(MODULES) "" ; do \
	if [ -n "$$tmp" ]; then \
	    if [ -d "$$tmp" ]; then \
		echo  "Instaling $$tmp ..."; \
		echo  "" ; \
		$(MAKE) -j -C $$tmp install; \
		echo "" ; \
	    fi ; \
	fi ; \
	done

modules_uninstall:
	@set -e; \
	for tmp in $(MODULES) "" ; do \
	if [ -n "$$tmp" ]; then \
	    if [ -d "$$tmp" ]; then \
		echo  "Uninstaling $$tmp ..."; \
		echo  "" ; \
		$(MAKE) -j -C $$tmp uninstall; \
		echo "" ; \
	    fi ; \
	fi ; \
	done

modules_clean:
	@set -e; \
	for tmp in $(MODULES) "" ; do \
	if [ -n "$$tmp" ]; then \
	    if [ -d "$$tmp" ]; then \
		echo  "Cleaning $$tmp ..."; \
		echo  "" ; \
		$(MAKE) -j -C $$tmp clean; \
		echo "" ; \
	    fi ; \
	fi ; \
	done


.PHONY: clean 
clean: modules_clean
	@rm -fv $(CORE_LIB) $(CFG_OBJS) $(LOG_OBJS) $(DB_OBJS) $(MISC_OBJS) $(MOD_OBJS) $(EXTEN_OBJS) $(MEM_OBJS) \
	$(PROC_OBJS) $(STAT_OBJS) $(NET_OBJS) $(MAIN_OBJ) $(MAIN_TARGET)

.PHONY: help
help:
	-@echo ""
	-@echo " Help for RateEngine make commands:"
	-@echo "  make                             - compiletion with gcc standart flags"
	-@echo "  make DEBUG=1                     - compiletion with gcc debug flags"
	-@echo "  make clean                       - remove all object,share libs and exe files from the src dir"
	-@echo "  make module name=                - "
	-@echo "  make module_install name=        - "
	-@echo "  make modules                     - "
	-@echo "  make modules_install             - "
	-@echo "  make modules_uninstall           - "
	-@echo "  make install                     - instalation with default PREFIX=/usr/local/RateEngine/"
	-@echo "  make install PREFIX=/usr/local   - instalation in define path in PREFIX="
	-@echo "  make uninstall                   - uninstalation from default path,PREFIX=/usr/local/RateEngine/"
	-@echo "  make uninstall PREFIX=/usr/local - uninstalation from define path in PREFIX="
	-@echo ""

.PHONY: install
install: RE7Core RateEngine modules_install
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
	@if [ -f $(CORE_LIB) ]; then \
	    cp -fv $(CORE_LIB) $(LIBS); \
	else \
	    echo ""; \
	    echo "Installation ERROR! The file "$(CORE_LIB)" is not found!"; \
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
	    cp -vfR $(_SMP) $(CFG);\
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


