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

# If the library source code has changed at all since the last release,
#   then increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
# If any interfaces have been added/removed/changed since the last release,
#   then increment current, and set revision to 0.
# If any public interfaces have been added since the last public release,
#   then increment age.
# If any interfaces have been removed or changed since the last release,
#   then set age to 0.
LIBDT_CURRENT=2
LIBDT_REVISION=1
LIBDT_AGE=2

pkginclude_HEADERS = \
	src/dt/dt.h \
	src/dt/fdt.h \
	src/dt/list.h \
	src/dt/common.h
lib_LTLIBRARIES = src/libdt-utils.la

bin_PROGRAMS = barebox-state dtblint fdtdump

barebox_state_SOURCES = \
	src/crypto/digest.c \
	src/crypto/hmac.c \
	src/crypto/sha1.c \
	src/crypto/sha2.c \
	src/keystore-blob.c \
	src/base64.c \
	src/barebox-state/backend_bucket_circular.c \
	src/barebox-state/backend_bucket_direct.c \
	src/barebox-state/backend_format_dtb.c \
	src/barebox-state/backend_format_raw.c \
	src/barebox-state/backend_storage.c \
	src/barebox-state/state.c \
	src/barebox-state/state.h \
	src/barebox-state/state_variables.c \
	src/barebox-state.c \
	src/barebox-state.h \
	\
	src/asm/unaligned.h \
	src/base64.h \
	src/crypto/internal.h \
	src/crypto/sha.h \
	src/digest.h \
	src/fs.h \
	src/init.h \
	src/crypto/keystore.h \
	src/linux/err.h \
	src/linux/list.h \
	src/module.h \
	src/libfile.h \
	src/linux/mtd/mtd-abi.h \
	src/printk.h \
	src/net.h \
	src/state.h \
	src/libbb.h \
	src/fdt.h \
	src/of.h \
	src/crc.h \
	src/mtd/mtd-peb.h \
	src/driver.h
barebox_state_CFLAGS = $(LIBDT_CFLAGS)
barebox_state_LDADD = src/libdt-utils.la

dtblint_SOURCES = src/dtblint.c src/dtblint-imx-pinmux.c src/dtblint.h
dtblint_LDADD = src/libdt-utils.la

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
