/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <gio/gunixfdlist.h>
#include <glib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <setjmp.h>
#include <cmocka.h>

#include <tss2/tss2_tpm2_types.h>

#include "tabrmd.h"
#include "tss2-tcti-tabrmd.h"
#include "tcti-tabrmd-priv.h"
#include "tpm2-header.h"
#include "util.h"

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define TSS2_TCTI_POLL_HANDLE_ZERO_INIT { .fd = 0, .events = 0, .revents = 0 }
#else
#define TSS2_TCTI_POLL_HANDLE_ZERO_INIT { 0 }
#endif

/*
 * when given an NULL context and a pointer to a size_t, set the size_t
 * parameter to the size of the TSS2_TCTI_TABRMD_CONTEXT structure.
 */
static void
tcti_tabrmd_init_size_test (void **state)
{
    size_t tcti_size;
    UNUSED_PARAM(state);

    Tss2_Tcti_Tabrmd_Init (NULL, &tcti_size, NULL);
    assert_int_equal (tcti_size, sizeof (TSS2_TCTI_TABRMD_CONTEXT));
}
/*
 * when given a NULL context and a pointer to size_t, the init function
 * returns TSS2_RC_SUCCESS
 */
static void
tcti_tabrmd_init_success_return_value_test (void **state)
{
    size_t tcti_size;
    TSS2_RC ret;
    UNUSED_PARAM(state);

    ret = Tss2_Tcti_Tabrmd_Init (NULL, &tcti_size, NULL);
    assert_int_equal (ret, TSS2_RC_SUCCESS);
}
/*
 * when given NULL pointers for both parameters the init function returns
 * an error indicating that the values are bad (TSS2_TCTI_RC_BAD_VALUE)
 */
static void
tcti_tabrmd_init_allnull_is_bad_value_test (void **state)
{
    TSS2_RC ret;
    UNUSED_PARAM(state);

    ret = Tss2_Tcti_Tabrmd_Init (NULL, NULL, NULL);
    assert_int_equal (ret, TSS2_TCTI_RC_BAD_VALUE);
}
/*
 * Ensure that the standard Tss2_Tcti_Info discovery function returns a
 * reference to the config structure.
 */
static void
tcti_tabrmd_info_test (void **state)
{
    const TSS2_TCTI_INFO *tcti_info;
    UNUSED_PARAM(state);

    /*
     * This function isn't meant to be called directly. We do it here for the
     * test coverage
     */
    tcti_info = Tss2_Tcti_Info ();
    assert_non_null (tcti_info);
    assert_non_null (tcti_info->init);
}
/*
 * Ensure that when we pass the string "session" to the
 * tabrmd_bus_type_from_str returns G_BUS_TYPE_SESSION.
 */
static void
tcti_tabrmd_bus_type_from_str_session_test (void **state)
{
    UNUSED_PARAM(state);
    assert_int_equal (tabrmd_bus_type_from_str ("session"),
                      G_BUS_TYPE_SESSION);
}
/*
 * Ensure that when we pass the string "system" to the
 * tabrmd_bus_type_from_str returns G_BUS_TYPE_SYSTEM.
 */
static void
tcti_tabrmd_bus_type_from_str_system_test (void **state)
{
    UNUSED_PARAM(state);
    assert_int_equal (tabrmd_bus_type_from_str ("system"),
                      G_BUS_TYPE_SYSTEM);
}
/*
 * Ensure that when we pass an unexpected string to the
 * tabrmd_bus_type_from_str returns G_BUS_TYPE_NONE.
 */
static void
tcti_tabrmd_bus_type_from_str_bad_test (void **state)
{
    UNUSED_PARAM(state);
    assert_int_equal (tabrmd_bus_type_from_str ("foobar"),
                      G_BUS_TYPE_NONE);
}
/*
 * Ensure that when we pass the key "bus_name" and value "any string"
 * to tabrmd_kv_callback that it returns an RC indicating success while
 * the conf structure 'bus_name' field is set to the value string.
 */
static void
tcti_tabrmd_kv_callback_name_test (void **state)
{
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    key_value_t key_value = { 
        .key = "bus_name",
        .value = "foo.bar",
    };
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = tabrmd_kv_callback (&key_value, &conf);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_string_equal ("foo.bar", conf.bus_name);
}
/*
 * Ensure that when we pass the key "bus_type" and the value "system"
 * to tabrmd_kv_callback that it returns an RC indicating success while
 * the conf structure 'bus_type' field is set to G_BUS_TYPE_SYSTEM.
 */
static void
tcti_tabrmd_kv_callback_type_good_test (void **state)
{
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    key_value_t key_value = { 
        .key = "bus_type",
        .value = "system",
    };
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = tabrmd_kv_callback (&key_value, &conf);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_int_equal (conf.bus_type, G_BUS_TYPE_SYSTEM);
}
/*
 * Ensure that when we pass the key "bus_type" and with an invalid value
 * string (not "system" or "session") that it returns the BAD_VALUE RC and
 * sets the 'bus_type' field of the conf structure to
 * G_BUS_TYPE_NONE.
 */
static void
tcti_tabrmd_kv_callback_type_bad_test (void **state)
{
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    key_value_t key_value = { 
        .key = "bus_type",
        .value = "foo",
    };
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = tabrmd_kv_callback (&key_value, &conf);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
    assert_int_equal (conf.bus_type, G_BUS_TYPE_NONE);
}
/*
 * Ensure that when we pass an invalid key (not 'bus_type' or 'bus_name')
 * that it returns an RC indicating BAD_VALUE.
 */
static void
tcti_tabrmd_kv_callback_bad_key_test (void **state)
{
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    key_value_t key_value = { 
        .key = "foo",
        .value = "bar",
    };
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = tabrmd_kv_callback (&key_value, &conf);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/*
 * Ensure that a common config string selecting the session bus with
 * a user supplied name is parsed correctly.
 */
static void
tcti_tabrmd_conf_parse_named_session_test (void **state)
{
    TSS2_RC rc;
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    char conf_str[] = "bus_type=session,bus_name=com.example.Session";
    UNUSED_PARAM(state);

    rc = parse_key_value_string (conf_str, tabrmd_kv_callback, &conf);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_string_equal (conf.bus_name, "com.example.Session");
    assert_int_equal (conf.bus_type, G_BUS_TYPE_SESSION);
}
/*
 * Ensure that a common config string selecting the system bus with
 * a user supplied name is parsed correctly.
 */
static void
tcti_tabrmd_conf_parse_named_system_test (void **state)
{
    TSS2_RC rc;
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    char conf_str[] = "bus_type=system,bus_name=com.example.System";
    UNUSED_PARAM(state);

    rc = parse_key_value_string (conf_str, tabrmd_kv_callback, &conf);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_string_equal (conf.bus_name, "com.example.System");
    assert_int_equal (conf.bus_type, G_BUS_TYPE_SYSTEM);
}
/*
 * Ensure that an unknown bus_type string results in the appropriate RC.
 */
static void
tcti_tabrmd_conf_parse_bad_type_test (void **state)
{
    TSS2_RC rc;
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    char conf_str[] = "bus_type=foobar";
    UNUSED_PARAM(state);

    rc = parse_key_value_string (conf_str, tabrmd_kv_callback, &conf);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/*
 * Ensure that a config string that omits the bus_name results in a conf
 * structure with the associated field set to the default.
 */
static void
tcti_tabrmd_conf_parse_no_name_test (void **state)
{
    TSS2_RC rc;
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    char conf_str[] = "bus_type=session";
    UNUSED_PARAM(state);

    rc = parse_key_value_string (conf_str, tabrmd_kv_callback, &conf);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_string_equal (conf.bus_name, TABRMD_DBUS_NAME_DEFAULT);
    assert_int_equal (conf.bus_type, G_BUS_TYPE_SESSION);
}
/*
 * Ensure that a config string that omits the bus_type results in a conf
 * structure with the associated field set to the default.
 */
static void
tcti_tabrmd_conf_parse_no_type_test (void **state)
{
    TSS2_RC rc;
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    char conf_str[] = "bus_name=com.example.FooBar";
    UNUSED_PARAM(state);

    rc = parse_key_value_string (conf_str, tabrmd_kv_callback, &conf);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_string_equal (conf.bus_name, "com.example.FooBar");
    assert_int_equal (conf.bus_type, TABRMD_DBUS_TYPE_DEFAULT);
}
/*
 * Ensure that a missing value results in the appropriate RC.
 */
static void
tcti_tabrmd_conf_parse_no_value_test (void **state)
{
    TSS2_RC rc;
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    char conf_str[] = "bus_name=";
    UNUSED_PARAM(state);

    rc = parse_key_value_string (conf_str, tabrmd_kv_callback, &conf);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/*
 * Ensure that a missing key results in the appropriate RC
 */
static void
tcti_tabrmd_conf_parse_no_key_test (void **state)
{
    TSS2_RC rc;
    tabrmd_conf_t conf = TABRMD_CONF_INIT_DEFAULT;
    char conf_str[] = "=some-string";
    UNUSED_PARAM(state);

    rc = parse_key_value_string (conf_str, tabrmd_kv_callback, &conf);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/*
 * This mock function is used to insert error conditions in the TCTI
 * initialization function. The function mocked is generated by
 * gdbus-codegen from the dbus xml. Much of the functionality exposed by
 * that code uses a 'proxy' object used to interact with the remote
 * object (tabrmd in this case). The function mocked here is the one used
 * to connect to the tabrmd.
 *
 * The only tricky part to this function is the handling of the GError
 * object. If the function being mocked failes it returns NULL *and*
 * populates the error parameter passed by the caller with a GError object
 * describing the failure. The first item that *must* be on the mock stack
 * is the TctiTabrmd* being returned. If this is NULL then there MUST be
 * a second object on the mock stack and this MUST be a GError object. The
 * caller will g_clear_error this object for us.
 */
TctiTabrmd *
__wrap_tcti_tabrmd_proxy_new_for_bus_sync (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error)
{
    UNUSED_PARAM (bus_type);
    UNUSED_PARAM (flags);
    UNUSED_PARAM (name);
    UNUSED_PARAM (object_path);
    UNUSED_PARAM (cancellable);

    TctiTabrmd* proxy = mock_type (TctiTabrmd*);
    if (proxy == NULL && error != NULL) {
        *error = mock_type (GError*);
    }
    return proxy;
}

/*
 * Force failure acquiring proxy object.
 */
static void
tcti_tabrmd_proxy_new_fail (void **state)
{
    UNUSED_PARAM (state);
    size_t size = sizeof (TSS2_TCTI_TABRMD_CONTEXT);
    uint8_t buf [sizeof (TSS2_TCTI_TABRMD_CONTEXT)] = { 0 };
    TSS2_RC rc = TSS2_RC_SUCCESS;

    will_return (__wrap_tcti_tabrmd_proxy_new_for_bus_sync, NULL);
    will_return (__wrap_tcti_tabrmd_proxy_new_for_bus_sync,
                 g_error_new_literal (1, 1, __func__));
    rc = Tss2_Tcti_Tabrmd_Init ((TSS2_TCTI_CONTEXT*)buf, &size, NULL);
    assert_int_equal (rc, TSS2_TCTI_RC_NO_CONNECTION);
}
/*
 * This is a mock function to control return values from the connection
 * creation logic that invokes the DBus "CreateConnection" function exposed
 * by the tabrmd. This is called by the tcti as part of initializing the
 * TCTI context.
 */
GVariant *
__wrap_g_dbus_proxy_call_with_unix_fd_list_sync (
    GDBusProxy *proxy,
    const gchar *method_name,
    GVariant *parameters,
    GDBusCallFlags flags,
    gint timeout_msec,
    GUnixFDList *fd_list,
    GUnixFDList **out_fd_list,
    GCancellable *cancellable,
    GError **error)
{
    GVariant *variant_array[2] = { 0 }, *variant_tuple;
    gint client_fd;
    guint64 id;
    UNUSED_PARAM(proxy);
    UNUSED_PARAM(method_name);
    UNUSED_PARAM(parameters);
    UNUSED_PARAM(flags);
    UNUSED_PARAM(timeout_msec);
    UNUSED_PARAM(fd_list);
    UNUSED_PARAM(cancellable);
    UNUSED_PARAM(error);

    client_fd = mock_type (gint);
    id = mock_type (guint64);

    *out_fd_list = g_unix_fd_list_new_from_array (&client_fd, 1);
    variant_array[0] = g_variant_new_fixed_array (G_VARIANT_TYPE ("h"),
                                                  &client_fd,
                                                  1,
                                                  sizeof (gint32));
    variant_array[1] = g_variant_new_uint64 (id);
    variant_tuple = g_variant_new_tuple (variant_array, 2);

    return variant_tuple;
}
/*
 * This is a mock function to control behavior of the
 * tcti_tabrmd_call_cancel_sync function. This is a function generated from
 * the DBus xml specification. It returns only a TSS2_RC.
 */
gboolean
__wrap_tcti_tabrmd_call_cancel_sync (
    TctiTabrmd *proxy,
    guint64 arg_id,
    guint *out_return_code,
    GCancellable *cancellable,
    GError **error)
{
    UNUSED_PARAM(proxy);
    UNUSED_PARAM(arg_id);
    UNUSED_PARAM(cancellable);
    UNUSED_PARAM(error);

    *out_return_code = mock_type (guint);
    return mock_type (gboolean);
}
/*
 * This is a mock function to control behavior of the
 * tcti_tabrmd_call_set_locality_sync function. This is a function generated from
 * the DBus xml specification. It returns only a TSS2_RC.
 */
gboolean
__wrap_tcti_tabrmd_call_set_locality_sync (
    TctiTabrmd *proxy,
    guint64 arg_id,
    guchar arg_locality,
    guint *out_return_code,
    GCancellable *cancellable,
    GError **error)
{
    UNUSED_PARAM(proxy);
    UNUSED_PARAM(arg_id);
    UNUSED_PARAM(arg_locality);
    UNUSED_PARAM(cancellable);
    UNUSED_PARAM(error);

    *out_return_code = mock_type (guint);
    return mock_type (gboolean);
}
/*
 * Structure to hold data relevant to tabrmd TCTI unit tests.
 */
typedef struct {
    guint64 id;
    gint    client_fd;
    gint    server_fd;
    TSS2_TCTI_CONTEXT  *context;
    TctiTabrmdProxy *proxy;
} data_t;
/*
 * This is the setup function used to create the TCTI context structure for
 * tests that require the initialization be done before the test can be
 * executed. It *must* be paired with a call to the teardown function.
 */
static int
tcti_tabrmd_setup (void **state)
{
    data_t *data;
    TSS2_RC ret = TSS2_RC_SUCCESS;
    size_t tcti_size = 0;
    gint fds [2];
    guint64 id = 666;

    data = calloc (1, sizeof (data_t));
    ret = Tss2_Tcti_Tabrmd_Init (NULL, &tcti_size, NULL);
    if (ret != TSS2_RC_SUCCESS) {
        free (data);
        printf ("tss2_tcti_tabrmd_init failed: %d\n", ret);
        return 1;
    }
    data->context = calloc (1, tcti_size);
    if (data->context == NULL) {
        free (data);
        perror ("calloc");
        return 1;
    }
    data->proxy = calloc (1, sizeof (TctiTabrmdProxy));
    will_return (__wrap_tcti_tabrmd_proxy_new_for_bus_sync, data->proxy);
    g_debug ("preparing g_dbus_proxy_call_with_unix_fd_list_sync mock wrapper");
    assert_int_equal (socketpair (PF_LOCAL, SOCK_STREAM, 0, fds), 0);
    data->client_fd = fds [0];
    data->server_fd = fds [1];
    will_return (__wrap_g_dbus_proxy_call_with_unix_fd_list_sync,
                 data->client_fd);
    data->id = id;
    will_return (__wrap_g_dbus_proxy_call_with_unix_fd_list_sync, id);
    ret = Tss2_Tcti_Tabrmd_Init (data->context, &tcti_size, "bus_type=session");
    assert_int_equal (ret, TSS2_RC_SUCCESS);

    *state = data;
    return 0;
}
/*
 * This is a teardown function to deallocate / cleanup all resources
 * associated with these tests.
 */
static int
tcti_tabrmd_teardown (void **state)
{
    data_t *data = *state;

    tss2_tcti_tabrmd_finalize (data->context);
    close (data->client_fd);
    close (data->server_fd);
    if (data->context)
        free (data->context);
    if (data->proxy)
        free (data->proxy);
    free (data);
    return 0;
}
/*
 * Ensure that after initialization the 'magic' value in the TCTI structure
 * is the one that we expect.
 */
static void
tcti_tabrmd_magic_test (void **state)
{
    data_t *data = *state;

    assert_int_equal (TSS2_TCTI_MAGIC (data->context),
                      TSS2_TCTI_TABRMD_MAGIC);
}
/*
 * Ensure that after initialization the 'version' value in the TCTI structure
 * is the one that we expect.
 */
static void
tcti_tabrmd_version_test (void **state)
{
    data_t *data = *state;

    assert_int_equal (TSS2_TCTI_VERSION (data->context), TSS2_TCTI_TABRMD_VERSION);
}
/*
 * Ensure that after initialization the 'id' value set for the connection is
 * the one taht we expect.
 */
static void
tcti_tabrmd_init_success_test (void **state)
{
    data_t *data = *state;

    assert_int_equal (data->id, TSS2_TCTI_TABRMD_ID (data->context));
}
/*
 * These are a series of tests to ensure that the exposed TCTI functions
 * return the appropriate RC when passed NULL contexts.
 *
 * NOTE: These RCs come from the tss2_tcti.h header and not the tabrmd TCTI
 *       code. Still we test for it here as it's a useful reminder that the
 *       case is covered. Also there's a bug in the TSS2 currently as these
 *       should return BAD_REFERENCE, not BAD_CONTEXT
 */
static void
tcti_tabrmd_transmit_null_context_test (void **state)
{
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = tss2_tcti_tabrmd_transmit (NULL, 5, NULL);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_REFERENCE);
}
static void
tcti_tabrmd_cancel_null_context_test (void **state)
{
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = tss2_tcti_tabrmd_cancel (NULL);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_CONTEXT);
}
static void
tcti_tabrmd_get_poll_handles_null_context_test (void **state)
{
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = tss2_tcti_tabrmd_get_poll_handles (NULL, NULL, NULL);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_CONTEXT);
}
static void
tcti_tabrmd_set_locality_null_context_test (void **state)
{
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = tss2_tcti_tabrmd_set_locality (NULL, 5);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_CONTEXT);
}
/*
 * This test makes a single call to the transmit function in the the
 * TSS2_TCTI_CONTEXT function pointer table. It sends a fixed 12 byte TPM
 * command buffer:
 * tag:     TPM2_ST_NO_SESSIONS
 * size:    12 bytes
 * command: 0x0
 * data:    0x01, 0x02
 */
static void
tcti_tabrmd_transmit_success_test (void **state)
{
    data_t *data = *state;
    uint8_t command_in [] = { 0x80, 0x02,
                              0x00, 0x00, 0x00, 0x0c,
                              0x00, 0x00, 0x00, 0x00,
                              0x01, 0x02};
    size_t size = sizeof (command_in);
    uint8_t command_out [sizeof (command_in)] = { 0 };
    TSS2_RC rc;
    ssize_t ret;

    rc = tss2_tcti_tabrmd_transmit (data->context, size, command_in);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    ret = read (data->server_fd, command_out, size);
    assert_int_equal (ret, size);
    ASSERT_NON_NULL(data);
    ASSERT_NON_NULL(data->context);
    assert_int_equal (TSS2_TCTI_TABRMD_STATE (data->context),
                      TABRMD_STATE_RECEIVE);
    assert_memory_equal (command_in, command_out, size);
}
/*
 * This test ensures that the magic value in the context structure is checked
 * before the transmit function executes and that the RC is what we expect.
 */
static void
tcti_tabrmd_transmit_bad_magic_test (void **state)
{
    data_t *data = *state;
    TSS2_RC rc;
    uint8_t command [12] = { 0 };
    size_t size = 12;

    TSS2_TCTI_MAGIC (data->context) = 1;
    rc = tss2_tcti_tabrmd_transmit (data->context, size, command);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_CONTEXT);
    assert_int_equal (TSS2_TCTI_TABRMD_STATE (data->context),
                      TABRMD_STATE_TRANSMIT);
}
/*
 * This test ensures that the version in the context structure is checked
 * before the transmit function executes and that the RC is what we expect.
 */
static void
tcti_tabrmd_transmit_bad_version_test (void **state)
{
    data_t *data = *state;
    TSS2_RC rc;
    uint8_t command [12] = { 0 };
    size_t size = 12;

    TSS2_TCTI_VERSION (data->context) = -1;
    rc = tss2_tcti_tabrmd_transmit (data->context, size, command);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_CONTEXT);
    assert_int_equal (TSS2_TCTI_TABRMD_STATE (data->context),
                      TABRMD_STATE_TRANSMIT);
}
/*
 * This test sets the state to RECEIVE and then calls transmit. This should
 * produce a BAD_SEQUENCE error.
 */
static void
tcti_tabrmd_transmit_bad_sequence_receive_test (void **state)
{
    data_t *data = *state;
    TSS2_RC rc;
    uint8_t command [12] = { 0 };
    size_t size = 12;

    TSS2_TCTI_TABRMD_STATE (data->context) = TABRMD_STATE_RECEIVE;
    rc = tss2_tcti_tabrmd_transmit (data->context, size, command);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_SEQUENCE);
    assert_int_equal (TSS2_TCTI_TABRMD_STATE (data->context),
                      TABRMD_STATE_RECEIVE);
}
/*
 * This test sets the state to FINAL and then calls transmit. This should
 * produce a BAD_SEQUENCE error.
 */
static void
tcti_tabrmd_transmit_bad_sequence_final_test (void **state)
{
    data_t *data = *state;
    TSS2_RC rc;
    uint8_t command [12] = { 0 };
    size_t size = 12;

    TSS2_TCTI_TABRMD_STATE (data->context) = TABRMD_STATE_FINAL;
    rc = tss2_tcti_tabrmd_transmit (data->context, size, command);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_SEQUENCE);
    assert_int_equal (TSS2_TCTI_TABRMD_STATE (data->context),
                      TABRMD_STATE_FINAL);
}
/*
 * This setup function is a thin wrapper around the main setup. The only
 * additional thing done is to set the state machine to the RECEIVE state
 * to simplify testing the receive function.
 */
static int
tcti_tabrmd_receive_setup (void **state)
{
    data_t *data = NULL;

    tcti_tabrmd_setup (state);
    data = *state;
    TSS2_TCTI_TABRMD_STATE (data->context) = TABRMD_STATE_RECEIVE;
    return 0;
}
/*
 * This test sets up the call_cancel mock function to return values
 * indicating success. It then ensures that an invocation of the cancel
 * convenience macro invokes the underlying function correctly.
 */
static void
tcti_tabrmd_cancel_test (void **state)
{
    data_t *data = *state;
    TSS2_RC rc;

    will_return (__wrap_tcti_tabrmd_call_cancel_sync, TSS2_RC_SUCCESS);
    will_return (__wrap_tcti_tabrmd_call_cancel_sync, TRUE);
    rc = tss2_tcti_tabrmd_cancel (data->context);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    ASSERT_NON_NULL(data);
    ASSERT_NON_NULL(data->context);
    assert_int_equal (TSS2_TCTI_TABRMD_STATE (data->context),
                      TABRMD_STATE_RECEIVE);
}
/*
 * This test initializes the TCTI context state to TRANSMIT and then calls
 * the cancel function. This should produce a BAD_SEQUENCE error.
 */
static void
tcti_tabrmd_cancel_bad_sequence_test (void **state)
{
    data_t *data = *state;
    TSS2_RC rc;

    TSS2_TCTI_TABRMD_STATE (data->context) = TABRMD_STATE_TRANSMIT;
    rc = tss2_tcti_tabrmd_cancel (data->context);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_SEQUENCE);
    assert_int_equal (TSS2_TCTI_TABRMD_STATE (data->context),
                      TABRMD_STATE_TRANSMIT);
}
/*
 * This test ensures that when passed a NULL reference for both the handles
 * and num_handles parameters the appropriate RC is returned.
 */
static void
tcti_tabrmd_get_poll_handles_all_null_test (void **state)
{
    data_t *data = *state;
    TSS2_RC rc;

    rc = tss2_tcti_tabrmd_get_poll_handles (data->context, NULL, NULL);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_REFERENCE);
}
/*
 * This test invokes the get_poll_handles function passing in a NULL pointer
 * to the handles variable and a valid reference to the count. It then ensures
 * that the result is some number of handles in num_handles out parameter.
 */
static void
tcti_tabrmd_get_poll_handles_count_test (void **state)
{
    data_t *data = *state;
    size_t num_handles = 0;
    TSS2_RC rc;

    rc = tss2_tcti_tabrmd_get_poll_handles (data->context, NULL, &num_handles);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_true (num_handles > 0);
}
/*
 * This test ensures that when passed a valid handles reference with a
 * num_handles parameter that indicates an insufficient number of elements
 * in the handles array, that we get back the appropriate RC.
 */
static void
tcti_tabrmd_get_poll_handles_bad_handles_count_test (void **state)
{
    data_t *data = *state;
    TSS2_TCTI_POLL_HANDLE handles[5] = {
        TSS2_TCTI_POLL_HANDLE_ZERO_INIT, TSS2_TCTI_POLL_HANDLE_ZERO_INIT,
        TSS2_TCTI_POLL_HANDLE_ZERO_INIT, TSS2_TCTI_POLL_HANDLE_ZERO_INIT,
        TSS2_TCTI_POLL_HANDLE_ZERO_INIT
    };
    size_t num_handles = 0;
    TSS2_RC rc;

    rc = tss2_tcti_tabrmd_get_poll_handles (data->context, handles, &num_handles);
    assert_int_equal (rc, TSS2_TCTI_RC_INSUFFICIENT_BUFFER);
}
/*
 * This test invokes the get_poll_handles function passing in valid pointer
 * to an array for the handles and a valid reference to the count. It then
 * ensures that the result is some number of handles in num_handles out
 * parameter as well as valid handles in the array.
 */
static void
tcti_tabrmd_get_poll_handles_handles_test (void **state)
{
    data_t *data = *state;
    TSS2_TCTI_POLL_HANDLE handles[5] = {
        TSS2_TCTI_POLL_HANDLE_ZERO_INIT, TSS2_TCTI_POLL_HANDLE_ZERO_INIT,
        TSS2_TCTI_POLL_HANDLE_ZERO_INIT, TSS2_TCTI_POLL_HANDLE_ZERO_INIT,
        TSS2_TCTI_POLL_HANDLE_ZERO_INIT
    };
    size_t num_handles = 5;
    TSS2_RC rc;
    int fd = -1;

    rc = tss2_tcti_tabrmd_get_poll_handles (data->context, handles, &num_handles);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_int_equal (1, num_handles);
    ASSERT_NON_NULL(data);
    ASSERT_NON_NULL(data->context);
    fd = TSS2_TCTI_TABRMD_FD (data->context);
    assert_int_equal (handles [0].fd, fd);
}
/*
 * This test sets up the call_set_locality mock function to return values
 * indicating success. It then ensures that an invocation of the set_locality
 * convenience macro invokes the underlying function correctly.
 */
static void
tcti_tabrmd_set_locality_test (void **state)
{
    data_t *data = *state;
    uint8_t locality = 0;
    TSS2_RC rc;

    will_return (__wrap_tcti_tabrmd_call_set_locality_sync, TSS2_RC_SUCCESS);
    will_return (__wrap_tcti_tabrmd_call_set_locality_sync, TRUE);
    rc = tss2_tcti_tabrmd_set_locality (data->context, locality);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
}
/*
 * This test invokes the set_locality function with the context in the RECEIVE
 * state. This should produce a BAD_SEQUENCE error.
 */
static void
tcti_tabrmd_set_locality_bad_sequence_test (void **state)
{
    data_t *data = *state;
    uint8_t locality = 0;
    TSS2_RC rc;

    TSS2_TCTI_TABRMD_STATE (data->context) = TABRMD_STATE_RECEIVE;
    rc = tss2_tcti_tabrmd_set_locality (data->context, locality);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_SEQUENCE);
}
int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test (tcti_tabrmd_init_size_test),
        cmocka_unit_test (tcti_tabrmd_init_success_return_value_test),
        cmocka_unit_test (tcti_tabrmd_init_allnull_is_bad_value_test),
        cmocka_unit_test (tcti_tabrmd_proxy_new_fail),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_init_success_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test (tcti_tabrmd_info_test),
        cmocka_unit_test (tcti_tabrmd_bus_type_from_str_session_test),
        cmocka_unit_test (tcti_tabrmd_bus_type_from_str_system_test),
        cmocka_unit_test (tcti_tabrmd_bus_type_from_str_bad_test),
        cmocka_unit_test (tcti_tabrmd_kv_callback_name_test),
        cmocka_unit_test (tcti_tabrmd_kv_callback_type_good_test),
        cmocka_unit_test (tcti_tabrmd_kv_callback_type_bad_test),
        cmocka_unit_test (tcti_tabrmd_kv_callback_bad_key_test),
        cmocka_unit_test (tcti_tabrmd_conf_parse_named_session_test),
        cmocka_unit_test (tcti_tabrmd_conf_parse_named_system_test),
        cmocka_unit_test (tcti_tabrmd_conf_parse_bad_type_test),
        cmocka_unit_test (tcti_tabrmd_conf_parse_no_name_test),
        cmocka_unit_test (tcti_tabrmd_conf_parse_no_type_test),
        cmocka_unit_test (tcti_tabrmd_conf_parse_no_value_test),
        cmocka_unit_test (tcti_tabrmd_conf_parse_no_key_test),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_magic_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_version_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test (tcti_tabrmd_transmit_null_context_test),
        cmocka_unit_test (tcti_tabrmd_cancel_null_context_test),
        cmocka_unit_test (tcti_tabrmd_get_poll_handles_null_context_test),
        cmocka_unit_test (tcti_tabrmd_set_locality_null_context_test),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_transmit_success_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_transmit_bad_magic_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_transmit_bad_version_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_transmit_bad_sequence_receive_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_transmit_bad_sequence_final_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_cancel_test,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_cancel_bad_sequence_test,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_get_poll_handles_all_null_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_get_poll_handles_bad_handles_count_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_get_poll_handles_count_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_get_poll_handles_handles_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_set_locality_test,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_set_locality_bad_sequence_test,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
