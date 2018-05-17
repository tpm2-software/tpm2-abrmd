# we have BUILT_SOURCES so builddir must be in VPATH
VPATH = $(srcdir) $(builddir)
ACLOCAL_AMFLAGS = -I m4

.PHONY: unit-count

unit-count: check
	sh scripts/unit-count.sh

AM_CFLAGS = $(EXTRA_CFLAGS) \
    -I$(srcdir)/src -I$(srcdir)/src/include -I$(builddir)/src \
    $(DBUS_CFLAGS) $(GIO_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) \
    $(TSS2_SYS_CFLAGS) $(TCTI_DEVICE_CFLAGS) $(TCTI_SOCKET_CFLAGS) \
    $(CODE_COVERAGE_CFLAGS)
AM_LDFLAGS = $(EXTRA_LDFLAGS) $(CODE_COVERAGE_LIBS)
UNIT_AM_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS)

TESTS_UNIT = \
    test/access-broker_unit \
    test/command-attrs_unit \
    test/connection_unit \
    test/connection-manager_unit \
    test/logging_unit \
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
    test/tcti-dynamic_unit \
    test/tcti-echo_unit \
    test/tcti-util_unit \
    test/thread_unit \
    test/tpm2-command_unit \
    test/tpm2-response_unit \
    test/tss2-tcti-tabrmd_unit \
    test/tss2-tcti-echo_unit \
    test/util_unit

TESTS_INTEGRATION = \
    test/integration/auth-session-max.int \
    test/integration/auth-session-start-flush.int \
    test/integration/auth-session-start-save.int \
    test/integration/auth-session-start-save-load.int \
    test/integration/max-transient-upperbound.int \
    test/integration/get-capability-handles-transient.int \
    test/integration/manage-transient-keys.int \
    test/integration/session-load-from-closed-connection.int \
    test/integration/session-load-from-closed-connections-lru.int \
    test/integration/session-load-from-open-connection.int \
    test/integration/start-auth-session.int \
    test/integration/tcti-cancel.int \
    test/integration/tcti-connections-max.int \
    test/integration/tcti-double-finalize.int \
    test/integration/tcti-set-locality.int \
    test/integration/hash-sequence.int \
    test/integration/not-enough-handles-for-command.int \
    test/integration/password-authorization.int \
    test/integration/tpm2-command-flush-no-handle.int \
    test/integration/util-buf-max-upper-bound.int

TESTS_INTEGRATION_NOHW = test/integration/tcti-connect-multiple.int

TESTS = $(TESTS_INTEGRATION)
XFAIL_TESTS = \
    test/integration/start-auth-session.int
TEST_EXTENSIONS = .int
AM_TESTS_ENVIRONMENT = \
    TEST_FUNC_LIB=$(srcdir)/scripts/int-test-funcs.sh \
    PATH=./src:$(PATH)
INT_LOG_COMPILER = $(srcdir)/scripts/int-test-setup.sh
INT_LOG_FLAGS = --tabrmd-tcti=$(TABRMD_TCTI)

if HWTPM
TABRMD_TCTI = device
endif

if ENABLE_INTEGRATION
TABRMD_TCTI = mssim
TESTS += $(TESTS_INTEGRATION_NOHW)
endif

if UNIT
TESTS += $(TESTS_UNIT)
endif

sbin_PROGRAMS   = src/tpm2-abrmd
check_PROGRAMS  = $(sbin_PROGRAMS) $(TESTS)

# libraries
libtss2_tcti_tabrmd = src/libtss2-tcti-tabrmd.la
libtss2_tcti_echo = src/libtss2-tcti-echo.la
libtest        = test/integration/libtest.la
libutil        = src/libutil.la

lib_LTLIBRARIES = $(libtss2_tcti_tabrmd)
noinst_LTLIBRARIES = \
    $(libtss2_tcti_echo) \
    $(libtest) \
    $(libutil)
man3_MANS = man/man3/Tss2_Tcti_Tabrmd_Init.3
man7_MANS = man/man7/tss2-tcti-tabrmd.7
man8_MANS = man/man8/tpm2-abrmd.8

libtss2_tcti_tabrmddir      = $(includedir)/tss2
libtss2_tcti_tabrmd_HEADERS = $(srcdir)/src/include/tss2-tcti-tabrmd.h

EXTRA_DIST = \
    src/tabrmd.xml \
    test/integration/test.h \
    test/integration/tpm2-struct-init.h \
    src/tcti-tabrmd.map \
    man/colophon.in \
    man/Tss2_Tcti_Tabrmd_Init.3.in \
    man/tss2-tcti-tabrmd.7.in \
    man/tpm2-abrmd.8.in \
    dist/tpm2-abrmd.conf \
    dist/tpm2-abrmd.preset.in \
    dist/tss2-tcti-tabrmd.pc.in \
    dist/tpm2-abrmd.service.in \
    dist/com.intel.tss2.Tabrmd.service \
    scripts/int-test-funcs.sh \
    scripts/int-test-setup.sh \
    selinux/Makefile \
    selinux/tabrmd.fc \
    selinux/tabrmd.if \
    selinux/tabrmd.te \
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
    $(systemdpreset_DATA) \
    $(systemdsystemunit_DATA) \
    test/integration/*.log \
    _tabrmd.log

BUILT_SOURCES = src/tabrmd-generated.h src/tabrmd-generated.c

pkgconfigdir     = $(libdir)/pkgconfig
pkgconfig_DATA   = dist/tss2-tcti-tabrmd.pc
dbuspolicy_DATA  = dist/tpm2-abrmd.conf
if HAVE_SYSTEMD
systemdpreset_DATA = dist/tpm2-abrmd.preset
systemdsystemunit_DATA = dist/tpm2-abrmd.service
dbusservicedir   = $(datadir)/dbus-1/system-services
dbusservice_DATA = dist/com.intel.tss2.Tabrmd.service
endif # HAVE_SYSTEMD

@CODE_COVERAGE_RULES@
@VALGRIND_CHECK_RULES@
VALGRIND_G_DEBUG = fatal-criticals,gc-friendly
VALGRIND_memcheck_FLAGS = --leak-check=full --show-leak-kinds=definite,indirect --track-origins=yes --error-exitcode=1

# This is a hack required to ensure that BUILT_SOURCES are generated before
# the file that requires them.
src/ipc-frontend-dbus.c : $(BUILT_SOURCES)

# utility library with most of the code that makes up the daemon
# -Wno-unused-parameter for automatically-generated tabrmd-generated.c:
src_libutil_la_CFLAGS  = $(AM_CFLAGS) -Wno-unused-parameter
src_libutil_la_LIBADD  = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) \
    $(TSS2_SYS_LIBS) $(TCTI_DEVICE_LIBS) $(TCTI_SOCKET_LIBS)
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
    src/tcti-dynamic.c \
    src/tcti-dynamic.h \
    src/tcti-util.c \
    src/tcti-util.h \
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

test_integration_libtest_la_LIBADD  = $(TSS2_SYS_LIBS) $(TCTI_SOCKET_LIBS) \
    $(TCTI_DEVICE_LIBS) $(GLIB_LIBS)
test_integration_libtest_la_SOURCES = \
    test/integration/common.c \
    test/integration/common.h \
    test/integration/context-util.c \
    test/integration/context-util.h \
    test/integration/test-options.c \
    test/integration/test-options.h

src_libtss2_tcti_tabrmd_la_LIBADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) \
    $(PTHREAD_LIBS) $(noinst_LTLIBRARIES) $(TSS2_SYS_LIBS)
src_libtss2_tcti_tabrmd_la_LDFLAGS = -fPIC -Wl,--no-undefined -Wl,--version-script=$(srcdir)/src/tcti-tabrmd.map
src_libtss2_tcti_tabrmd_la_SOURCES = src/tcti-tabrmd.c src/tcti-tabrmd-priv.h $(srcdir)/src/tcti-tabrmd.map

src_libtss2_tcti_echo_la_LIBADD  = $(DBUS_LIBS) $(GLIB_LIBS)
src_libtss2_tcti_echo_la_SOURCES = \
    src/tcti-echo.c \
    src/tcti-echo.h \
    src/tss2-tcti-echo.c \
    src/tss2-tcti-echo.h \
    src/tss2-tcti-echo-priv.h

src_tpm2_abrmd_LDADD   = $(DBUS_LIBS) $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) \
    $(TSS2_SYS_LIBS) $(libutil) $(libtss2_tcti_echo)
src_tpm2_abrmd_SOURCES = src/tabrmd.c

AUTHORS :
	git log --format='%aN <%aE>' | grep -v 'users.noreply.github.com' | sort | \
	    uniq -c | sort -nr | sed 's/^\s*//' | cut -d" " -f2- > $@

man/man3/%.3 : man/%.3.in
	$(AM_V_GEN)$(call man_tcti_prefix,$@,$^)

man/man7/%.7 : man/%.7.in
	$(AM_V_GEN)$(call man_tcti_prefix,$@,$^)

man/man8/%.8 : man/%.8.in
	$(AM_V_GEN)$(call man_tcti_prefix,$@,$^)

# this function is used to generate man pages from templates found in
# $(srcdir)/man/*.in
define man_tcti_prefix
	$(call make_parent_dir,$@)
	-$(AM_V_at)rm -f $@
	$(AM_V_GEN)sed -e "s,[@]VERSION[@],$(PACKAGE_VERSION),g;" $2 $(srcdir)/man/colophon.in >> $1
endef # man_tcti_prefix

%-generated.c %-generated.h : src/tabrmd.xml
	$(AM_V_GEN)$(call make_parent_dir,$@) && \
	$(GDBUS_CODEGEN) --interface-prefix=com.intel.tss2. \
	    --generate-c-code=src/tabrmd-generated $^

%.preset : %.preset.in
	$(AM_V_GEN)$(call make_parent_dir,$@) && \
	sed -e "s,[@]SYSTEMD_PRESET_DEFAULT[@],$(SYSTEMD_PRESET_DEFAULT),g;" $^ > $@

%.service : %.service.in
	$(AM_V_GEN)$(call make_parent_dir,$@) && \
	sed -e "s,[@]SBINDIR[@],$(sbindir),g; \
	        s,[@]sysconfdir[@],$(sysconfdir),g;" $^ > $@

%.pc : %.pc.in
	$(AM_V_GEN)$(call make_parent_dir,$@) && \
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
test_response_sink_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(TSS2_SYS_LIBS) \
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
test_command_attrs_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(TSS2_SYS_LIBS) $(GOBJECT_LIBS) $(libutil) $(libtss2_tcti_echo)
test_command_attrs_unit_LDFLAGS  = -Wl,--wrap=access_broker_lock_sapi,--wrap=access_broker_get_max_command,--wrap=Tss2_Sys_GetCapability
test_command_attrs_unit_SOURCES  = test/command-attrs_unit.c

test_command_source_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_command_source_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(TSS2_SYS_LIBS) $(PTHREAD_LIBS) $(GOBJECT_LIBS) $(libutil)
test_command_source_unit_LDFLAGS = -Wl,--wrap=g_source_set_callback,--wrap=connection_manager_lookup_istream,--wrap=connection_manager_remove,--wrap=sink_enqueue,--wrap=read_tpm_buffer_alloc,--wrap=command_attrs_from_cc
test_command_source_unit_SOURCES = test/command-source_unit.c

test_handle_map_entry_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_handle_map_entry_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_handle_map_entry_unit_SOURCES = test/handle-map-entry_unit.c

test_handle_map_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_handle_map_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(TSS2_SYS_LIBS) $(PTHREAD_LIBS) $(GOBJECT_LIBS) $(libutil)
test_handle_map_unit_SOURCES = test/handle-map_unit.c

test_ipc_frontend_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_ipc_frontend_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_ipc_frontend_unit_SOURCES = test/ipc-frontend_unit.c

test_ipc_frontend_dbus_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_ipc_frontend_dbus_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_ipc_frontend_dbus_unit_SOURCES = test/ipc-frontend-dbus_unit.c

test_logging_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_logging_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_logging_unit_LDFLAGS = -Wl,--wrap=getenv,--wrap=syslog
test_logging_unit_SOURCES = test/logging_unit.c

test_thread_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_thread_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(TSS2_SYS_LIBS) $(libutil)
test_thread_unit_SOURCES = test/thread_unit.c

test_tpm2_command_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tpm2_command_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(TSS2_SYS_LIBS) $(libutil)
test_tpm2_command_unit_SOURCES = test/tpm2-command_unit.c

test_tpm2_response_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tpm2_response_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(TSS2_SYS_LIBS) $(libutil)
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
test_access_broker_unit_LDADD = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(TSS2_SYS_LIBS) $(PTHREAD_LIBS) $(libutil) $(libtss2_tcti_echo)
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
test_resource_manager_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(TSS2_SYS_LIBS) $(PTHREAD_LIBS) $(libutil) $(libtss2_tcti_echo)
test_resource_manager_unit_SOURCES = test/resource-manager_unit.c

test_tcti_dynamic_unit_CFLAGS   = $(UNIT_AM_CFLAGS)
test_tcti_dynamic_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_tcti_dynamic_unit_LDFLAGS  = -Wl,--wrap=dlopen,--wrap=dlsym,--wrap=dlclose
test_tcti_dynamic_unit_SOURCES  = test/tcti-dynamic_unit.c

test_tcti_echo_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tcti_echo_unit_LDADD   = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil) $(libtss2_tcti_echo)
test_tcti_echo_unit_SOURCES = test/tcti-echo_unit.c

test_tcti_util_unit_CFLAGS   = $(UNIT_AM_CFLAGS)
test_tcti_util_unit_LDADD    = $(CMOCKA_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(libutil)
test_tcti_util_unit_LDFLAGS  = -Wl,--wrap=dlopen,--wrap=dlsym,--wrap=dlclose
test_tcti_util_unit_SOURCES  = test/tcti-util_unit.c

test_tss2_tcti_tabrmd_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tss2_tcti_tabrmd_unit_LDFLAGS = -Wl,--wrap=g_dbus_proxy_call_with_unix_fd_list_sync,--wrap=tcti_tabrmd_call_cancel_sync,--wrap=tcti_tabrmd_call_set_locality_sync,--wrap=read_data,--wrap=poll
test_tss2_tcti_tabrmd_unit_LDADD   = $(CMOCKA_LIBS) $(GIO_LIBS) $(libutil)
test_tss2_tcti_tabrmd_unit_SOURCES = src/tcti-tabrmd.c test/tss2-tcti-tabrmd_unit.c

test_tss2_tcti_echo_unit_CFLAGS  = $(UNIT_AM_CFLAGS)
test_tss2_tcti_echo_unit_LDADD   = $(CMOCKA_LIBS) $(libtss2_tcti_echo)
test_tss2_tcti_echo_unit_SOURCES = test/tss2-tcti-echo_unit.c
endif

TEST_INT_LIBS = $(libtest) $(libutil) $(libtss2_tcti_tabrmd) $(GLIB_LIBS)
test_integration_auth_session_max_int_LDADD = $(TEST_INT_LIBS)
test_integration_auth_session_max_int_SOURCES = test/integration/main.c \
    test/integration/auth-session-max.int.c

test_integration_auth_session_start_flush_int_LDADD = $(TEST_INT_LIBS)
test_integration_auth_session_start_flush_int_SOURCES = test/integration/main.c \
    test/integration/auth-session-start-flush.int.c

test_integration_auth_session_start_save_int_LDADD = $(TEST_INT_LIBS)
test_integration_auth_session_start_save_int_SOURCES = test/integration/main.c \
    test/integration/auth-session-start-save.int.c

test_integration_auth_session_start_save_load_int_LDADD = $(TEST_INT_LIBS)
test_integration_auth_session_start_save_load_int_SOURCES = test/integration/main.c \
    test/integration/auth-session-start-save-load.int.c

test_integration_max_transient_upperbound_int_LDADD = $(TEST_INT_LIBS)
test_integration_max_transient_upperbound_int_SOURCES = test/integration/main.c \
    test/integration/max-transient-upperbound.int.c

test_integration_get_capability_handles_transient_int_LDADD = $(TEST_INT_LIBS)
test_integration_get_capability_handles_transient_int_SOURCES = \
    test/integration/main.c test/integration/get-capability-handles-transient.int.c

test_integration_session_load_from_open_connection_int_LDADD = $(TEST_INT_LIBS)
test_integration_session_load_from_open_connection_int_SOURCES = \
    test/integration/session-load-from-open-connection.int.c

test_integration_session_load_from_closed_connection_int_LDADD = $(TEST_INT_LIBS)
test_integration_session_load_from_closed_connection_int_SOURCES = \
    test/integration/session-load-from-closed-connection.int.c

test_integration_session_load_from_closed_connections_lru_int_LDADD = $(TEST_INT_LIBS)
test_integration_session_load_from_closed_connections_lru_int_SOURCES = \
    test/integration/session-load-from-closed-connections-lru.int.c

test_integration_start_auth_session_int_LDADD = $(TEST_INT_LIBS)
test_integration_start_auth_session_int_SOURCES = test/integration/main.c test/integration/start-auth-session.int.c

test_integration_tcti_cancel_int_LDADD = $(TEST_INT_LIBS)
test_integration_tcti_cancel_int_SOURCES = test/integration/main.c test/integration/tcti-cancel.int.c

test_integration_tcti_double_finalize_int_LDADD = $(TEST_INT_LIBS)
test_integration_tcti_double_finalize_int_SOURCES = test/integration/main.c \
    test/integration/tcti-double-finalize.int.c

test_integration_tcti_connect_multiple_int_LDADD = $(TEST_INT_LIBS)
test_integration_tcti_connect_multiple_int_SOURCES = test/integration/tcti-connect-multiple.int.c

test_integration_tcti_connections_max_int_LDADD = $(TEST_INT_LIBS)
test_integration_tcti_connections_max_int_SOURCES = test/integration/tcti-connections-max.int.c

test_integration_tcti_set_locality_int_LDADD  = $(TEST_INT_LIBS)
test_integration_tcti_set_locality_int_SOURCES = test/integration/main.c test/integration/tcti-set-locality.int.c

test_integration_hash_sequence_int_LDADD = $(TEST_INT_LIBS)
test_integration_hash_sequence_int_SOURCES = test/integration/main.c test/integration/hash-sequence.int.c

test_integration_not_enough_handles_for_command_int_LDADD = $(TEST_INT_LIBS)
test_integration_not_enough_handles_for_command_int_SOURCES = test/integration/main.c test/integration/not-enough-handles-for-command.int.c

test_integration_password_authorization_int_LDADD = $(TEST_INT_LIBS)
test_integration_password_authorization_int_SOURCES = test/integration/main.c test/integration/password-authorization.int.c

test_integration_manage_transient_keys_int_LDADD = $(TEST_INT_LIBS)
test_integration_manage_transient_keys_int_SOURCES = test/integration/main.c test/integration/manage-transient-keys.int.c

test_integration_tpm2_command_flush_no_handle_int_LDADD = $(TEST_INT_LIBS)
test_integration_tpm2_command_flush_no_handle_int_SOURCES = \
    test/integration/main.c test/integration/tpm2-command-flush-no-handle.int.c

test_integration_util_buf_max_upper_bound_int_LDADD = $(TEST_INT_LIBS)
test_integration_util_buf_max_upper_bound_int_SOURCES = \
    test/integration/main.c test/integration/util-buf-max-upper-bound.int.c
