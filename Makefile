# -----------------------------------------------------------------------------
# Makefile -- build rsync with local extensions and gnulib crypto modules
#
# Steps (each one is a stamp file in .stamps/; each one is ordered after the
# previous one, even with "make -j", but is only redone when its own inputs
# change, e.g. editing a file under files/ never downloads anything again):
#
#   1. download the rsync sources             -> rsync.dev/
#   2. create the "xattr_sums" sub-directory  -> rsync.dev/xattr_sums/
#   3. install the extension files there     (XATTR_SUMS_FILES)
#   4. replace some rsync source files       (REPLACE_FILES)
#   5. download the gnulib sources            -> gnulib/
#   6. import crypto/md5, crypto/sha256, crypto/sha512
#                                             -> gnulib.build/ (own configure)
#   7. compile the gnulib modules             -> gnulib.build/gllib/libgnu.a
#   8. configure rsync (never before gnulib is finished and out of the way)
#   9. compile the xattr_sums files
#  10. compile rsync and link it with the xattr_sums objects and libgnu.a
#
# gnulib is configured and built in its own directory (gnulib.build/) with a
# clean environment, and rsync in rsync.dev/ with its own configure script.
# The only things that cross the border are:
#   - three headers (md5.h, sha256.h, sha512.h + helpers) copied into
#     gnulib.build/include/, never the gnulib config.h nor its header
#     wrappers, so that rsync's configuration is not disturbed;
#   - libgnu.a, passed to the rsync link command only.
#
# Files to provide (relative to FILES_DIR, default ./files):
#   files/xattr_sums/<name>   for each name in XATTR_SUMS_FILES
#   files/replace/<path>      for each path in REPLACE_FILES; <path> is relative
#                             to the rsync source tree and must already exist
#                             there (e.g. usage.c or lib/compat.c)
#
# Usage:
#   make            build everything, result: rsync.dev/rsync
#   make test       run a quick functional test with dummy files
#   make relink     rebuild changed objects and relink rsync
#   make clean      remove build products (keeps downloaded sources)
#   make distclean  remove everything that was generated or downloaded
#
# Host tools required: gcc, make (GNU, >= 4.0), curl, tar, git, python3, perl,
#   gawk, autoconf, automake (gnulib-tool and rsync's prepare-source need them).
#
# Tip: use RSYNC_CFLAGS / GL_CFLAGS below rather than CFLAGS on the make
# command line, as the latter would be exported to both sub-builds.
# -----------------------------------------------------------------------------

# ---- User settings ----------------------------------------------------------

CC				  = gcc

RSYNC_VERSION        ?= 3.4.1
RSYNC_URL            ?= https://github.com/RsyncProject/rsync/archive/refs/tags/v$(RSYNC_VERSION).tar.gz
RSYNC_CONFIGURE_ARGS ?= --disable-md2man
RSYNC_CFLAGS         ?= -DXATTR_SUMS -DAI_RSYNC
RSYNC_LDFLAGS         = -lcap

GNULIB_URL           ?= https://github.com/coreutils/gnulib.git
GNULIB_REF           ?=
GNULIB_MODULES       ?= crypto/md5 crypto/sha256 crypto/sha512
GL_CFLAGS            ?=
# Gnulib headers exposed to the extension sources:
GL_HEADERS           ?= md5.h sha256.h sha512.h u64.h gl_openssl.h

FILES_DIR            ?= $(CURDIR)/files
# Extension files (installed in rsync.dev/xattr_sums/); *.c files are compiled.
XATTR_SUMS_FILES     ?= ai_cpri.h ai_epri.h ai_gpri.c ai_xattr.c ai_wrapper.h
# rsync files to be replaced (paths relative to rsync.dev/).
REPLACE_FILES        ?= usage.c rsync.h checksum.c fileio.c receiver.c

# ---- Internal layout --------------------------------------------------------

TOP          := $(CURDIR)
STAMPS       := $(TOP)/.stamps
RSYNC_DIR    := $(TOP)/rsync.dev
XATTR_DIR    := $(RSYNC_DIR)/xattr_sums
GNULIB_DIR   := $(TOP)/gnulib
GL_BUILD     := $(TOP)/gnulib.build
GL_INC       := $(GL_BUILD)/include
GL_LIB       := $(GL_BUILD)/gllib/libgnu.a
GL_SHIM      := $(GL_INC)/gnulib-shim.h
RSYNC_TARBALL:= $(TOP)/rsync-$(RSYNC_VERSION).tar.gz
RSYNC_BIN    := $(RSYNC_DIR)/rsync

XATTR_SRCS   := $(filter %.c,$(XATTR_SUMS_FILES))
XATTR_OBJS   := $(patsubst %.c,$(XATTR_DIR)/%.o,$(XATTR_SRCS))

# Run sub-builds without any compiler-related variable inherited from the user.
CLEAN_ENV    := env -u CFLAGS -u CPPFLAGS -u LDFLAGS -u LIBS

REQUIRED_TOOLS := gcc make curl tar git python3 perl gawk autoconf autoheader \
                  aclocal automake

.DELETE_ON_ERROR:
.PHONY: all test relink clean distclean help
.DEFAULT_GOAL := all

all: $(RSYNC_BIN)

help:
	@sed -n '2,/^# ------.*$$/p' $(firstword $(MAKEFILE_LIST)) | sed 's/^# \{0,1\}//'

# ---- Step 0: sanity checks --------------------------------------------------

$(STAMPS)/tools:
	@mkdir -p $(STAMPS)
	@missing=; for t in $(REQUIRED_TOOLS); do \
	    command -v $$t >/dev/null 2>&1 || missing="$$missing $$t"; \
	done; \
	if test -n "$$missing"; then \
	    echo "error: missing host tools:$$missing" >&2; exit 1; \
	fi
	@touch $@

# ---- Step 1: download rsync -------------------------------------------------

$(RSYNC_TARBALL): | $(STAMPS)/tools
	curl -fL --retry 3 -o $@.part $(RSYNC_URL)
	mv $@.part $@

$(STAMPS)/1-rsync-fetched: $(RSYNC_TARBALL)
	rm -rf $(RSYNC_DIR)
	mkdir -p $(RSYNC_DIR)
	tar -xzf $< --strip-components=1 -C $(RSYNC_DIR)
	@touch $@

# ---- Step 2: create rsync.dev/xattr_sums ------------------------------------

$(STAMPS)/2-xattr-dir: $(STAMPS)/1-rsync-fetched
	mkdir -p $(XATTR_DIR)
	@touch $@

# ---- Step 3: install the extension files ------------------------------------

$(STAMPS)/3-xattr-files: $(addprefix $(FILES_DIR)/xattr_sums/,$(XATTR_SUMS_FILES)) \
                         | $(STAMPS)/2-xattr-dir
	@for f in $(XATTR_SUMS_FILES); do \
	    echo "install $(FILES_DIR)/xattr_sums/$$f -> $(XATTR_DIR)/"; \
	    install -m 644 $(FILES_DIR)/xattr_sums/$$f $(XATTR_DIR)/$$f || exit 1; \
	done
	@touch $@

# ---- Step 4: replace some rsync sources (originals saved as *.orig) ---------

$(STAMPS)/4-replaced: $(addprefix $(FILES_DIR)/replace/,$(REPLACE_FILES)) \
                      | $(STAMPS)/3-xattr-files
	@for f in $(REPLACE_FILES); do \
	    if test ! -f $(RSYNC_DIR)/$$f; then \
	        echo "error: $$f does not exist in the rsync tree" >&2; exit 1; \
	    fi; \
	    test -f $(RSYNC_DIR)/$$f.orig || cp -p $(RSYNC_DIR)/$$f $(RSYNC_DIR)/$$f.orig; \
	    echo "replace $(RSYNC_DIR)/$$f"; \
	    cp $(FILES_DIR)/replace/$$f $(RSYNC_DIR)/$$f || exit 1; \
	done
	@touch $@

# ---- Step 5: download gnulib ------------------------------------------------

$(STAMPS)/5-gnulib-fetched: | $(STAMPS)/4-replaced
	rm -rf $(GNULIB_DIR)
	git clone --depth 1 $(if $(GNULIB_REF),--branch $(GNULIB_REF)) \
	    $(GNULIB_URL) $(GNULIB_DIR)
	@touch $@

# ---- Step 6: import the crypto modules --------------------------------------
# gnulib-tool creates a self-contained directory (own configure.ac, own
# config.h, gllib/ with the module sources and libgnu.a's Makefile).

$(STAMPS)/6-gnulib-imported: $(STAMPS)/5-gnulib-fetched
	rm -rf $(GL_BUILD)
	cd $(GNULIB_DIR) && $(CLEAN_ENV) \
	    ./gnulib-tool --create-testdir --dir=$(GL_BUILD) $(GNULIB_MODULES)
	@touch $@

# ---- Step 7: compile the gnulib modules -------------------------------------

$(STAMPS)/7-gnulib-built: $(STAMPS)/6-gnulib-imported
	cd $(GL_BUILD) && $(CLEAN_ENV) ./configure --quiet \
	    $(if $(GL_CFLAGS),CFLAGS='$(GL_CFLAGS)')
	$(CLEAN_ENV) $(MAKE) -C $(GL_BUILD)/gllib
	@test -f $(GL_LIB) || { echo "error: $(GL_LIB) not built" >&2; exit 1; }
	@touch $@

# Public headers for the extension sources.  gnulib headers insist on having
# gnulib's own config.h included first; instead of exposing that file (it would
# clash with rsync's config.h) a tiny shim provides the few macros they need.
define GL_SHIM_TEXT
/* Generated by the Makefile: minimal stand-in for gnulib's config.h. */
#ifndef GNULIB_SHIM_H
#define GNULIB_SHIM_H
#ifndef _GL_CONFIG_H_INCLUDED
# define _GL_CONFIG_H_INCLUDED 1
#endif
#ifndef _GL_INLINE_HEADER_BEGIN
# define _GL_INLINE_HEADER_BEGIN
# define _GL_INLINE_HEADER_END
#endif
#ifndef _GL_INLINE
# define _GL_INLINE static inline
#endif
#endif
endef
export GL_SHIM_TEXT

$(GL_SHIM): $(STAMPS)/7-gnulib-built
	mkdir -p $(GL_INC)
	@for h in $(GL_HEADERS); do \
	    cp $(GL_BUILD)/gllib/$$h $(GL_INC)/ || exit 1; \
	done
	printf '%s\n' "$$GL_SHIM_TEXT" > $@

$(GL_LIB): $(STAMPS)/7-gnulib-built

# ---- Step 8: configure rsync ------------------------------------------------
# Done only now, when gnulib is completely built: the two configurations never
# run concurrently and never share an environment.

$(STAMPS)/8-rsync-configured: $(STAMPS)/1-rsync-fetched | $(GL_SHIM)
	cd $(RSYNC_DIR) && $(CLEAN_ENV) ./prepare-source
	cd $(RSYNC_DIR) && $(CLEAN_ENV) ./configure $(RSYNC_CONFIGURE_ARGS) \
	    $(if $(RSYNC_CFLAGS),CFLAGS='$(RSYNC_CFLAGS)')
	@touch $@

# ---- Step 9: compile the xattr_sums files -----------------------------------
# These see rsync's headers (config.h, rsync.h...) and the gnulib headers.

XATTR_CPPFLAGS = -I$(RSYNC_DIR) -I$(RSYNC_DIR)/popt -I$(RSYNC_DIR)/zlib \
                 -I$(XATTR_DIR) -I$(GL_INC) -include $(GL_SHIM) -DHAVE_CONFIG_H
XATTR_CFLAGS  ?= -g -O2 -Wall -W -DXATTR_SUMS -DAI_RSYNC

# (the stamps stand for the sources: they are renewed whenever a file under
# FILES_DIR changes, and they guarantee the ordering with the previous steps)
$(XATTR_OBJS): $(XATTR_DIR)/%.o: $(STAMPS)/3-xattr-files \
               $(STAMPS)/8-rsync-configured $(GL_SHIM)
	cc=`sed -n 's/^CC=//p' $(RSYNC_DIR)/Makefile`; \
	    $${cc:-$(CC)} $(XATTR_CPPFLAGS) $(XATTR_CFLAGS) -c $(XATTR_DIR)/$*.c -o $@

$(STAMPS)/9-xattr-built: $(XATTR_OBJS)
	@touch $@

# ---- Step 10: build rsync and link with xattr_sums + libgnu.a ---------------
# rsync's own Makefile drives the build; only LIBS is overridden on the command
# line for the link, keeping the value found by rsync's configure.  The old
# executable is removed first so that a change in the extra objects always
# triggers a relink.

$(RSYNC_BIN): $(STAMPS)/9-xattr-built $(GL_LIB) $(STAMPS)/8-rsync-configured \
              $(STAMPS)/4-replaced
	rm -f $@
	cd $(RSYNC_DIR) && orig_libs=`sed -n 's/^LIBS=//p' Makefile`; \
	    $(CLEAN_ENV) $(MAKE) LIBS="$(XATTR_OBJS) $(GL_LIB) $(RSYNC_LDFLAGS) $$orig_libs" rsync
	@echo "Built: $@"

relink:
	@rm -f $(RSYNC_BIN) $(STAMPS)/9-xattr-built
	$(MAKE) $(RSYNC_BIN)

# ---- Test with dummy files --------------------------------------------------

test: $(RSYNC_BIN)
	@set -e; t=`mktemp -d`; trap 'rm -rf $$t' EXIT; \
	echo "== version banner"; \
	$(RSYNC_BIN) --version | grep 'Extensions: xattr_sums'; \
	$(RSYNC_BIN) --version | grep -q 'self-test: md5+sha256+sha512 OK'; \
	echo "== dummy tree"; \
	mkdir -p $$t/src/a/b $$t/src/c; \
	echo "hello" > $$t/src/a/one.txt; \
	head -c 300000 /dev/urandom > $$t/src/a/b/random.bin; \
	: > $$t/src/c/empty; ln -s a/one.txt $$t/src/link; \
	echo "== rsync -a"; \
	$(RSYNC_BIN) -a $$t/src/ $$t/dst/; \
	diff -r $$t/src $$t/dst; \
	echo "== rsync --checksum (must transfer nothing)"; \
	out=`$(RSYNC_BIN) -a --checksum -i $$t/src/ $$t/dst/`; test -z "$$out"; \
	echo "== update after change"; \
	echo "changed" >> $$t/src/a/one.txt; \
	$(RSYNC_BIN) -a --checksum $$t/src/ $$t/dst/; diff -r $$t/src $$t/dst; \
	echo "ALL TESTS PASSED"

# ---- Cleaning ---------------------------------------------------------------

clean:
	-$(MAKE) -C $(RSYNC_DIR) clean >/dev/null 2>&1
	rm -f $(XATTR_OBJS)
	-$(MAKE) -C $(GL_BUILD)/gllib clean >/dev/null 2>&1
	rm -f $(STAMPS)/7-* $(STAMPS)/8-* $(STAMPS)/9-*

distclean:
	rm -rf $(RSYNC_DIR) $(GNULIB_DIR) $(GL_BUILD) $(STAMPS) $(RSYNC_TARBALL)
