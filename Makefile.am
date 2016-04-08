ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(srcdir)/src -I$(srcdir)/src/include

sbin_PROGRAMS   = src/tss2-tabd
check_PROGRAMS  = test/connection-manager_unit
lib_LTLIBRARIES = src/libtss2-tcti-tabd.la
TESTS = $(check_PROGRAMS)

src_libtss2_tcti_tabd_la_CFLAGS  = $(DEBUS_CFLAGS) $(GIO_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS)
src_libtss2_tcti_tabd_la_LIBADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS)
src_libtss2_tcti_tabd_la_LDFLAGS = -fPIC -Wl,--no-undefined -Wl,--version-script=$(srcdir)/src/tss2-tcti-tabd.map
src_libtss2_tcti_tabd_la_SOURCES = src/tss2-tcti-tabd.c

src_tss2_tabd_CFLAGS  = $(DBUS_CFLAGS) $(GIO_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS)
src_tss2_tabd_LDADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS)
src_tss2_tabd_SOURCES = src/tss2-tabd.c src/connection-manager.c

test_connection_manager_unit_CFLAGS  = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS)
test_connection_manager_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS)
test_connection_manager_unit_SOURCES = src/connection-manager.c test/connection-manager_unit.c
