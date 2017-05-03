# we have BUILT_SOURCES so builddir must be in VPATH
VPATH = $(srcdir) $(builddir)
ACLOCAL_AMFLAGS = -I m4

.PHONY: unit-count

unit-count: check
	sh scripts/unit-count.sh

AM_CFLAGS = $(EXTRA_CFLAGS) \
    -I$(srcdir)/src -I$(srcdir)/src/include -I$(builddir)/src \
    $(DBUS_CFLAGS) $(GIO_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) \
    $(SAPI_CFLAGS) $(TCTI_DEVICE_CFLAGS) $(TCTI_SOCKET_CFLAGS)
AM_LDFLAGS = $(EXTRA_LDFLAGS)
UNIT_AM_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS)

if UNIT
TESTS_UNIT = \
    test/access-broker_unit \
    test/command-attrs_unit \
    test/connection_unit \
    test/connection-manager_unit \
    test/message-queue_unit \
    test/resource-manager_unit \
    test/response-sink_unit \
    test/command-source_unit \
    test/handle-map-entry_unit \
    test/handle-map_unit \
    test/random_unit \
    test/session-entry_unit \
    test/test-skeleton_unit \
    test/tcti-echo_unit \
    test/tcti-options_unit \
    test/tpm2-command_unit \
    test/tpm2-response_unit \
    test/tss2-tcti-tabrmd_unit \
    test/tss2-tcti-echo_unit \
    test/util_unit
if TCTI_DEVICE
    TESTS_UNIT += test/tcti-device_unit
endif
if TCTI_SOCKET
    TESTS_UNIT += test/tcti-socket_unit
endif
endif #UNIT

sbin_PROGRAMS   = src/tpm2-abrmd
noinst_PROGRAMS = \
    test/integration/create-keys \
    test/integration/get-capability-handles-transient \
    test/integration/manage-transient-keys \
    test/integration/start-auth-session \
    test/integration/tcti-cancel \
    test/integration/tcti-sessions-max \
    test/integration/tcti-set-locality \
    test/integration/hash-sequence \
    test/integration/password-authorization \
    test/tcti-tabrmd_read-write
check_PROGRAMS  = $(TESTS_UNIT)

# libraries
libtcti_tabrmd = src/libtcti-tabrmd.la
libtcti_echo   = src/libtcti-echo.la
libtest        = test/integration/libtest.la
libutil        = src/libutil.la

lib_LTLIBRARIES = src/libtcti-tabrmd.la
noinst_LTLIBRARIES = \
    $(libtcti_echo) \
    $(libutil) \
    $(libtest)
man8_MANS = man/man8/tpm2-abrmd.8
TESTS = $(check_PROGRAMS)

tpm2_abrmddir      = $(includedir)/tss2
tpm2_abrmd_HEADERS = $(srcdir)/src/include/tabrmd.h
libtcti_tabrmddir      = $(includedir)/tcti
libtcti_tabrmd_HEADERS = $(srcdir)/src/include/tcti-tabrmd.h

CLEANFILES = \
    src/tabrmd-generated.c \
    src/tabrmd-generated.h \
    $(systemdsystemunit_DATA)
BUILT_SOURCES = src/tabrmd-generated.h src/tabrmd-generated.c

pkgconfigdir          = $(libdir)/pkgconfig
dist_pkgconfig_DATA   = dist/tcti-tabrmd.pc
dist_dbuspolicy_DATA  = dist/tpm2-abrmd.conf
dist_udevrules_DATA   = dist/tpm-udev.rules
if HAVE_SYSTEMD
dist_systemdsystemunit_DATA = dist/tpm2-abrmd.service
endif # HAVE_SYSTEMD


@VALGRIND_CHECK_RULES@
VALGRIND_G_DEBUG = fatal-criticals,gc-friendly
VALGRIND_memcheck_FLAGS = --show-leak-kinds=definite,indirect --track-origins=yes

# utility library with most of the code that makes up the daemon
src_libutil_la_LIBADD  = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) \
    $(SAPI_LIBS)
src_libutil_la_SOURCES = \
    src/access-broker.c \
    src/command-attrs.c \
    src/connection.c \
    src/connection-manager.c \
    src/control-message.c \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/message-queue.c \
    src/logging.c \
    src/thread.c \
    src/command-source.c \
    src/random.c \
    src/resource-manager.c \
    src/response-sink.c \
    src/session-entry.c \
    src/session-entry-state-enum.c \
    src/session-list.c \
    src/sink-interface.c \
    src/source-interface.c \
    src/tcti.c \
    src/tcti-options.c \
    src/tcti-type-enum.c \
    src/tpm2-command.c \
    src/tpm2-response.c \
    src/tabrmd-error.c \
    src/tabrmd-generated.c \
    src/tpm2-header.c \
    src/util.c
if TCTI_DEVICE
src_libutil_la_LIBADD  += $(TCTI_DEVICE_LIBS)
src_libutil_la_SOURCES += src/tcti-device.c
endif
if TCTI_SOCKET
src_libutil_la_LIBADD  += $(TCTI_SOCKET_LIBS)
src_libutil_la_SOURCES += src/tcti-socket.c
endif

test_integration_libtest_la_LIBADD  = $(SAPI_LIBS) $(TCTI_SOCKET_LIBS) $(TCTI_DEVICE_LIBS) $(GLIB_LIBS)
test_integration_libtest_la_SOURCES = \
    test/integration/common.c \
    test/integration/context-util.c \
    test/integration/test-options.c

src_libtcti_tabrmd_la_LIBADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) \
    $(PTHREAD_LIBS) $(noinst_LTLIBRARIES) $(SAPI_LIBS)
src_libtcti_tabrmd_la_LDFLAGS = -fPIC -Wl,--no-undefined -Wl,--version-script=$(srcdir)/src/tcti-tabrmd.map
src_libtcti_tabrmd_la_SOURCES = src/tcti-tabrmd.c

src_libtcti_echo_la_LIBADD  = $(DBUS_LIBS) $(GLIB_LIBS)
src_libtcti_echo_la_SOURCES = src/tss2-tcti-echo.c src/tcti-echo.c

src_tpm2_abrmd_LDADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) \
    $(SAPI_LIBS) $(libutil) $(libtcti_echo)
src_tpm2_abrmd_SOURCES = src/tabrmd.c

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
	mkdir -p src/
	$(GDBUS_CODEGEN) --interface-prefix=com.intel.tss2. \
	    --generate-c-code=src/tabrmd-generated $^

dist/%.service : dist/%.service.in
	sed -e 's,[@]SBINDIR[@],$(sbindir),g' $^ > $@

%.pc : %.pc.in
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	sed -e "s,[@]VERSION[@],$(PACKAGE_VERSION),g; \
                s,[@]includedir[@],$(includedir),g;" $^ > $@

if UNIT
test_test_skeleton_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_test_skeleton_unit_LDADD   = $(CMOCKA_LIBS)
test_test_skeleton_unit_SOURCES = test/test-skeleton_unit.c

test_response_sink_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_response_sink_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) \
    $(PTHREAD_LIBS) $(noinst_LTLIBRARIES) $(GOBJECT_LIBS)
test_response_sink_unit_SOURCES = \
    src/connection.c \
    src/control-message.c \
    src/handle-map.c \
    src/handle-map-entry.c \
    src/thread.c \
    src/message-queue.c \
    src/response-sink.c \
    src/sink-interface.c \
    src/tpm2-response.c \
    test/response-sink_unit.c

test_connection_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_connection_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_connection_unit_SOURCES = test/connection_unit.c

test_connection_manager_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_connection_manager_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_connection_manager_unit_SOURCES = test/connection-manager_unit.c

test_command_attrs_unit_CFLAGS   = $(UNIT_AM_CFLAGS)
test_command_attrs_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) $(GOBJECT_LIBS) $(libutil) $(libtcti_echo)
test_command_attrs_unit_LDFLAGS  = -Wl,--wrap=access_broker_lock_sapi,--wrap=access_broker_get_max_command,--wrap=Tss2_Sys_GetCapability
test_command_attrs_unit_SOURCES  = test/command-attrs_unit.c

test_command_source_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_command_source_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) $(PTHREAD_LIBS) $(GOBJECT_LIBS) $(libutil)
test_command_source_unit_LDFLAGS = -Wl,--wrap=connection_manager_lookup_fd,--wrap=sink_enqueue,--wrap=read_data
test_command_source_unit_SOURCES = test/command-source_unit.c

test_handle_map_entry_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_handle_map_entry_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_handle_map_entry_unit_SOURCES = test/handle-map-entry_unit.c

test_handle_map_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_handle_map_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) $(PTHREAD_LIBS) $(GOBJECT_LIBS) $(libutil)
test_handle_map_unit_SOURCES = test/handle-map_unit.c

if TCTI_DEVICE
test_tcti_device_unit_CFLAGS   = $(UNIT_AM_CFLAGS)
test_tcti_device_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_tcti_device_unit_LDFLAGS  = -Wl,--wrap=InitDeviceTcti
test_tcti_device_unit_SOURCES  = test/tcti-device_unit.c
endif

if TCTI_SOCKET
test_tcti_socket_unit_CFLAGS   = $(UNIT_AM_CFLAGS)
test_tcti_socket_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(TCTI_SOCKET_LIBS) $(GOBJECT_LIBS) $(libutil)
test_tcti_socket_unit_LDFLAGS  = -Wl,--wrap=InitSocketTcti
test_tcti_socket_unit_SOURCES  = test/tcti-socket_unit.c
endif

test_tcti_options_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tcti_options_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil) $(libtcti_echo)
test_tcti_options_unit_SOURCES = test/tcti-options_unit.c
if TCTI_DEVICE
test_tcti_options_unit_LDADD   += $(TCTI_DEVICE_LIBS)
test_tcti_options_unit_SOURCES += src/tcti-device.c
endif
if TCTI_SOCKET
test_tcti_options_unit_LDADD   += $(TCTI_SOCKET_LIBS)
test_tcti_options_unit_SOURCES += src/tcti-socket.c
endif

test_tpm2_command_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tpm2_command_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(libutil)
test_tpm2_command_unit_SOURCES = test/tpm2-command_unit.c

test_tpm2_response_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tpm2_response_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(libutil)
test_tpm2_response_unit_SOURCES = test/tpm2-response_unit.c

test_util_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_util_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(libutil)
test_util_unit_LDFLAGS = -Wl,--wrap=read,--wrap=write
test_util_unit_SOURCES = test/util_unit.c

test_message_queue_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_message_queue_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_message_queue_unit_SOURCES = test/message-queue_unit.c

test_access_broker_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_access_broker_unit_LDFLAGS = -Wl,--wrap=Tss2_Sys_Startup,--wrap=Tss2_Sys_GetCapability,--wrap=tcti_echo_transmit
test_access_broker_unit_LDADD = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(PTHREAD_LIBS) $(libutil) $(libtcti_echo)
test_access_broker_unit_SOURCES = test/access-broker_unit.c

test_random_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_random_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_random_unit_LDFLAGS = -Wl,--wrap=open,--wrap=read,--wrap=close
test_random_unit_SOURCES = test/random_unit.c

test_session_entry_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_session_entry_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_session_entry_unit_SOURCES = test/session-entry_unit.c

test_resource_manager_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_resource_manager_unit_LDFLAGS = -Wl,--wrap=access_broker_send_command,--wrap=sink_enqueue,--wrap=access_broker_context_saveflush,--wrap=access_broker_context_load
test_resource_manager_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(PTHREAD_LIBS) $(libutil) $(libtcti_echo)
test_resource_manager_unit_SOURCES = test/resource-manager_unit.c

test_tcti_echo_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tcti_echo_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil) $(libtcti_echo)
test_tcti_echo_unit_SOURCES = test/tcti-echo_unit.c

test_tss2_tcti_tabrmd_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tss2_tcti_tabrmd_unit_LDFLAGS = -Wl,--wrap=g_dbus_proxy_call_with_unix_fd_list_sync,--wrap=tcti_tabrmd_call_cancel_sync,--wrap=tcti_tabrmd_call_set_locality_sync,--wrap=read_data,--wrap=poll
test_tss2_tcti_tabrmd_unit_LDADD   = $(CMOCKA_LIBS) $(GIO_LIBS) $(libutil)
test_tss2_tcti_tabrmd_unit_SOURCES = src/tcti-tabrmd.c test/tss2-tcti-tabrmd_unit.c

test_tss2_tcti_echo_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tss2_tcti_echo_unit_LDADD   = $(CMOCKA_LIBS) $(libtcti_echo)
test_tss2_tcti_echo_unit_SOURCES = test/tss2-tcti-echo_unit.c
endif

test_tcti_tabrmd_read_write_LDADD   = $(GLIB_LIBS) $(libtcti_tabrmd)
test_tcti_tabrmd_read_write_SOURCES = test/tcti-tabrmd_read-write.c

test_integration_create_keys_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(TCTI_DEVICE_LIBS) \
    $(TCTI_SOCKET_LIBS)
test_integration_create_keys_SOURCES = test/integration/main.c test/integration/create-keys.c

test_integration_get_capability_handles_transient_LDADD   = $(libtest) \
    $(libtcti_tabrmd) $(GLIB_LIBS) $(SAPI_LIBS)
test_integration_get_capability_handles_transient_SOURCES = \
    test/integration/main.c test/integration/get-capability-handles-transient.c

test_integration_start_auth_session_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS)
test_integration_start_auth_session_SOURCES = test/integration/main.c test/integration/start-auth-session.c

test_integration_tcti_cancel_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_tcti_cancel_SOURCES = test/integration/main.c test/integration/tcti-cancel.c

test_integration_tcti_sessions_max_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_tcti_sessions_max_SOURCES = test/integration/tcti-sessions-max.c

test_integration_tcti_set_locality_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_tcti_set_locality_SOURCES = test/integration/main.c test/integration/tcti-set-locality.c

test_integration_hash_sequence_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_hash_sequence_SOURCES = test/integration/main.c test/integration/hash-sequence.c

test_integration_password_authorization_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_password_authorization_SOURCES = test/integration/main.c test/integration/password-authorization.c

test_integration_manage_transient_keys_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS)
test_integration_manage_transient_keys_SOURCES = test/integration/manage-transient-keys.c
