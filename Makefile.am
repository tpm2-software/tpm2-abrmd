ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(srcdir)/src -I$(srcdir)/src/include \
    $(DBUS_CFLAGS) $(GIO_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) \
    $(SAPI_CFLAGS) $(TCTI_DEVICE_CFLAGS) $(TCTI_SOCKET_CFLAGS)
UNIT_AM_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS)

sbin_PROGRAMS   = src/tpm2-abrmd
noinst_PROGRAMS = \
    test/integration/create-keys \
    test/tcti-tabrmd_read-write
check_PROGRAMS  = \
    test/access-broker_unit \
    test/command-attrs_unit \
    test/message-queue_unit \
    test/resource-manager_unit \
    test/response-sink_unit \
    test/session-data_unit \
    test/session-manager_unit \
    test/command-source_unit \
    test/handle-map-entry_unit \
    test/handle-map_unit \
    test/random_unit \
    test/test-skeleton_unit \
    test/tcti-echo_unit \
    test/tcti-options_unit \
    test/tpm2-command_unit \
    test/tpm2-response_unit \
    test/tss2-tcti-echo_unit \
    test/util_unit \
    test/util-mock-realloc_unit
if TCTI_DEVICE
check_PROGRAMS += test/tcti-device_unit
endif
if TCTI_SOCKET
check_PROGRAMS += test/tcti-socket_unit
endif

lib_LTLIBRARIES = src/libtcti-tabrmd.la
noinst_LTLIBRARIES = src/libtcti-echo.la src/libutil.la
man8_MANS = man/man8/tpm2-abrmd.8
TESTS = $(check_PROGRAMS)

tpm2_abrmddir      = $(includedir)/tss2
tpm2_abrmd_HEADERS = $(srcdir)/src/include/tabrmd.h
libtcti_tabrmddir      = $(includedir)/tcti
libtcti_tabrmd_HEADERS = $(srcdir)/src/include/tcti-tabrmd.h
CLEANFILES = \
    src/tabrmd-generated.c \
    src/tabrmd-generated.h \
    dist/*.service
BUILT_SOURCES = src/tabrmd-generated.h src/tabrmd-generated.c

pkgconfigdir          = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = lib/tcti-tabrmd.pc

@VALGRIND_CHECK_RULES@
VALGRIND_G_DEBUG = fatal-criticals,gc-friendly
VALGRIND_memcheck_FLAGS = --show-leak-kinds=definite,indirect

src_libutil_la_SOURCES = src/tabrmd-generated.c src/tpm2-header.c src/util.c

src_libtcti_tabrmd_la_LIBADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) \
    $(PTHREAD_LIBS) $(noinst_LTLIBRARIES) $(SAPI_LIBS)
src_libtcti_tabrmd_la_LDFLAGS = -fPIC -Wl,--no-undefined -Wl,--version-script=$(srcdir)/src/tcti-tabrmd.map
src_libtcti_tabrmd_la_SOURCES = src/tcti-tabrmd.c

src_libtcti_echo_la_SOURCES = src/tss2-tcti-echo.c

src_tpm2_abrmd_LDADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) \
    $(SAPI_LIBS) $(noinst_LTLIBRARIES) $(noinst_LTLIBRARIES)
src_tpm2_abrmd_SOURCES = \
    src/access-broker.c \
    src/command-attrs.c \
    src/control-message.c \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/message-queue.c \
    src/tabrmd.c \
    src/logging.c \
    src/thread-interface.c \
    src/command-source.c \
    src/session-manager.c \
    src/session-data.c \
    src/random.c \
    src/resource-manager.c \
    src/response-sink.c \
    src/sink-interface.c \
    src/source-interface.c \
    src/tcti.c \
    src/tcti-options.c \
    src/tcti-echo.c \
    src/tcti-type-enum.c \
    src/tpm2-command.c \
    src/tpm2-response.c
if TCTI_DEVICE
src_tpm2_abrmd_LDADD += $(TCTI_DEVICE_LIBS)
src_tpm2_abrmd_SOURCES += src/tcti-device.c
endif
if TCTI_SOCKET
src_tpm2_abrmd_LDADD += $(TCTI_SOCKET_LIBS)
src_tpm2_abrmd_SOURCES += src/tcti-socket.c
endif

man/man8/%.8 : man/%.8.in
	rm -f $@
	mkdir -p man/man8
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

%-generated.c %-generated.h : src/tabrmd.xml
	$(GDBUS_CODEGEN) --interface-prefix=com.intel.tss2. \
	    --generate-c-code=src/tabrmd-generated $^

dist/%.service : dist/%.service.in
	sed -e 's,[@]SBINDIR[@],$(sbindir),g' $^ > $@

%.pc : %.pc.in
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	sed -e "s,[@]VERSION[@],$(PACKAGE_VERSION),g; \
                s,[@]includedir[@],$(includedir),g;" $^ > $@

test_test_skeleton_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_test_skeleton_unit_LDADD   = $(CMOCKA_LIBS)
test_test_skeleton_unit_SOURCES = test/test-skeleton_unit.c

test_response_sink_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_response_sink_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) \
    $(PTHREAD_LIBS) $(noinst_LTLIBRARIES) $(GOBJECT_LIBS)
test_response_sink_unit_SOURCES = \
    src/control-message.c \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/thread-interface.c \
    src/message-queue.c \
    src/response-sink.c \
    src/session-data.c \
    src/sink-interface.c \
    src/tpm2-response.c \
    test/response-sink_unit.c

test_session_data_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_session_data_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_session_data_unit_SOURCES = \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/session-data.c \
    test/session-data_unit.c

test_session_manager_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_session_manager_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_session_manager_unit_SOURCES = \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/session-manager.c \
    src/session-data.c \
    test/session-manager_unit.c

test_command_attrs_unit_CFLAGS   = $(UNIT_AM_CFLAGS)
test_command_attrs_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) $(GOBJECT_LIBS) $(noinst_LTLIBRARIES)
test_command_attrs_unit_LDFLAGS  = -Wl,--wrap=access_broker_lock_sapi,--wrap=access_broker_get_max_command,--wrap=Tss2_Sys_GetCapability
test_command_attrs_unit_SOURCES  = \
    src/access-broker.c \
    src/command-attrs.c \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/session-data.c \
    src/tcti.c \
    src/tcti-echo.c \
    src/tpm2-command.c \
    src/tpm2-response.c \
    test/command-attrs_unit.c

test_command_source_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_command_source_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) $(PTHREAD_LIBS) $(GOBJECT_LIBS) $(noinst_LTLIBRARIES)
test_command_source_unit_LDFLAGS = -Wl,--wrap=session_manager_lookup_fd,--wrap=tpm2_command_new_from_fd,--wrap=sink_enqueue
test_command_source_unit_SOURCES = \
    src/access-broker.c \
    src/command-attrs.c \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/message-queue.c \
    src/command-source.c \
    src/session-manager.c \
    src/session-data.c \
    src/sink-interface.c \
    src/source-interface.c \
    src/tcti.c \
    src/thread-interface.c \
    src/tpm2-command.c \
    src/tpm2-response.c \
    test/command-source_unit.c

test_handle_map_entry_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_handle_map_entry_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_handle_map_entry_unit_SOURCES = \
    src/handle-map-entry.c \
    test/handle-map-entry_unit.c

test_handle_map_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_handle_map_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) $(PTHREAD_LIBS) $(GOBJECT_LIBS)
test_handle_map_unit_SOURCES = \
    src/handle-map.c \
    src/handle-map-entry.c \
    test/handle-map_unit.c

if TCTI_DEVICE
test_tcti_device_unit_CFLAGS   = $(UNIT_AM_CFLAGS)
test_tcti_device_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_tcti_device_unit_LDFLAGS  = -Wl,--wrap=InitDeviceTcti
test_tcti_device_unit_SOURCES  = \
    src/tcti.c \
    src/tcti-device.c \
    test/tcti-device_unit.c
endif

if TCTI_SOCKET
test_tcti_socket_unit_CFLAGS   = $(UNIT_AM_CFLAGS)
test_tcti_socket_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(TCTI_SOCKET_LIBS) $(GOBJECT_LIBS)
test_tcti_socket_unit_LDFLAGS  = -Wl,--wrap=InitSocketTcti
test_tcti_socket_unit_SOURCES  = \
    src/tcti.c \
    src/tcti-socket.c \
    test/tcti-socket_unit.c
endif

test_tcti_options_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tcti_options_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(noinst_LTLIBRARIES)
test_tcti_options_unit_SOURCES = \
    src/tcti.c \
    src/tcti-echo.c \
    src/tcti-options.c \
    src/tcti-type-enum.c \
    test/tcti-options_unit.c
if TCTI_DEVICE
test_tcti_options_unit_LDADD   += $(TCTI_DEVICE_LIBS)
test_tcti_options_unit_SOURCES += src/tcti-device.c
endif
if TCTI_SOCKET
test_tcti_options_unit_LDADD   += $(TCTI_SOCKET_LIBS)
test_tcti_options_unit_SOURCES += src/tcti-socket.c
endif

test_tcti_tabrmd_read_write_LDADD   = $(GLIB_LIBS) src/libtcti-tabrmd.la
test_tcti_tabrmd_read_write_SOURCES = test/tcti-tabrmd_read-write.c

test_tpm2_command_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tpm2_command_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(noinst_LTLIBRARIES)
test_tpm2_command_unit_SOURCES = \
    src/access-broker.c \
    src/command-attrs.c \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/tpm2-command.c \
    src/tpm2-response.c \
    src/session-data.c \
    src/tcti.c \
    test/tpm2-command_unit.c

test_tpm2_response_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tpm2_response_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS)
test_tpm2_response_unit_SOURCES = \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/tpm2-response.c \
    src/session-data.c \
    test/tpm2-response_unit.c

test_util_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_util_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) $(noinst_LTLIBRARIES)
test_util_unit_LDFLAGS = -Wl,--wrap=read,--wrap=write
test_util_unit_SOURCES = test/util_unit.c

test_util_mock_realloc_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_util_mock_realloc_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) $(noinst_LTLIBRARIES)
test_util_mock_realloc_unit_LDFLAGS = -Wl,--wrap=realloc
test_util_mock_realloc_unit_SOURCES = test/util-mock-realloc_unit.c

test_message_queue_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_message_queue_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_message_queue_unit_SOURCES = \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/message-queue.c \
    src/session-data.c \
    test/message-queue_unit.c

test_access_broker_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_access_broker_unit_LDFLAGS = -Wl,--wrap=Tss2_Sys_Startup,--wrap=Tss2_Sys_GetCapability,--wrap=tcti_echo_transmit
test_access_broker_unit_LDADD = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(PTHREAD_LIBS) $(noinst_LTLIBRARIES)
test_access_broker_unit_SOURCES = \
    src/access-broker.c \
    src/command-attrs.c \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/session-data.c \
    src/tcti.c \
    src/tcti-echo.c \
    src/tpm2-command.c \
    src/tpm2-response.c \
    test/access-broker_unit.c

test_random_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_random_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_random_unit_LDFLAGS = -Wl,--wrap=open,--wrap=read,--wrap=close
test_random_unit_SOURCES = \
    src/random.c \
    test/random_unit.c

test_resource_manager_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_resource_manager_unit_LDFLAGS = -Wl,--wrap=access_broker_send_command,--wrap=sink_enqueue,--wrap=access_broker_context_saveflush,--wrap=access_broker_context_load
test_resource_manager_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(PTHREAD_LIBS) $(noinst_LTLIBRARIES)
test_resource_manager_unit_SOURCES = \
    src/access-broker.c \
    src/command-attrs.c \
    src/control-message.c \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/message-queue.c \
    src/resource-manager.c \
    src/session-data.c \
    src/sink-interface.c \
    src/source-interface.c \
    src/tcti.c \
    src/tcti-echo.c \
    src/thread-interface.c \
    src/tpm2-command.c \
    src/tpm2-response.c \
    test/resource-manager_unit.c

test_tcti_echo_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tcti_echo_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(noinst_LTLIBRARIES)
test_tcti_echo_unit_SOURCES = \
    src/tcti.c \
    src/tcti-echo.c \
    test/tcti-echo_unit.c

test_tss2_tcti_echo_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tss2_tcti_echo_unit_LDADD   = $(CMOCKA_LIBS) $(noinst_LTLIBRARIES)
test_tss2_tcti_echo_unit_SOURCES = \
    test/tss2-tcti-echo_unit.c

test_integration_create_keys_LDADD   = $(GLIB_LIBS) $(lib_LTLIBRARIES) $(SAPI_LIBS)
test_integration_create_keys_SOURCES = \
    test/integration/create-keys.c
