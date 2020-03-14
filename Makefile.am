# SPDX-License-Identifier: BSD-2-Clause
# we have BUILT_SOURCES so builddir must be in VPATH
VPATH = $(srcdir) $(builddir)
ACLOCAL_AMFLAGS = -I m4 --install

.PHONY: unit-count

unit-count: check
	sh scripts/unit-count.sh

CLEAN_LOCAL_DEPS =
clean-local: $(CLEAN_LOCAL_DEPS)

AM_CFLAGS = $(EXTRA_CFLAGS) \
    -I$(srcdir)/src -I$(srcdir)/src/include -I$(builddir)/src \
    $(GIO_CFLAGS) $(GLIB_CFLAGS) $(PTHREAD_CFLAGS) \
    $(TSS2_SYS_CFLAGS) $(TSS2_MU_CFLAGS) $(TSS2_TCTILDR_CFLAGS) \
    $(CODE_COVERAGE_CFLAGS) $(TSS2_RC_CFLAGS)
AM_LDFLAGS = $(EXTRA_LDFLAGS) $(CODE_COVERAGE_LIBS)

TESTS_UNIT = \
    test/tpm2_unit \
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
    test/session-list_unit \
    test/tabrmd-init_unit \
    test/tabrmd-options_unit \
    test/test-skeleton_unit \
    test/tcti_unit \
    test/thread_unit \
    test/tpm2-command_unit \
    test/tpm2-response_unit \
    test/tss2-tcti-tabrmd_unit \
    test/tcti-tabrmd-receive_unit \
    test/util_unit

TESTS_INTEGRATION = \
    test/integration/auth-session-max.int \
    test/integration/auth-session-start-flush.int \
    test/integration/auth-session-start-save.int \
    test/integration/auth-session-start-save-load.int \
    test/integration/max-transient-upperbound.int \
    test/integration/get-capability-handles-transient.int \
    test/integration/manage-transient-keys.int \
    test/integration/session-gap.int \
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

# empty init for these since they're manipulated by conditionals
TESTS =
noinst_LTLIBRARIES =
XFAIL_TESTS = \
    test/integration/start-auth-session.int
TEST_EXTENSIONS = .int
AM_TESTS_ENVIRONMENT = \
    TEST_FUNC_LIB=$(srcdir)/scripts/int-test-funcs.sh \
    PATH=./src:$(PATH) \
    dbus-run-session
INT_LOG_COMPILER = $(srcdir)/scripts/int-test-setup.sh
INT_LOG_FLAGS = --tabrmd-tcti=$(TABRMD_TCTI)

if ENABLE_INTEGRATION
noinst_LTLIBRARIES += $(libtest)
TESTS += $(TESTS_INTEGRATION)
if HWTPM
TABRMD_TCTI = device
else
TABRMD_TCTI = mssim
TESTS += $(TESTS_INTEGRATION_NOHW)
endif
endif

if UNIT
TESTS += $(TESTS_UNIT)
endif

sbin_PROGRAMS   = src/tpm2-abrmd
check_PROGRAMS  = $(sbin_PROGRAMS) $(TESTS)

# libraries
libtss2_tcti_tabrmd = src/libtss2-tcti-tabrmd.la
libtest        = test/integration/libtest.la
libutil        = src/libutil.la

lib_LTLIBRARIES = $(libtss2_tcti_tabrmd)
noinst_LTLIBRARIES += \
    $(libutil)
man_MANS = \
    man/man3/Tss2_Tcti_Tabrmd_Init.3 \
    man/man7/tss2-tcti-tabrmd.7 \
    man/man8/tpm2-abrmd.8

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
    dist/com.intel.tss2.Tabrmd.service \
    scripts/int-test-funcs.sh \
    scripts/int-test-setup.sh \
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
    $(man_MANS) \
    src/tabrmd-generated.c \
    src/tabrmd-generated.h \
    test/integration/*.log \
    _tabrmd.log
DISTCLEANFILES = \
    core \
    default.profraw \
    $(pkgconfig_DATA) \
    $(systemdpreset_DATA) \
    $(systemdsystemunit_DATA)

BUILT_SOURCES = src/tabrmd-generated.h src/tabrmd-generated.c

pkgconfig_DATA   = dist/tss2-tcti-tabrmd.pc
dbuspolicy_DATA  = dist/tpm2-abrmd.conf
if HAVE_SYSTEMD
systemdpreset_DATA = dist/tpm2-abrmd.preset
systemdsystemunit_DATA = dist/tpm2-abrmd.service
dbusservicedir   = $(datadir)/dbus-1/system-services
dbusservice_DATA = dist/com.intel.tss2.Tabrmd.service
endif # HAVE_SYSTEMD

# ax_code_coverage
if AUTOCONF_CODE_COVERAGE_2019_01_06
include $(top_srcdir)/aminclude_static.am
CLEAN_LOCAL_DEPS += code-coverage-clean
distclean-local: code-coverage-dist-clean

# workaround for a bug in Autoconf Archive 2019.01.06
# (https://github.com/autoconf-archive/autoconf-archive/pull/182)
if CODE_COVERAGE_ENABLED
AM_DISTCHECK_CONFIGURE_FLAGS := $(AM_DISTCHECK_CONFIGURE_FLAGS) --disable-code-coverage
endif # CODE_COVERAGE_ENABLED

else # AUTOCONF_CODE_COVERAGE_2019_01_06
@CODE_COVERAGE_RULES@
endif # AUTOCONF_CODE_COVERAGE_2019_01_06

@VALGRIND_CHECK_RULES@
VALGRIND_G_DEBUG = fatal-criticals,gc-friendly
VALGRIND_memcheck_FLAGS = --leak-check=full --show-leak-kinds=definite,indirect --track-origins=yes --error-exitcode=1

# This is a hack required to ensure that BUILT_SOURCES are generated before
# the file that requires them.
src/ipc-frontend-dbus.c : $(BUILT_SOURCES)

# utility library with most of the code that makes up the daemon
# -Wno-unused-parameter for automatically-generated tabrmd-generated.c:
src_libutil_la_CFLAGS  = $(AM_CFLAGS) -Wno-unused-parameter
src_libutil_la_LIBADD  = $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) \
    $(TSS2_SYS_LIBS) $(TSS2_MU_LIBS) $(TSS2_TCTILDR_LIBS) $(TSS2_RC_LIBS)
src_libutil_la_SOURCES = \
    src/tpm2.c \
    src/tpm2.h \
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
    src/resource-manager-session.c \
    src/resource-manager-session.h \
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
    src/tabrmd-defaults.h \
    src/tabrmd-error.c \
    src/tabrmd-generated.c \
    src/tabrmd-generated.h \
    src/tabrmd-init.c \
    src/tabrmd-init.h \
    src/tabrmd-options.c \
    src/tabrmd-options.h \
    src/tabrmd.h \
    src/tcti.c \
    src/tcti.h \
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

test_integration_libtest_la_LIBADD  = $(TSS2_SYS_LIBS) $(TSS2_TCTILDR_LIBS) \
    $(GLIB_LIBS)
test_integration_libtest_la_SOURCES = \
    test/integration/common.c \
    test/integration/common.h \
    test/integration/context-util.c \
    test/integration/context-util.h \
    test/integration/test-options.c \
    test/integration/test-options.h \
    test/mock-funcs.c \
    test/mock-funcs.h \
    test/mock-io-stream.c \
    test/mock-io-stream.h \
    test/tcti-mock.c \
    test/tcti-mock.h

src_libtss2_tcti_tabrmd_la_LIBADD = $(GIO_LIBS) $(GLIB_LIBS) $(TSS2_MU_LIBS) \
    $(libutil) $(TSS2_TCTILDR_LIBS)
src_libtss2_tcti_tabrmd_la_LDFLAGS = -fPIC -Wl,--no-undefined -Wl,-z,nodelete -Wl,--version-script=$(srcdir)/src/tcti-tabrmd.map
src_libtss2_tcti_tabrmd_la_SOURCES = src/tcti-tabrmd.c src/tcti-tabrmd-priv.h $(srcdir)/src/tcti-tabrmd.map

src_tpm2_abrmd_LDADD   = $(GIO_LIBS) $(GLIB_LIBS) $(PTHREAD_LIBS) \
    $(TSS2_SYS_LIBS) $(TSS2_TCTILDR_LIBS) $(libutil)
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

define make_parent_dir
    if [ ! -d $(dir $1) ]; then mkdir -p $(dir $1); fi
endef

if UNIT
UNIT_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS)
UNIT_LIBS = $(CMOCKA_LIBS) $(libutil) $(libtest)
test_tabrmd_init_unit_CFLAGS = $(UNIT_CFLAGS)
test_tabrmd_init_unit_LDADD = $(UNIT_LIBS)
test_tabrmd_init_unit_LDFLAGS = -Wl,--wrap=g_main_loop_is_running \
    -Wl,--wrap=g_main_loop_quit,--wrap=g_unix_signal_add \
    -Wl,--wrap=random_seed_from_file,--wrap=random_get_bytes \
    -Wl,--wrap=ipc_frontend_connect,--wrap=Tss2_TctiLdr_Initialize \
    -Wl,--wrap=tpm2_init_tpm,--wrap=command_attrs_init_tpm \
    -Wl,--wrap=thread_start,--wrap=tpm2_flush_all_context
test_tabrmd_init_unit_SOURCES = test/tabrmd-init_unit.c

test_tabrmd_options_unit_CFLAGS = $(UNIT_CFLAGS)
test_tabrmd_options_unit_LDADD = $(UNIT_LIBS)
test_tabrmd_options_unit_LDFLAGS = -Wl,--wrap=g_option_context_new \
    -Wl,--wrap=g_option_context_add_main_entries,--wrap=g_option_context_parse \
    -Wl,--wrap=set_logger,--wrap=g_option_context_free
test_tabrmd_options_unit_SOURCES = test/tabrmd-options_unit.c

test_test_skeleton_unit_CFLAGS = $(UNIT_CFLAGS)
test_test_skeleton_unit_LDADD = $(UNIT_LIBS)
test_test_skeleton_unit_SOURCES = test/test-skeleton_unit.c

test_response_sink_unit_CFLAGS = $(UNIT_CFLAGS)
test_response_sink_unit_LDADD = $(UNIT_LIBS)
test_response_sink_unit_SOURCES = test/response-sink_unit.c

test_connection_unit_CFLAGS = $(UNIT_CFLAGS)
test_connection_unit_LDADD = $(UNIT_LIBS)
test_connection_unit_SOURCES = test/connection_unit.c

test_connection_manager_unit_CFLAGS = $(UNIT_CFLAGS)
test_connection_manager_unit_LDADD = $(UNIT_LIBS)
test_connection_manager_unit_SOURCES = test/connection-manager_unit.c

test_command_attrs_unit_CFLAGS = $(UNIT_CFLAGS)
test_command_attrs_unit_LDADD = $(UNIT_LIBS)
test_command_attrs_unit_LDFLAGS  = -Wl,--wrap=tpm2_get_command_attrs
test_command_attrs_unit_SOURCES  = test/command-attrs_unit.c

test_command_source_unit_CFLAGS = $(UNIT_CFLAGS)
test_command_source_unit_LDADD = $(UNIT_LIBS)
test_command_source_unit_LDFLAGS = -Wl,--wrap=g_source_set_callback,--wrap=connection_manager_lookup_istream,--wrap=connection_manager_remove,--wrap=sink_enqueue,--wrap=read_tpm_buffer_alloc,--wrap=command_attrs_from_cc
test_command_source_unit_SOURCES = test/command-source_unit.c

test_handle_map_entry_unit_CFLAGS = $(UNIT_CFLAGS)
test_handle_map_entry_unit_LDADD = $(UNIT_LIBS)
test_handle_map_entry_unit_SOURCES = test/handle-map-entry_unit.c

test_handle_map_unit_CFLAGS = $(UNIT_CFLAGS)
test_handle_map_unit_LDADD = $(UNIT_LIBS)
test_handle_map_unit_SOURCES = test/handle-map_unit.c

test_ipc_frontend_unit_CFLAGS = $(UNIT_CFLAGS)
test_ipc_frontend_unit_LDADD = $(UNIT_LIBS)
test_ipc_frontend_unit_SOURCES = test/ipc-frontend_unit.c

test_ipc_frontend_dbus_unit_CFLAGS = $(UNIT_CFLAGS)
test_ipc_frontend_dbus_unit_LDADD = $(UNIT_LIBS)
test_ipc_frontend_dbus_unit_SOURCES = test/ipc-frontend-dbus_unit.c

test_logging_unit_CFLAGS = $(UNIT_CFLAGS)
test_logging_unit_LDADD = $(UNIT_LIBS)
test_logging_unit_LDFLAGS = -Wl,--wrap=getenv,--wrap=syslog
test_logging_unit_SOURCES = test/logging_unit.c

test_thread_unit_CFLAGS = $(UNIT_CFLAGS)
test_thread_unit_LDADD = $(UNIT_LIBS)
test_thread_unit_SOURCES = test/thread_unit.c

test_tpm2_command_unit_CFLAGS = $(UNIT_CFLAGS)
test_tpm2_command_unit_LDADD = $(UNIT_LIBS)
test_tpm2_command_unit_SOURCES = test/tpm2-command_unit.c

test_tpm2_response_unit_CFLAGS = $(UNIT_CFLAGS)
test_tpm2_response_unit_LDADD = $(UNIT_LIBS)
test_tpm2_response_unit_SOURCES = test/tpm2-response_unit.c

test_util_unit_CFLAGS = $(UNIT_CFLAGS)
test_util_unit_LDADD = $(UNIT_LIBS)
test_util_unit_LDFLAGS = -Wl,--wrap=g_input_stream_read,--wrap=g_output_stream_write
test_util_unit_SOURCES = test/util_unit.c

test_message_queue_unit_CFLAGS = $(UNIT_CFLAGS)
test_message_queue_unit_LDADD = $(UNIT_LIBS)
test_message_queue_unit_SOURCES = test/message-queue_unit.c

test_tpm2_unit_CFLAGS = $(UNIT_CFLAGS)
test_tpm2_unit_LDADD = $(UNIT_LIBS)
test_tpm2_unit_LDFLAGS = -Wl,--wrap=Tss2_Sys_FlushContext \
    -Wl,--wrap=Tss2_Sys_ContextLoad,--wrap=Tss2_Sys_ContextSave \
    -Wl,--wrap=Tss2_Sys_GetCapability,--wrap=Tss2_Sys_Initialize \
    -Wl,--wrap=Tss2_Sys_Startup
test_tpm2_unit_SOURCES = test/tpm2_unit.c

test_random_unit_CFLAGS = $(UNIT_CFLAGS)
test_random_unit_LDADD = $(UNIT_LIBS)
test_random_unit_LDFLAGS = -Wl,--wrap=open,--wrap=read,--wrap=close
test_random_unit_SOURCES = test/random_unit.c

test_session_entry_unit_CFLAGS = $(UNIT_CFLAGS)
test_session_entry_unit_LDADD = $(UNIT_LIBS)
test_session_entry_unit_SOURCES = test/session-entry_unit.c

test_session_list_unit_CFLAGS = $(UNIT_CFLAGS)
test_session_list_unit_LDADD = $(UNIT_LIBS)
test_session_list_unit_SOURCES = test/session-list_unit.c

test_resource_manager_unit_CFLAGS = $(UNIT_CFLAGS)
test_resource_manager_unit_LDADD = $(UNIT_LIBS)
test_resource_manager_unit_LDFLAGS = -Wl,--wrap=tpm2_send_command,--wrap=sink_enqueue,--wrap=tpm2_context_saveflush,--wrap=tpm2_context_load
test_resource_manager_unit_SOURCES = test/resource-manager_unit.c

test_tcti_unit_CFLAGS = $(UNIT_CFLAGS)
test_tcti_unit_LDADD = $(UNIT_LIBS)
test_tcti_unit_SOURCES  = test/tcti_unit.c

test_tss2_tcti_tabrmd_unit_CFLAGS = $(UNIT_CFLAGS)
test_tss2_tcti_tabrmd_unit_LDADD = $(UNIT_LIBS)
test_tss2_tcti_tabrmd_unit_LDFLAGS = -Wl,--wrap=g_dbus_proxy_call_with_unix_fd_list_sync,--wrap=tcti_tabrmd_call_cancel_sync,--wrap=tcti_tabrmd_call_set_locality_sync,--wrap=tcti_tabrmd_proxy_new_for_bus_sync
test_tss2_tcti_tabrmd_unit_SOURCES = src/tcti-tabrmd.c test/tss2-tcti-tabrmd_unit.c

test_tcti_tabrmd_receive_unit_CFLAGS = $(UNIT_CFLAGS) -DG_DISABLE_CAST_CHECKS
test_tcti_tabrmd_receive_unit_LDADD = $(UNIT_LIBS)
test_tcti_tabrmd_receive_unit_LDFLAGS = -Wl,--wrap=poll,--wrap=g_socket_connection_get_socket,--wrap=g_socket_get_fd,--wrap=g_input_stream_read,--wrap=g_io_stream_get_input_stream,--wrap=g_input_stream_read
test_tcti_tabrmd_receive_unit_SOURCES = src/tcti-tabrmd.c test/tcti-tabrmd-receive_unit.c
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

test_integration_session_gap_int_LDADD = $(TEST_INT_LIBS)
test_integration_session_gap_int_SOURCES = test/integration/main.c \
    test/integration/session-gap.int.c

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

if WITH_SEPOLICY

refpoldir = $(datadir)/selinux/packages
refpol_DATA = selinux/tabrmd.pp.bz2

refpolifdir = $(datadir)/selinux/devel/include/contrib
refpolif_DATA = selinux/tabrmd.if

CLEANFILES += \
    $(refpol_DATA) \
    selinux/all_interfaces.conf \
    selinux/iferror.m4 \
    selinux/tabrmd.mod \
    selinux/tabrmd.mod.fc \
    selinux/tabrmd.pp \
    selinux/tabrmd.tmp

selinux/all_interfaces.conf: $(SEPOL_SUPPORT) $(SEPOL_INTERFACES) $(refpolif_DATA)
	@test -d $(@D) || mkdir -p $(@D)
	@echo "ifdef(\`__if_error',\`m4exit(1)')" > selinux/iferror.m4
	@echo "divert(-1)" > $@
	$(M4) $^ selinux/iferror.m4 | $(SED) -e s/dollarsstar/\$$\*/g >> $@
	@echo "divert" >> $@

%.mod: $(SEPOL_SUPPORT) selinux/all_interfaces.conf %.te
	@test -d $(@D) || mkdir -p $(@D)
	$(M4) $(SEPOL_M4PARAM) -s $^ > $(@:.mod=.tmp)
	$(CHECKMODULE) -M -m $(@:.mod=.tmp) -o $@

%.mod.fc: $(SEPOL_SUPPORT) %.fc
	@test -d $(@D) || mkdir -p $(@D)
	$(M4) $(SEPOL_M4PARAM) $^ > $@

%.pp: %.mod %.mod.fc
	$(SEMODULE_PACKAGE) -o $@ -m $< -f $<.fc

%.pp.bz2: %.pp
	$(BZIP2) -k -f -9 $^ && touch $@

endif
