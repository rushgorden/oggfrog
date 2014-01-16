# Makefile.in generated automatically by automake 1.4-p6 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999, 2001 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/libvorbis
pkglibdir = $(libdir)/libvorbis
pkgincludedir = $(includedir)/libvorbis

top_builddir = ..

ACLOCAL = /OggFrog/libvorbis/missing aclocal-1.4
AUTOCONF = autoconf
AUTOMAKE = /OggFrog/libvorbis/missing automake-1.4
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = 
host_triplet = i686-apple-darwin8.6.1
AR = ar
AS = @AS@
CC = gcc
CPP = gcc -E
CXX = g++
CXXCPP = g++ -E
DEBUG = -DDARWIN -fno-common -force_cpusubtype_ALL -Wall -g -O0 -fsigned-char
DLLTOOL = @DLLTOOL@
ECHO = echo
EGREP = grep -E
EXEEXT = 
F77 = 
GCJ = @GCJ@
GCJFLAGS = @GCJFLAGS@
HAVE_LIB = @HAVE_LIB@
LIB = @LIB@
LIBS = -lm  -logg
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LN_S = ln -s
LTLIB = @LTLIB@
MAINT = #
MAKEINFO = makeinfo
OBJDUMP = @OBJDUMP@
OBJEXT = o
OGG_CFLAGS = 
OGG_LIBS =  -logg
PACKAGE = libvorbis
PDFXMLTEX = 
PROFILE = -DDARWIN -fno-common -force_cpusubtype_ALL -Wall -g -pg -O4 -ffast-math -fsigned-char
RANLIB = ranlib
RC = @RC@
STRIP = strip
VERSION = 1.0.1
VE_LIB_AGE = 0
VE_LIB_CURRENT = 2
VE_LIB_REVISION = 0
VF_LIB_AGE = 1
VF_LIB_CURRENT = 4
VF_LIB_REVISION = 0
V_LIB_AGE = 3
V_LIB_CURRENT = 3
V_LIB_REVISION = 0
XSLTPROC = 
pthread_lib = -lpthread

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I../lib -I$(top_srcdir)/include 

EXTRA_PROGRAMS = latticebuild latticepare latticehint 	latticetune huffbuild distribution

CLEANFILES = $(EXTRA_PROGRAMS)

LDFLAGS = -static
LDADD = ../lib/libvorbis.la

latticebuild_SOURCES = latticebuild.c vqgen.c bookutil.c 	vqgen.h bookutil.h

latticepare_SOURCES = latticepare.c vqgen.c bookutil.c vqsplit.c 	vqgen.h bookutil.h

latticehint_SOURCES = latticehint.c bookutil.c 	bookutil.h

latticetune_SOURCES = latticetune.c vqgen.c bookutil.c 	vqgen.h bookutil.h

huffbuild_SOURCES = huffbuild.c vqgen.c bookutil.c 	vqgen.h bookutil.h

distribution_SOURCES = distribution.c bookutil.c 	bookutil.h


EXTRA_DIST = residue_entropy auxpartition.pl
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_CLEAN_FILES = 

DEFS = -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE=\"libvorbis\" -DVERSION=\"1.0.1\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DHAVE_ALLOCA_H=1 -DHAVE_ALLOCA=1  -I. -I$(srcdir) 
CPPFLAGS = 
latticebuild_OBJECTS =  latticebuild.$(OBJEXT) vqgen.$(OBJEXT) \
bookutil.$(OBJEXT)
latticebuild_LDADD = $(LDADD)
latticebuild_DEPENDENCIES =  ../lib/libvorbis.la
latticebuild_LDFLAGS = 
latticepare_OBJECTS =  latticepare.$(OBJEXT) vqgen.$(OBJEXT) \
bookutil.$(OBJEXT) vqsplit.$(OBJEXT)
latticepare_LDADD = $(LDADD)
latticepare_DEPENDENCIES =  ../lib/libvorbis.la
latticepare_LDFLAGS = 
latticehint_OBJECTS =  latticehint.$(OBJEXT) bookutil.$(OBJEXT)
latticehint_LDADD = $(LDADD)
latticehint_DEPENDENCIES =  ../lib/libvorbis.la
latticehint_LDFLAGS = 
latticetune_OBJECTS =  latticetune.$(OBJEXT) vqgen.$(OBJEXT) \
bookutil.$(OBJEXT)
latticetune_LDADD = $(LDADD)
latticetune_DEPENDENCIES =  ../lib/libvorbis.la
latticetune_LDFLAGS = 
huffbuild_OBJECTS =  huffbuild.$(OBJEXT) vqgen.$(OBJEXT) \
bookutil.$(OBJEXT)
huffbuild_LDADD = $(LDADD)
huffbuild_DEPENDENCIES =  ../lib/libvorbis.la
huffbuild_LDFLAGS = 
distribution_OBJECTS =  distribution.$(OBJEXT) bookutil.$(OBJEXT)
distribution_LDADD = $(LDADD)
distribution_DEPENDENCIES =  ../lib/libvorbis.la
distribution_LDFLAGS = 
CFLAGS = -DDARWIN -fno-common -force_cpusubtype_ALL -Wall -g -O4 -ffast-math -fsigned-char  -DUSE_MEMORY_H
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP_ENV = --best
SOURCES = $(latticebuild_SOURCES) $(latticepare_SOURCES) $(latticehint_SOURCES) $(latticetune_SOURCES) $(huffbuild_SOURCES) $(distribution_SOURCES)
OBJECTS = $(latticebuild_OBJECTS) $(latticepare_OBJECTS) $(latticehint_OBJECTS) $(latticetune_OBJECTS) $(huffbuild_OBJECTS) $(distribution_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .lo .o .obj .s
$(srcdir)/Makefile.in: # Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --foreign --include-deps vq/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


.c.o:
	$(COMPILE) -c $<

# FIXME: We should only use cygpath when building on Windows,
# and only if it is available.
.c.obj:
	$(COMPILE) -c `cygpath -w $<`

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core
	-rm -f *.$(OBJEXT)

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

.c.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

.s.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

.S.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs _libs

distclean-libtool:

maintainer-clean-libtool:

latticebuild$(EXEEXT): $(latticebuild_OBJECTS) $(latticebuild_DEPENDENCIES)
	@rm -f latticebuild$(EXEEXT)
	$(LINK) $(latticebuild_LDFLAGS) $(latticebuild_OBJECTS) $(latticebuild_LDADD) $(LIBS)

latticepare$(EXEEXT): $(latticepare_OBJECTS) $(latticepare_DEPENDENCIES)
	@rm -f latticepare$(EXEEXT)
	$(LINK) $(latticepare_LDFLAGS) $(latticepare_OBJECTS) $(latticepare_LDADD) $(LIBS)

latticehint$(EXEEXT): $(latticehint_OBJECTS) $(latticehint_DEPENDENCIES)
	@rm -f latticehint$(EXEEXT)
	$(LINK) $(latticehint_LDFLAGS) $(latticehint_OBJECTS) $(latticehint_LDADD) $(LIBS)

latticetune$(EXEEXT): $(latticetune_OBJECTS) $(latticetune_DEPENDENCIES)
	@rm -f latticetune$(EXEEXT)
	$(LINK) $(latticetune_LDFLAGS) $(latticetune_OBJECTS) $(latticetune_LDADD) $(LIBS)

huffbuild$(EXEEXT): $(huffbuild_OBJECTS) $(huffbuild_DEPENDENCIES)
	@rm -f huffbuild$(EXEEXT)
	$(LINK) $(huffbuild_LDFLAGS) $(huffbuild_OBJECTS) $(huffbuild_LDADD) $(LIBS)

distribution$(EXEEXT): $(distribution_OBJECTS) $(distribution_DEPENDENCIES)
	@rm -f distribution$(EXEEXT)
	$(LINK) $(distribution_LDFLAGS) $(distribution_OBJECTS) $(distribution_LDADD) $(LIBS)

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags -o $$here/TAGS $(ETAGS_ARGS) $$tags  $$unique $(LISP))

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = vq

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$d/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done

info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am:
install-exec: install-exec-am

install-data-am:
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am:
uninstall: uninstall-am
all-am: Makefile
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:


mostlyclean-generic:

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-compile mostlyclean-libtool \
		mostlyclean-tags mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-compile clean-libtool clean-tags clean-generic \
		mostlyclean-am

clean: clean-am

distclean-am:  distclean-compile distclean-libtool distclean-tags \
		distclean-generic clean-am
	-rm -f libtool

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-compile maintainer-clean-libtool \
		maintainer-clean-tags maintainer-clean-generic \
		distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile mostlyclean-libtool distclean-libtool \
clean-libtool maintainer-clean-libtool tags mostlyclean-tags \
distclean-tags clean-tags maintainer-clean-tags distdir info-am info \
dvi-am dvi check check-am installcheck-am installcheck install-exec-am \
install-exec install-data-am install-data install-am install \
uninstall-am uninstall all-redirect all-am all installdirs \
mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


debugvq:
	$(MAKE) vq CFLAGS="-DDARWIN -fno-common -force_cpusubtype_ALL -Wall -g -O0 -fsigned-char"

profilevq:
	$(MAKE) vq CFLAGS="-DDARWIN -fno-common -force_cpusubtype_ALL -Wall -g -pg -O4 -ffast-math -fsigned-char"

vq: 
	$(MAKE) $(EXTRA_PROGRAMS) 

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
