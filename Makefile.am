ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(srcdir)/src -I$(srcdir)/src/include

sbin_PROGRAMS   = src/tabd
noinst_PROGRAMS = test/tcti-tabd_read-write
check_PROGRAMS  = \
    test/data-message_unit \
    test/message-queue_unit \
    test/msg_unit \
    test/response-watcher_unit \
    test/session-data_unit \
    test/session-manager_unit \
    test/session-watcher_unit \
    test/tab_unit \
    test/tcti-options_unit \
    test/util_unit \
    test/util-mock-realloc_unit
if TCTI_DEVICE
check_PROGRAMS += test/tcti-device_unit
endif
if TCTI_SOCKET
check_PROGRAMS += test/tcti-socket_unit
endif

lib_LTLIBRARIES = src/libtcti-tabd.la
noinst_LTLIBRARIES = src/libtcti-echo.la
noinst_LIBRARIES = src/libutil.a
man8_MANS = man/tabd.8
TESTS = $(check_PROGRAMS)

tabddir      = $(includedir)/tss2
tabd_HEADERS = $(srcdir)/src/include/tabd.h
libtcti_tabddir      = $(includedir)/tss2
libtcti_tabd_HEADERS = $(srcdir)/src/include/tcti-tabd.h
CLEANFILES = \
    src/tabd-generated.c \
    src/tabd-generated.h \
    dist/*.service
@VALGRIND_CHECK_RULES@
VALGRIND_G_DEBUG = fatal-criticals,gc-friendly
VALGRIND_memcheck_FLAGS = --show-leak-kinds=definite,indirect

src_libutil_a_CFLAGS  = $(AM_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) -fPIC
src_libutil_a_SOURCES = src/util.c

src_libtcti_tabd_la_CFLAGS  = -fPIC $(DEBUS_CFLAGS) $(GIO_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS)
src_libtcti_tabd_la_LIBADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS)
src_libtcti_tabd_la_LDFLAGS = -Wl,--no-undefined -Wl,--version-script=$(srcdir)/src/tcti-tabd.map
src_libtcti_tabd_la_SOURCES = \
    src/tabd-generated.c \
    src/tcti-tabd.c \
    src/util.c

src_libtcti_echo_la_CFLAGS  = -fPIC $(GLIB_CFLAGS) $(AM_CFLAGS)
src_libtcti_echo_la_SOURCES = src/tcti-interface.c src/tcti-echo.c

src_tabd_CFLAGS  = $(DBUS_CFLAGS) $(GIO_CFLAGS) $(GLIB_CFLAGS) \
    $(PTHREAD_CFLAGS) $(AM_CFLAGS) $(SAPI_CFLAGS)
src_tabd_LDADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) \
    $(SAPI_LIBS) $(noinst_LTLIBRARIES) $(noinst_LIBRARIES)
src_tabd_SOURCES = \
    src/control-message.c \
    src/data-message.c \
    src/message-queue.c \
    src/tabd.c \
    src/tabd-generated.c \
    src/logging.c \
    src/session-watcher.c \
    src/session-manager.c \
    src/session-data.c \
    src/response-watcher.c \
    src/tab.c \
    src/tcti-interface.c \
    src/tcti-options.c \
    src/tcti-echo.c \
    src/tcti-type-enum.c
if TCTI_DEVICE
src_tabd_CFLAGS += $(TCTI_DEVICE_CFLAGS)
src_tabd_LDADD += $(TCTI_DEVICE_LIBS)
src_tabd_SOURCES += src/tcti-device.c
endif
if TCTI_SOCKET
src_tabd_CFLAGS += $(TCTI_SOCKET_CFLAGS)
src_tabd_LDADD += $(TCTI_SOCKET_LIBS)
src_tabd_SOURCES += src/tcti-socket.c
endif

man/%.8 : man/%.8.in
	rm -f $@
if TCTI_DEVICE
	echo ".nr HAVE_DEVICE_TCTI 1" >> $@
else
	echo ".nr HAVE_DEVICE_TCTI 0" >> $@
endif
if TCTI_SOCKET
	echo ".nr HAVE_SOCKET_TCTI 1" >> $@
else
	echo ".nr HAVE_SOCKET_TCTI 0" >> $@
endif
	cat $^ >> $@

%-generated.c %-generated.h : src/tabd.xml
	$(GDBUS_CODEGEN) --interface-prefix=us.twobit.tss2. \
	    --generate-c-code=src/tabd-generated $^

dist/%.service : dist/%.service.in
	sed -e 's,[@]SBINDIR[@],$(sbindir),g' $^ > $@

test_response_watcher_unit_CFLAGS  = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(SAPI_CFLAGS) $(AM_CFLAGS) $(PTHREAD_CFLAGS) $(GOBJECT_CFLAGS)
test_response_watcher_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) \
    $(PTHREAD_LIBS) $(noinst_LIBRARIES) $(GOBJECT_LIBS)
test_response_watcher_unit_LDFLAGS = -Wl,--wrap=response_watcher_start
test_response_watcher_unit_SOURCES = \
    src/control-message.c \
    src/data-message.c \
    src/message-queue.c \
    src/response-watcher.c \
    src/session-data.c \
    src/session-manager.c \
    src/tab.c \
    src/tcti-echo.c \
    src/tcti-interface.c \
    test/response-watcher_unit.c

test_session_data_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(AM_CFLAGS)
test_session_data_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS)
test_session_data_unit_SOURCES = src/session-data.c test/session-data_unit.c

test_session_manager_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(AM_CFLAGS)
test_session_manager_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS)
test_session_manager_unit_SOURCES = src/session-manager.c src/session-data.c test/session-manager_unit.c

test_session_watcher_unit_CFLAGS  = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(SAPI_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS) $(GOBJECT_CFLAGS)
test_session_watcher_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) $(GOBJECT_LIBS) $(noinst_LIBRARIES)
test_session_watcher_unit_SOURCES = \
    src/control-message.c \
    src/data-message.c \
    src/message-queue.c \
    src/session-watcher.c \
    src/session-manager.c \
    src/session-data.c \
    src/tab.c \
    src/tcti-echo.c \
    src/tcti-interface.c \
    test/session-watcher_unit.c

test_tab_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(SAPI_CFLAGS) \
    $(AM_CFLAGS) $(PTHREAD_CFLAGS) $(GOBJECT_CFLAGS)
test_tab_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) \
    $(PTHREAD_LIBS) $(GOBJECT_LIBS) $(noinst_LIBRARIES)
test_tab_unit_SOURCES = \
    src/control-message.c \
    src/data-message.c \
    src/message-queue.c \
    src/tab.c \
    src/tcti-echo.c \
    src/tcti-interface.c \
    test/tab_unit.c

# message tests
test_msg_unit_CFLAGS  = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(AM_CFLAGS)
test_msg_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS)
test_msg_unit_SOURCES = \
    src/msg.c \
    src/session-data.c \
    test/msg_unit.c

if TCTI_DEVICE
test_tcti_device_unit_CFLAGS   = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(GOJBECT_CFLAGS) $(TCTI_DEVICE_CFLAGS) $(AM_CFLAGS)
test_tcti_device_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(TCTI_DEVICE_LIBS)
test_tcti_device_unit_SOURCES  = \
    src/tcti-device.c \
    src/tcti-interface.c \
    test/tcti-device_unit.c
endif

if TCTI_SOCKET
test_tcti_socket_unit_CFLAGS   = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(GOJBECT_CFLAGS) $(TCTI_SOCKET_CFLAGS) $(AM_CFLAGS)
test_tcti_socket_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(TCTI_SOCKET_LIBS)
test_tcti_socket_unit_SOURCES  = \
    src/tcti-socket.c \
    src/tcti-interface.c \
    test/tcti-socket_unit.c
endif

test_tcti_options_unit_CFLAGS  = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(AM_CFLAGS)
test_tcti_options_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_tcti_options_unit_SOURCES = \
    src/tcti-echo.c \
    src/tcti-interface.c \
    src/tcti-options.c \
    src/tcti-type-enum.c \
    test/tcti-options_unit.c
if TCTI_DEVICE
test_tcti_options_unit_CFLAGS  += $(TCTI_DEVICE_CFLAGS)
test_tcti_options_unit_LDADD   += $(TCTI_DEVICE_LIBS)
test_tcti_options_unit_SOURCES += src/tcti-device.c
endif
if TCTI_SOCKET
test_tcti_options_unit_CFLAGS  += $(TCTI_SOCKET_CFLAGS)
test_tcti_options_unit_LDADD   += $(TCTI_SOCKET_LIBS)
test_tcti_options_unit_SOURCES += src/tcti-socket.c
endif

test_tcti_tabd_read_write_CFLAGS  = $(GLIB_CFLAGS) $(AM_CFLAGS)
test_tcti_tabd_read_write_LDADD   = $(GLIB_LIBS) src/libtcti-tabd.la
test_tcti_tabd_read_write_SOURCES = test/tcti-tabd_read-write.c

test_util_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS)
test_util_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) $(noinst_LIBRARIES)
test_util_unit_LDFLAGS = -Wl,--wrap=read,--wrap=write
test_util_unit_SOURCES = test/util_unit.c

test_util_mock_realloc_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS)
test_util_mock_realloc_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) $(noinst_LIBRARIES)
test_util_mock_realloc_unit_LDFLAGS = -Wl,--wrap=realloc
test_util_mock_realloc_unit_SOURCES = test/util-mock-realloc_unit.c

test_data_message_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(AM_CFLAGS)
test_data_message_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_data_message_unit_SOURCES = \
    src/data-message.c \
    src/session-data.c \
    test/data-message_unit.c

test_message_queue_unit_CFLAGS = $(CMOCKA_CFLAGS) $(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(AM_CFLAGS)
test_message_queue_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_message_queue_unit_SOURCES = \
    src/data-message.c \
    src/message-queue.c \
    test/message-queue_unit.c
