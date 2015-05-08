EXTRA_DIST =
CLEANFILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\" \
	-I${top_srcdir}/src/dt \
	-I${top_srcdir}/src

AM_CFLAGS = ${my_CFLAGS} \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections

AM_LDFLAGS = \
	-Wl,--gc-sections \
	-Wl,--as-needed

SED_PROCESS = \
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(SED) \
	-e 's,@VERSION\@,$(VERSION),g' \
	-e 's,@prefix\@,$(prefix),g' \
	-e 's,@exec_prefix\@,$(exec_prefix),g' \
	-e 's,@libdir\@,$(libdir),g' \
	-e 's,@includedir\@,$(includedir),g' \
	< $< > $@ || rm $@

%.pc: %.pc.in Makefile
	$(SED_PROCESS)

LIBDT_CURRENT=2
LIBDT_REVISION=0
LIBDT_AGE=2

pkginclude_HEADERS = \
	src/dt/dt.h \
	src/dt/fdt.h \
	src/dt/list.h
lib_LTLIBRARIES = src/libdt-utils.la

bin_PROGRAMS = barebox-state fdtdump
barebox_state_SOURCES = src/barebox-state.c
barebox_state_CFLAGS = $(LIBDT_CFLAGS)
barebox_state_LDADD = src/libdt-utils.la

fdtdump_SOURCES = src/fdtdump.c
fdtdump_CFLAGS = $(LIBDT_CFLAGS)
fdtdump_LDADD = src/libdt-utils.la

src_libdt_utils_la_SOURCES = \
	src/crc32.c \
	src/libdt.c \
	src/fdt.c \
	src/dt/common.h

src_libdt_utils_la_CFLAGS = $(UDEV_CFLAGS)
src_libdt_utils_la_LIBADD = $(UDEV_LIBS)

EXTRA_DIST += src/libdt-utils.sym

src_libdt_utils_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBDT_CURRENT):$(LIBDT_REVISION):$(LIBDT_AGE) \
	-Wl,--version-script=$(top_srcdir)/src/libdt-utils.sym
src_libdt_utils_la_DEPENDENCIES = ${top_srcdir}/src/libdt-utils.sym

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = src/libdt-utils.pc
EXTRA_DIST += src/libdt-utils.pc.in
CLEANFILES += src/libdt-utils.pc

EXTRA_DIST += scripts/barebox-mark-successful-boot.sh
