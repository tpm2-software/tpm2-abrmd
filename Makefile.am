ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(srcdir)/src -I$(srcdir)/src/include

sbin_PROGRAMS   = src/tss2-tabd
noinst_PROGRAMS = test/tss2-tcti-tabd_read-write
check_PROGRAMS  = \
    test/session-data_unit \
    test/session-manager_unit \
    test/session-watcher_unit \
    test/blob_unit \
    test/blob-queue_unit \
    test/tab_unit
lib_LTLIBRARIES = src/libtss2-tcti-tabd.la
man8_MANS = man/tabd.man
TESTS = $(check_PROGRAMS)

tss2_tabddir      = $(includedir)/tss2
tss2_tabd_HEADERS = $(srcdir)/src/include/tss2-tabd.h
libtss2_tcti_tabddir      = $(includedir)/tss2
libtss2_tcti_tabd_HEADERS = $(srcdir)/src/include/tss2-tcti-tabd.h
CLEANFILES = \
    src/tss2-tabd-generated.c \
    src/tss2-tabd-generated.h \
    units/*.service

src_libtss2_tcti_tabd_la_CFLAGS  = $(DEBUS_CFLAGS) $(GIO_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS)
src_libtss2_tcti_tabd_la_LIBADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS)
src_libtss2_tcti_tabd_la_LDFLAGS = -fPIC -Wl,--no-undefined -Wl,--version-script=$(srcdir)/src/tss2-tcti-tabd.map
src_libtss2_tcti_tabd_la_SOURCES = src/tss2-tcti-tabd.c src/tss2-tabd-generated.c

src_tss2_tabd_CFLAGS  = $(DBUS_CFLAGS) $(GIO_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS)
src_tss2_tabd_LDADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS)
src_tss2_tabd_SOURCES = \
    src/tss2-tabd.c \
    src/tss2-tabd-generated.c \
    src/tss2-tabd-logging.c \
    src/session-watcher.c \
    src/session-manager.c \
    src/session-data.c

%-generated.c %-generated.h : src/tss2-tabd.xml
	$(GDBUS_CODEGEN) --interface-prefix=us.twobit.tss2. \
	    --generate-c-code=src/tss2-tabd-generated $^

units/%.service : units/%.service.in
	sed -e 's,[@]SBINDIR[@],$(sbindir),g' $^ > $@

test_session_data_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(AM_CFLAGS)
test_session_data_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS)
test_session_data_unit_SOURCES = src/session-data.c test/session-data_unit.c

test_session_manager_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(AM_CFLAGS)
test_session_manager_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS)
test_session_manager_unit_SOURCES = src/session-manager.c src/session-data.c test/session-manager_unit.c

test_session_watcher_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS)
test_session_watcher_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS)
test_session_watcher_unit_SOURCES = \
    src/session-watcher.c \
    src/session-manager.c \
    src/session-data.c \
    test/session-watcher_unit.c

test_tab_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(TSS2_CFLAGS) \
    $(AM_CFLAGS) $(PTHREAD_CFLAGS)
test_tab_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(TSS2_LIBS) \
    $(PTHREAD_LIBS)
test_tab_unit_SOURCES = \
    src/blob.c \
    src/blob-queue.c \
    src/tab.c \
    test/tab_unit.c
test_blob_unit_CFLAGS  = $(CMOCKA_CLFAGS) $(GLIB_CFLAGS) $(AM_CFLAGS)
test_blob_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS)
test_blob_unit_SOURCES = \
    src/blob.c \
    src/session-data.c \
    test/blob_unit.c

test_blob_queue_unit_CFLAGS  = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(AM_CFLAGS)
test_blob_queue_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS)
test_blob_queue_unit_SOURCES = \
    src/blob-queue.c \
    src/blob.c \
    test/blob-queue_unit.c

test_tss2_tcti_tabd_read_write_CFLAGS  = $(GLIB_CFLAGS) $(AM_CFLAGS)
test_tss2_tcti_tabd_read_write_LDADD   = $(GLIB_LIBS) src/libtss2-tcti-tabd.la
test_tss2_tcti_tabd_read_write_SOURCES = test/tss2-tcti-tabd_read-write.c
