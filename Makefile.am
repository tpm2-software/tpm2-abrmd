# we have BUILT_SOURCES so builddir must be in VPATH
VPATH = $(srcdir) $(builddir)
ACLOCAL_AMFLAGS = -I m4

.PHONY: unit-count

unit-count: check
	sh scripts/unit-count.sh

AM_CFLAGS = $(EXTRA_CFLAGS) \
    -I$(srcdir)/src -I$(srcdir)/src/include -I$(builddir)/src \
    $(DBUS_CFLAGS) $(GIO_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) \
    $(SAPI_CFLAGS) $(TCTI_DEVICE_CFLAGS) $(TCTI_SOCKET_CFLAGS) \
    $(CODE_COVERAGE_CFLAGS)
AM_LDFLAGS = $(EXTRA_LDFLAGS) $(CODE_COVERAGE_LIBS)
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
    test/ipc-frontend_unit \
    test/ipc-frontend-dbus_unit \
    test/random_unit \
    test/session-entry_unit \
    test/test-skeleton_unit \
    test/tcti-echo_unit \
    test/thread_unit \
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

tests_integration = \
    test/integration/auth-session-max.int \
    test/integration/auth-session-start-flush.int \
    test/integration/auth-session-start-save.int \
    test/integration/auth-session-start-save-load.int \
    test/integration/create-keys.int \
    test/integration/get-capability-handles-transient.int \
    test/integration/manage-transient-keys.int \
    test/integration/session-load-from-closed-connection.int \
    test/integration/session-load-from-closed-connections-lru.int \
    test/integration/session-load-from-open-connection.int \
    test/integration/start-auth-session.int \
    test/integration/tcti-cancel.int \
    test/integration/tcti-connect-multiple.int \
    test/integration/tcti-sessions-max.int \
    test/integration/tcti-set-locality.int \
    test/integration/hash-sequence.int \
    test/integration/not-enough-handles-for-command.int \
    test/integration/password-authorization.int \
    test/integration/tpm2-command-flush-no-handle.int \
    test/integration/util-buf-max-upper-bound.int

if SIMULATOR_BIN
TESTS_INTEGRATION = $(tests_integration)
endif

if HWTPM
TESTS_INTEGRATION = $(tests_integration)
endif

XFAIL_TESTS = \
    test/integration/start-auth-session.int \
    test/integration/tcti-sessions-max.int

TESTS = $(TESTS_UNIT) $(TESTS_INTEGRATION)
TEST_EXTENSIONS = .int
AM_TESTS_ENVIRONMENT = TEST_FUNC_LIB=$(srcdir)/scripts/int-test-funcs.sh
if HWTPM
INT_LOG_COMPILER = $(srcdir)/scripts/int-hardware-setup.sh
else
INT_LOG_COMPILER = $(srcdir)/scripts/int-simulator-setup.sh
endif
INT_LOG_FLAGS = --tabrmd-bin=$(sbin_PROGRAMS)
if SIMULATOR_BIN
INT_LOG_FLAGS += --simulator-bin=$(SIMULATOR_BIN)
endif

sbin_PROGRAMS   = src/tpm2-abrmd
check_PROGRAMS  = $(sbin_PROGRAMS) $(TESTS)

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
man3_MANS = man/man3/tss2_tcti_tabrmd_init.3
man7_MANS = man/man7/tcti-tabrmd.7
man8_MANS = man/man8/tpm2-abrmd.8

install-data-hook:
	$(LN_S) -f tss2_tcti_tabrmd_init.3 \
                $(DESTDIR)$(mandir)/man3/tss2_tcti_tabrmd_init_full.3
if WITH_UDEVRULESPREFIX
	mv $(DESTDIR)$(udevrulesdir)/tpm-udev.rules $(DESTDIR)$(udevrulesdir)/$(udevrulesprefix)tpm-udev.rules
endif

uninstall-local:
	-rm $(DESTDIR)$(mandir)/man3/tss2_tcti_tabrmd_init_full.3
	-rm $(DESTDIR)$(udevrulesdir)/$(udevrulesprefix)tpm-udev.rules

libtcti_tabrmddir      = $(includedir)/tcti
libtcti_tabrmd_HEADERS = $(srcdir)/src/include/tcti-tabrmd.h

EXTRA_DIST = \
    src/tabrmd.xml \
    test/integration/test.h \
    test/integration/tpm2-struct-init.h \
    src/tcti-tabrmd.map \
    man/colophon.in \
    man/tss2_tcti_tabrmd_init.3.in \
    man/tcti-tabrmd.7.in \
    man/tpm2-abrmd.8.in \
    dist/tpm2-abrmd.conf \
    dist/tcti-tabrmd.pc.in \
    dist/tpm2-abrmd.preset \
    dist/tpm2-abrmd.service.in \
    dist/tpm-udev.rules \
    scripts/int-simulator-setup.sh \
    scripts/int-hardware-setup.sh \
    scripts/int-test-funcs.sh \
    AUTHORS \
    CHANGELOG.md \
    CONTRIBUTING.md \
    INSTALL.md \
    LICENSE \
    README.md \
    VERSION

CLEANFILES = \
    $(man3_MANS) \
    $(man7_MANS) \
    man/man8/tpm2-abrmd.8 \
    src/tabrmd-generated.c \
    src/tabrmd-generated.h \
    $(pkgconfig_DATA) \
    $(systemdsystemunit_DATA) \
    test/integration/*.log \
    _tabrmd.log

BUILT_SOURCES = src/tabrmd-generated.h src/tabrmd-generated.c

pkgconfigdir     = $(libdir)/pkgconfig
pkgconfig_DATA   = dist/tcti-tabrmd.pc
dbuspolicy_DATA  = dist/tpm2-abrmd.conf
udevrules_DATA   = dist/tpm-udev.rules
if HAVE_SYSTEMD
systemdsystemunit_DATA = dist/tpm2-abrmd.service
endif # HAVE_SYSTEMD
systemdpreset_DATA = dist/tpm2-abrmd.preset

@CODE_COVERAGE_RULES@
@VALGRIND_CHECK_RULES@
VALGRIND_G_DEBUG = fatal-criticals,gc-friendly
VALGRIND_memcheck_FLAGS = --leak-check=full --show-leak-kinds=definite,indirect --track-origins=yes --error-exitcode=1

# This is a hack required to ensure that BUILT_SOURCES are generated before
# the file that requires them.
src/ipc-frontend-dbus.c : $(BUILT_SOURCES)

# utility library with most of the code that makes up the daemon
src_libutil_la_LIBADD  = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) \
    $(SAPI_LIBS) $(TCTI_DEVICE_LIBS) $(TCTI_SOCKET_LIBS)
src_libutil_la_SOURCES = \
    src/access-broker.c \
    src/access-broker.h \
    src/command-attrs.c \
    src/command-attrs.h \
    src/command-source.c \
    src/command-source.h \
    src/connection.c \
    src/connection.h \
    src/connection-manager.c \
    src/connection-manager.h \
    src/control-message.c \
    src/control-message.h \
    src/handle-map-entry.c \
    src/handle-map-entry.h \
    src/handle-map.c \
    src/handle-map.h \
    src/ipc-frontend.c \
    src/ipc-frontend.h \
    src/ipc-frontend-dbus.h \
    src/ipc-frontend-dbus.c \
    src/logging.c \
    src/logging.h \
    src/message-queue.c \
    src/message-queue.h \
    src/random.c \
    src/random.h \
    src/resource-manager.c \
    src/resource-manager.h \
    src/response-sink.c \
    src/response-sink.h \
    src/session-entry-state-enum.c \
    src/session-entry-state-enum.h \
    src/session-entry.c \
    src/session-entry.h \
    src/session-list.c \
    src/session-list.h \
    src/sink-interface.c \
    src/sink-interface.h \
    src/source-interface.c \
    src/source-interface.h \
    src/tabrmd-error.c \
    src/tabrmd-generated.c \
    src/tabrmd-generated.h \
    src/tabrmd.h \
    src/tcti.c \
    src/tcti.h \
    src/tcti-options.c \
    src/tcti-options.h \
    src/tcti-type-enum.c \
    src/tcti-type-enum.h \
    src/thread.c \
    src/thread.h \
    src/tpm2-command.c \
    src/tpm2-command.h \
    src/tpm2-header.c \
    src/tpm2-header.h \
    src/tpm2-response.c \
    src/tpm2-response.h \
    src/util.c \
    src/util.h
if TCTI_DEVICE
src_libutil_la_SOURCES += src/tcti-device.c src/tcti-device.h
endif
if TCTI_SOCKET
src_libutil_la_SOURCES += src/tcti-socket.c src/tcti-socket.h
endif

test_integration_libtest_la_LIBADD  = $(SAPI_LIBS) $(TCTI_SOCKET_LIBS) $(TCTI_DEVICE_LIBS) $(GLIB_LIBS)
test_integration_libtest_la_SOURCES = \
    test/integration/common.c \
    test/integration/common.h \
    test/integration/context-util.c \
    test/integration/context-util.h \
    test/integration/test-options.c \
    test/integration/test-options.h

src_libtcti_tabrmd_la_LIBADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) \
    $(PTHREAD_LIBS) $(noinst_LTLIBRARIES) $(SAPI_LIBS)
src_libtcti_tabrmd_la_LDFLAGS = -fPIC -Wl,--no-undefined -Wl,--version-script=$(srcdir)/src/tcti-tabrmd.map
src_libtcti_tabrmd_la_SOURCES = src/tcti-tabrmd.c src/tcti-tabrmd-priv.h $(srcdir)/src/tcti-tabrmd.map

src_libtcti_echo_la_LIBADD  = $(DBUS_LIBS) $(GLIB_LIBS)
src_libtcti_echo_la_SOURCES = \
    src/tcti-echo.c \
    src/tcti-echo.h \
    src/tss2-tcti-echo.c \
    src/tss2-tcti-echo.h \
    src/tss2-tcti-echo-priv.h

src_tpm2_abrmd_LDADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) \
    $(SAPI_LIBS) $(libutil) $(libtcti_echo)
src_tpm2_abrmd_SOURCES = src/tabrmd.c

AUTHORS :
	git log --format='%aN <%aE>' | grep -v 'users.noreply.github.com' | sort | \
	    uniq -c | sort -nr | sed 's/^\s*//' | cut -d" " -f2- > $@

man/man3/%.3 : man/%.3.in
	$(call man_tcti_prefix,$@,$^)

man/man7/%.7 : man/%.7.in
	$(call man_tcti_prefix,$@,$^)

man/man8/%.8 : man/%.8.in
	$(call man_tcti_prefix,$@,$^)

# this function is used to generate man pages from templates found in
# $(srcdir)/man/*.in
define man_tcti_prefix
	$(call make_parent_dir,$@)
	-rm $@
if TCTI_DEVICE
	echo ".nr HAVE_DEVICE_TCTI 1" >> $1
else
	echo ".nr HAVE_DEVICE_TCTI 0" >> $1
endif
if TCTI_SOCKET
	echo ".nr HAVE_SOCKET_TCTI 1" >> $1
else
	echo ".nr HAVE_SOCKET_TCTI 0" >> $1
endif
	sed -e "s,[@]VERSION[@],$(PACKAGE_VERSION),g;" $2 $(srcdir)/man/colophon.in >> $1
endef # man_tcti_prefix

%-generated.c %-generated.h : src/tabrmd.xml
	$(call make_parent_dir,$@)
	$(GDBUS_CODEGEN) --interface-prefix=com.intel.tss2. \
	    --generate-c-code=src/tabrmd-generated $^

%.preset : %.preset.in
	$(call make_parent_dir,$@)
	sed -e "s,[@]SYSTEMD_PRESET_DEFAULT[@],$(SYSTEMD_PRESET_DEFAULT),g;" $^ > $@

%.service : %.service.in
	$(call make_parent_dir,$@)
	sed -e "s,[@]SBINDIR[@],$(sbindir),g; \
	        s,[@]sysconfdir[@],$(sysconfdir),g;" $^ > $@

%.pc : %.pc.in
	$(call make_parent_dir,$@)
	sed -e "s,[@]VERSION[@],$(PACKAGE_VERSION),g; \
                s,[@]includedir[@],$(includedir),g;" $^ > $@

define make_parent_dir
    if [ ! -d $(dir $1) ]; then mkdir -p $(dir $1); fi
endef

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
test_command_source_unit_LDFLAGS = -Wl,--wrap=g_source_set_callback,--wrap=connection_manager_lookup_istream,--wrap=connection_manager_remove,--wrap=sink_enqueue,--wrap=read_tpm_buffer_alloc,--wrap=command_attrs_from_cc
test_command_source_unit_SOURCES = test/command-source_unit.c

test_handle_map_entry_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_handle_map_entry_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_handle_map_entry_unit_SOURCES = test/handle-map-entry_unit.c

test_handle_map_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_handle_map_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(SAPI_LIBS) $(PTHREAD_LIBS) $(GOBJECT_LIBS) $(libutil)
test_handle_map_unit_SOURCES = test/handle-map_unit.c

test_ipc_frontend_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_ipc_frontend_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_ipc_frontend_unit_SOURCES = test/ipc-frontend_unit.c

test_ipc_frontend_dbus_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_ipc_frontend_dbus_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_ipc_frontend_dbus_unit_SOURCES = test/ipc-frontend-dbus_unit.c

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

test_thread_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_thread_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(libutil)
test_thread_unit_SOURCES = test/thread_unit.c

test_tpm2_command_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tpm2_command_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(libutil)
test_tpm2_command_unit_SOURCES = test/tpm2-command_unit.c

test_tpm2_response_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tpm2_response_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(libutil)
test_tpm2_response_unit_SOURCES = test/tpm2-response_unit.c

test_util_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_util_unit_LDADD  = $(CMOCKA_LIBS) $(GLIB_LIBS) $(libutil)
test_util_unit_LDFLAGS = -Wl,--wrap=g_input_stream_read,--wrap=g_output_stream_write
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

test_integration_auth_session_max_int_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS)
test_integration_auth_session_max_int_SOURCES = test/integration/main.c \
    test/integration/auth-session-max.int.c

test_integration_auth_session_start_flush_int_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS)
test_integration_auth_session_start_flush_int_SOURCES = test/integration/main.c \
    test/integration/auth-session-start-flush.int.c

test_integration_auth_session_start_save_int_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS)
test_integration_auth_session_start_save_int_SOURCES = test/integration/main.c \
    test/integration/auth-session-start-save.int.c

test_integration_auth_session_start_save_load_int_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS)
test_integration_auth_session_start_save_load_int_SOURCES = test/integration/main.c \
    test/integration/auth-session-start-save-load.int.c

test_integration_create_keys_int_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS) $(TCTI_DEVICE_LIBS) \
    $(TCTI_SOCKET_LIBS)
test_integration_create_keys_int_SOURCES = test/integration/main.c test/integration/create-keys.int.c

test_integration_get_capability_handles_transient_int_LDADD   = $(libtest) \
    $(libtcti_tabrmd) $(GLIB_LIBS) $(SAPI_LIBS)
test_integration_get_capability_handles_transient_int_SOURCES = \
    test/integration/main.c test/integration/get-capability-handles-transient.int.c

test_integration_session_load_from_open_connection_int_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS)
test_integration_session_load_from_open_connection_int_SOURCES = \
    test/integration/session-load-from-open-connection.int.c

test_integration_session_load_from_closed_connection_int_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS)
test_integration_session_load_from_closed_connection_int_SOURCES = \
    test/integration/session-load-from-closed-connection.int.c

test_integration_session_load_from_closed_connections_lru_int_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS)
test_integration_session_load_from_closed_connections_lru_int_SOURCES = \
    test/integration/session-load-from-closed-connections-lru.int.c

test_integration_start_auth_session_int_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS)
test_integration_start_auth_session_int_SOURCES = test/integration/main.c test/integration/start-auth-session.int.c

test_integration_tcti_cancel_int_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_tcti_cancel_int_SOURCES = test/integration/main.c test/integration/tcti-cancel.int.c

test_integration_tcti_connect_multiple_int_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_tcti_connect_multiple_int_SOURCES = test/integration/tcti-connect-multiple.int.c

test_integration_tcti_sessions_max_int_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_tcti_sessions_max_int_SOURCES = test/integration/tcti-sessions-max.int.c

test_integration_tcti_set_locality_int_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_tcti_set_locality_int_SOURCES = test/integration/main.c test/integration/tcti-set-locality.int.c

test_integration_hash_sequence_int_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_hash_sequence_int_SOURCES = test/integration/main.c test/integration/hash-sequence.int.c

test_integration_not_enough_handles_for_command_int_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_not_enough_handles_for_command_int_SOURCES = test/integration/main.c test/integration/not-enough-handles-for-command.int.c

test_integration_password_authorization_int_LDADD   = $(libtest) $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_password_authorization_int_SOURCES = test/integration/main.c test/integration/password-authorization.int.c

test_integration_manage_transient_keys_int_LDADD   = $(libtest) $(libtcti_tabrmd) \
    $(GLIB_LIBS) $(GOBJECT_LIBS) $(SAPI_LIBS)
test_integration_manage_transient_keys_int_SOURCES = test/integration/main.c test/integration/manage-transient-keys.int.c

test_integration_tpm2_command_flush_no_handle_int_LDADD   = $(libtest) \
    $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_tpm2_command_flush_no_handle_int_SOURCES = \
    test/integration/main.c test/integration/tpm2-command-flush-no-handle.int.c

test_integration_util_buf_max_upper_bound_int_LDADD   = $(libtest) \
    $(libtcti_tabrmd) $(GLIB_LIBS) $(GOBJECT_LIBS)
test_integration_util_buf_max_upper_bound_int_SOURCES = \
    test/integration/main.c test/integration/util-buf-max-upper-bound.int.c
