
# Build system for COPRIS
# Possible common targets (you don't need to specify one for a regular release build):
#	- release   build the release build (executable name `copris')
#	- debug     build the debugging build (executable name `copris_dbg')
#	- install   install a copy of release build with samples and documentation
#	- clean     remove temporary object, dependency, binary and test files
#	- help      print this text

# Run `make' with `WITHOUT_CMARK=1' to omit Markdown support.

# Code analysis targets:
#	- check                   build and run unit tests
#	- analyse                 same as `debug', but compile with GCC's static analyser
#	- analyse-cppcheck        analyse codebase with Cppcheck, print results to stdout
#	- analyse-cppcheck-html   analyse codebase with Cppcheck, generate a HTML report

# GNU Make docs: https://www.gnu.org/software/make/manual/html_node/index.html

# Get latest version tag if in a git repository, else from a local file
VERSION := $(shell git describe --tags --dirty 2>/dev/null || cat VERSION)

# Installation directories
PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share
MANDIR  ?= $(PREFIX)/man
DESTDIR ?=

INSTALL ?= install -p

# Default common, release and debug build compiler flags
CFLAGS   ?= -Wall -Wextra -Wstrict-prototypes -Wshadow -Wundef -pedantic
RELFLAGS ?= -O2 -g -DNDEBUG
DBGFLAGS ?= -Og -g3 -ggdb -gdwarf -DDEBUG

# -Wconversion

# Dynamic libraries to be linked
LIBRARIES = inih

# Source files (don't forget printerset.c being added below)
SOURCES = read_socket.c read_stdin.c writer.c translate.c utf8.c \
          parse_value.c main.c

# Include Markdown support and link with libcmark if not requested otherwise
ifndef WITHOUT_CMARK
	CFLAGS    += -DW_CMARK
	LIBRARIES += libcmark
	SOURCES   += printerset.c
endif

# Additional compiler and linker library flags + version string
# (it is here because it could be overrided above)
CFLAGS  += $(shell pkg-config --cflags $(LIBRARIES)) -DVERSION=\"$(VERSION)\"
LDFLAGS += $(shell pkg-config --libs $(LIBRARIES))

# Object and dependency files for release and debug builds
OBJS_REL := $(SOURCES:%.c=src/%_rel.o)
DEPS_REL := $(SOURCES:%.c=src/%_rel.d)
OBJS_DBG := $(SOURCES:%.c=src/%_dbg.o)
DEPS_DBG := $(SOURCES:%.c=src/%_dbg.d)

# Cppcheck settings. Note that 'style' includes 'warning', 'performance' and 'portability'.
CPPCHECK_DIR   = cppcheck_report
CPPCHECK_XML   = $(CPPCHECK_DIR)/report.xml
CPPCHECK_FLAGS = --cppcheck-build-dir=$(CPPCHECK_DIR) --enable=style,information,missingInclude

# Targets that do not produce an eponymous file
.PHONY: release debug analyse check analyse-cppcheck analyse-cppcheck-html \
        install clean help

release: copris
debug:   copris_dbg
analyse: DBGFLAGS += -fanalyzer
analyse: debug

# Automatic variables of GNU Make:
# $@  The file name of the target of the rule.
# $<  The name of the first prerequisite.
# $^  The names of all the prerequisites, with spaces between them.

# Compile the release binary
copris: $(OBJS_REL)
	$(CC) $(CFLAGS) $(RELFLAGS) $^ $(LDFLAGS) -o $@
	# Validate predefined values from 'config.h' if compiled with Markdown support
	./copris --can-process-md && ./copris --dump-commands >/dev/null || true

%_rel.o: %.c
	$(CC) $(CFLAGS) $(RELFLAGS) -MMD -MP -c $< -o $@

# Compile the debug binary
copris_dbg: $(OBJS_DBG)
	$(CC) $(CFLAGS) $(DBGFLAGS) $^ $(LDFLAGS) -o $@

%_dbg.o: %.c
	$(CC) $(CFLAGS) $(DBGFLAGS) -MMD -MP -c $< -o $@

check:
	$(MAKE) -C tests/ all

# Run Cppcheck code analysis (first recipe prints to stdout, second generates a HTML report)
analyse-cppcheck:
	mkdir -p $(CPPCHECK_DIR)
	cppcheck $(CPPCHECK_FLAGS) src/

analyse-cppcheck-html: $(CPPCHECK_DIR)/index.html
$(CPPCHECK_DIR)/index.html: $(SOURCES)
	mkdir -p $(CPPCHECK_DIR)
	cppcheck $(CPPCHECK_FLAGS) --xml src/ 2>$(CPPCHECK_XML)
	cppcheck-htmlreport --file=$(CPPCHECK_XML) --report-dir=$(CPPCHECK_DIR)

# Automatic dependency tracking (-MMD -MP). If a header file is
# changed, only the files including it will be recompiled.
-include $(DEPS_REL) $(DEPS_DBG)

install: copris man/copris.1
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(DATADIR)/copris $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m755 copris $(DESTDIR)$(BINDIR)
	$(INSTALL) -m644 man/copris.1 $(DESTDIR)$(MANDIR)/man1

clean:
	rm -f $(OBJS_REL) $(DEPS_REL) copris
	rm -f $(OBJS_DBG) $(DEPS_DBG) copris_dbg
	rm -fr $(CPPCHECK_DIR)

help:
	head -n 17 $(firstword $(MAKEFILE_LIST)); \
	grep -m 3 -C 1 -E '(CFLAGS|RELFLAGS|DBGFLAGS)' $(firstword $(MAKEFILE_LIST))
	# Default installation prefix (overridable with PREFIX=<path>): $(PREFIX)
