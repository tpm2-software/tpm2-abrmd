/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tpm2-command.h"
#include "util.h"

#define HANDLE_FIRST  0x80000000
#define HANDLE_SECOND 0x80000001

uint8_t cmd_with_auths [] = {
    0x80, 0x02, /* TPM2_ST_SESSIONS */
    0x00, 0x00, 0x00, 0x73, /* command buffer size */
    0x00, 0x00, 0x01, 0x37, /* command code: 0x137 / TPM2_CC_NV_Write */
    0x01, 0x50, 0x00, 0x20, /* auth handle */
    0x01, 0x50, 0x00, 0x20, /* nv index handle */
    0x00, 0x00, 0x00, 0x92, /* size of auth area (2x73 byte auths) */
    0x02, 0x00, 0x00, 0x00, /* auth session handle */
    0x00, 0x20, /* sizeof caller nonce */
    0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
    0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
    0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
    0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
    0x01, /* session attributes */
    0x00, 0x20, /* sizeof  hmac */
    0x4d, 0x91, 0x26, 0xa3, 0xd9, 0xf6, 0x74, 0xde,
    0x98, 0x94, 0xb1, 0x0f, 0xe6, 0xb1, 0x5c, 0x72,
    0x7d, 0x36, 0xeb, 0x39, 0x6b, 0xf2, 0x31, 0x72,
    0x89, 0xb6, 0xc6, 0x8e, 0x54, 0xa9, 0x4c, 0x3e,
    0x02, 0x00, 0x00, 0x01, /* auth session handle */
    0x00, 0x20, /* sizeof caller nonce */
    0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
    0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
    0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
    0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
    0x01, /* session attributes */
    0x00, 0x20, /* sizeof  hmac */
    0x4d, 0x91, 0x26, 0xa3, 0xd9, 0xf6, 0x74, 0xde,
    0x98, 0x94, 0xb1, 0x0f, 0xe6, 0xb1, 0x5c, 0x72,
    0x7d, 0x36, 0xeb, 0x39, 0x6b, 0xf2, 0x31, 0x72,
    0x89, 0xb6, 0xc6, 0x8e, 0x54, 0xa9, 0x4c, 0x3e,
    0x00, 0x10, /* sizeof data */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x00, 0x00
};
/* command buffer for ContextFlush command with no handle */
uint8_t cmd_buf_context_flush_no_handle [] = {
    0x80, 0x01, /* TPM2_ST_NO_SESSIONS */
    0x00, 0x00, 0x00, 0x0a, /* size: 10 bytes */
    0x00, 0x00, 0x01, 0x65, /* command code for ContextFlush */
};

typedef struct {
    Tpm2Command *command;
    guint8      *buffer;
    size_t       buffer_size;
    Connection *connection;
} test_data_t;
/**
 * This is the minimum work required to instantiate a Tpm2Command. It needs
 * a data buffer to hold the command and a Connection object. We also
 * allocate a structure to hold these things so that we can free them in
 * the teardown.
 */
static int
tpm2_command_setup_base (void **state)
{
    test_data_t *data   = NULL;
    gint         client_fd;
    GIOStream   *iostream;
    HandleMap   *handle_map;

    data = calloc (1, sizeof (test_data_t));
    /* allocate a buffer large enough to hold a TPM2 header and 3 handles */
    data->buffer_size = TPM_RESPONSE_HEADER_SIZE + sizeof (TPM2_HANDLE) * 3;
    data->buffer = calloc (1, data->buffer_size);
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    data->connection = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    *state = data;
    return 0;
}
static int
tpm2_command_setup (void **state)
{
    test_data_t *data   = NULL;
    TPMA_CC  attributes = 0x0;

    tpm2_command_setup_base (state);
    data = (test_data_t*)*state;
    data->command = tpm2_command_new (data->connection,
                                      data->buffer,
                                      data->buffer_size,
                                      attributes);
    *state = data;
    return 0;
}
static int
tpm2_command_setup_two_handles (void **state)
{
    test_data_t *data = NULL;
    TPMA_CC  attributes = 2 << 25;

    tpm2_command_setup_base (state);
    data = (test_data_t*)*state;
    data->command = tpm2_command_new (data->connection,
                                      data->buffer,
                                      data->buffer_size,
                                      attributes);
    /*
     * This sets the two handles to 0x80000000 and 0x80000001, assuming the
     * buffer was initialized to all 0's
     */
    data->buffer [10] = 0x80;
    data->buffer [14] = 0x80;
    data->buffer [17] = 0x01;
    return 0;
}

uint8_t two_handles_not_three [] = {
    0x80, 0x02, /* TPM2_ST_SESSIONS */
    0x00, 0x00, 0x00, 0x12, /* command buffer size */
    0x00, 0x00, 0x01, 0x49, /* command code: 0x149 / TPM2_CC_PolicyNV */
    0x01, 0x02, 0x03, 0x04, /* first handle */
    0xf0, 0xe0, 0xd0, 0xc0  /* second handle */
};
/*
 * This setup function creates all of the components necessary to carry out
 * a tpm2_command test. Additionally it creates a tpm2_command with a command
 * buffer that should have 3 handles (per the TPMA_CC) but is only large
 * enough to hold 2.
 */
static int
tpm2_command_setup_two_handles_not_three (void **state)
{
    test_data_t *data   = NULL;
    gint         client_fd;
    GIOStream   *iostream;
    HandleMap   *handle_map;

    data = calloc (1, sizeof (test_data_t));
    *state = data;
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    data->connection = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    /* */
    data->buffer_size = sizeof (two_handles_not_three);
    data->buffer = calloc (1, data->buffer_size);
    memcpy (data->buffer,
            two_handles_not_three,
            data->buffer_size);
    data->command = tpm2_command_new (data->connection,
                                      data->buffer,
                                      data->buffer_size,
                                      (TPMA_CC)((UINT32)0x06000149));
    return 0;
}
/*
 * This test setup function is much like the others with the exception of the
 * Tpm2Command buffer being set to the 'cmd_with_auths'. This allows testing
 * of the functions that parse / process the auth are of the command.
 */
static int
tpm2_command_setup_with_auths (void **state)
{
    test_data_t *data   = NULL;
    gint         client_fd;
    GIOStream   *iostream;
    HandleMap   *handle_map;
    TPMA_CC attributes = 2 << 25;

    data = calloc (1, sizeof (test_data_t));
    /* allocate a buffer large enough to hold the cmd_with_auths buffer */
    data->buffer_size = sizeof (cmd_with_auths);
    data->buffer = calloc (1, data->buffer_size);
    memcpy (data->buffer, cmd_with_auths, sizeof (cmd_with_auths));
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    data->connection = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    data->command = tpm2_command_new (data->connection,
                                      data->buffer,
                                      data->buffer_size,
                                      attributes);

    *state = data;
    return 0;
}
static int
tpm2_command_setup_flush_context_no_handle (void **state)
{
    test_data_t *data   = NULL;
    gint         client_fd;
    GIOStream   *iostream;
    HandleMap   *handle_map;
    TPMA_CC attributes = 2 << 25;

    data = calloc (1, sizeof (test_data_t));
    /* allocate a buffer large enough to hold the cmd_with_auths buffer */
    data->buffer_size = sizeof (cmd_buf_context_flush_no_handle);
    data->buffer = calloc (1, data->buffer_size);
    memcpy (data->buffer,
            cmd_buf_context_flush_no_handle,
            data->buffer_size);
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    data->connection = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    data->command = tpm2_command_new (data->connection,
                                      data->buffer,
                                      data->buffer_size,
                                      attributes);

    *state = data;
    return 0;
}
/**
 * Tear down all of the data from the setup function. We don't have to
 * free the data buffer (data->buffer) since the Tpm2Command frees it as
 * part of its finalize function.
 */
static int
tpm2_command_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_object_unref (data->connection);
    g_object_unref (data->command);
    free (data);
    return 0;
}
/**
 * This is a test for memory management / reference counting. The setup
 * function does exactly that so when we get the Tpm2Command object we just
 * check to be sure it's a GObject and then we unref it. This test will
 * probably only fail when run under valgrind if the reference counting is
 * off.
 */
static void
tpm2_command_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (G_IS_OBJECT (data->command));
    assert_true (IS_TPM2_COMMAND (data->command));
}

static void
tpm2_command_get_connection_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (data->connection, tpm2_command_get_connection (data->command));
}

static void
tpm2_command_get_buffer_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (data->buffer, tpm2_command_get_buffer (data->command));
}

static void
tpm2_command_get_tag_test (void **state)
{
    test_data_t         *data   = (test_data_t*)*state;
    guint8              *buffer = tpm2_command_get_buffer (data->command);
    TPMI_ST_COMMAND_TAG  tag_ret;

    /* this is TPM2_ST_SESSIONS in network byte order */
    buffer[0] = 0x80;
    buffer[1] = 0x02;

    tag_ret = tpm2_command_get_tag (data->command);
    assert_int_equal (tag_ret, TPM2_ST_SESSIONS);
}

static void
tpm2_command_get_size_test (void **state)
{
    test_data_t *data     = (test_data_t*)*state;
    guint8      *buffer   = tpm2_command_get_buffer (data->command);
    guint32      size_ret = 0;

    /* this is tpm_st_connections in network byte order */
    buffer[0] = 0x80;
    buffer[1] = 0x02;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x06;

    size_ret = tpm2_command_get_size (data->command);
    assert_int_equal (0x6, size_ret);
}

static void
tpm2_command_get_code_test (void **state)
{
    test_data_t *data     = (test_data_t*)*state;
    guint8      *buffer   = tpm2_command_get_buffer (data->command);
    TPM2_CC       command_code;

    /**
     * This is TPM2_ST_SESSIONS + a size of 0x0a + the command code for
     * GetCapability in network byte order
     */
    buffer[0] = 0x80;
    buffer[1] = 0x02;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x0a;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x01;
    buffer[9] = 0x7a;

    command_code = tpm2_command_get_code (data->command);
    assert_int_equal (command_code, TPM2_CC_GetCapability);
}

static void
tpm2_command_get_two_handle_count_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    guint8 command_handles;

    command_handles = tpm2_command_get_handle_count (data->command);
    assert_int_equal (command_handles, 2);
}

static void
tpm2_command_get_handles_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE handles [3] = { 0, 0, 0 };
    size_t count = 3;
    gboolean ret;

    ret = tpm2_command_get_handles (data->command, handles, &count);
    assert_true (ret == TRUE);
    assert_int_equal (handles [0], HANDLE_FIRST);
    assert_int_equal (handles [1], HANDLE_SECOND);
}

/*
 * Get the handle at the first position in the handle area of the command.
 */
static void
tpm2_command_get_handle_first_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE   handle_out;

    handle_out = tpm2_command_get_handle (data->command, 0);
    assert_int_equal (handle_out, HANDLE_FIRST);
}
/*
 * Get the handle at the second position in the handle area of the command.
 */
static void
tpm2_command_get_handle_second_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE   handle_out;

    handle_out = tpm2_command_get_handle (data->command, 1);
    assert_int_equal (handle_out, HANDLE_SECOND);
}
/*
 * Attempt to get the handle at the third position in the handle area of the
 * command. This should fail since the command has only two handles.
 */
static void
tpm2_command_get_handle_fail_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE   handle_out;

    handle_out = tpm2_command_get_handle (data->command, 2);
    assert_int_equal (handle_out, 0);
}
/*
 */
static void
tpm2_command_set_handle_first_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE   handle_in = 0xdeadbeef, handle_out = 0;
    gboolean     ret;

    ret = tpm2_command_set_handle (data->command, handle_in, 0);
    assert_true (ret);
    handle_out = tpm2_command_get_handle (data->command, 0);
    assert_int_equal (handle_out, handle_in);
}
static void
tpm2_command_set_handle_second_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE   handle_in = 0xdeadbeef, handle_out = 0;
    gboolean     ret;

    ret = tpm2_command_set_handle (data->command, handle_in, 1);
    assert_true (ret);
    handle_out = tpm2_command_get_handle (data->command, 1);
    assert_int_equal (handle_out, handle_in);
}
static void
tpm2_command_set_handle_fail_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE   handle_in = 0xdeadbeef;
    gboolean     ret;

    ret = tpm2_command_set_handle (data->command, handle_in, 2);
    assert_false (ret);
}
static void
tpm2_command_get_auth_size_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    UINT32 auths_area_size = 0;

    auths_area_size = tpm2_command_get_auths_size (data->command);
    assert_int_equal (auths_area_size, 0x92);
}
/*
 * This structure is used to track state while processing the authorizations
 * from the command authorization area.
 */
typedef struct {
    Tpm2Command *command;
    size_t counter;
    size_t handles_count;
    TPM2_HANDLE handles [3];
} callback_auth_state_t;
/*
 * The tpm2_command_foreach_auth function invokes this function for each
 * authorization in the command authorization area. The 'user_data' is an
 * instance of the callback_auth_state_t structure that we use to track state.
 * The expected handles from the auth area are in the handles array in the
 * order that they should be received. We then use the 'counter' to identify
 * which handle we should receive for each callback (assuming that the
 * callback is invoked for each auth IN ORDER).
 */
static void
tpm2_command_foreach_auth_callback (gpointer authorization,
                                    gpointer user_data)
{
    size_t auth_offset = *(size_t*)authorization;
    callback_auth_state_t *callback_state = (callback_auth_state_t*)user_data;
    TPM2_HANDLE handle;

    handle = tpm2_command_get_auth_handle (callback_state->command,
                                           auth_offset);
    g_debug ("tpm2_command_foreach_auth_callback:\n  counter: %zd\n"
            "  handles_count: %zd\n  handle: 0x%08" PRIx32,
            callback_state->counter,
            callback_state->handles_count,
            callback_state->handles [callback_state->counter]);
    g_debug ("  auth_offset: %zu", auth_offset);
    g_debug ("  AUTH_HANDLE_GET: 0x%08" PRIx32, handle);

    assert_true (callback_state->counter < callback_state->handles_count);
    assert_int_equal (handle,
                      callback_state->handles [callback_state->counter]);
    ++callback_state->counter;
}
/*
 * This test exercises the tpm2_command_foreach_auth function. The state
 * structure must be initialized with the handles that are expected from
 * the command. Each time the callback is invoked we compare the authorization
 * handle to the handles array.
 */
static void
tpm2_command_foreach_auth_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    /* this data is highly dependent on the */
    callback_auth_state_t callback_state = {
        .command = data->command,
        .counter = 0,
        .handles_count = 2,
        .handles = {
            0x02000000,
            0x02000001,
        },
    };

    tpm2_command_foreach_auth (data->command,
                               tpm2_command_foreach_auth_callback,
                               &callback_state);
}
static void
tpm2_command_flush_context_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;
    TPM2_HANDLE handle;

    rc = tpm2_command_get_flush_handle (data->command,
                                        &handle);
    assert_int_equal (rc, RM_RC (TPM2_RC_INSUFFICIENT));
}
/*
 * This test attempts to read 3 handles from a command. It's paired with
 * the `tpm2_command_setup_two_handles_not_three` setup function which
 * populates the command with data appropriate for this test. The command
 * body is for a command with 3 handles but only enough room is provided
 * for two handles. Attempts to read 3 should stop at the end of the buffer.
 */
static void
tpm2_command_two_handles_not_three (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE   handles [3] = { 0 };
    size_t       handle_count = 3;
    gboolean     ret = TRUE;

    ret = tpm2_command_get_handles (data->command,
                                    handles,
                                    &handle_count);
    assert_true (ret);
}

uint8_t get_cap_no_cap [] = {
    0x80, 0x02, /* TPM2_ST_SESSIONS */
    0x00, 0x00, 0x00, 0x0a, /* command buffer size */
    0x00, 0x00, 0x01, 0x7a, /* TPM2_CC_GetCapability */
};
/*
 * This setup function creates all of the components necessary to carry out
 * a tpm2_command test. Additionally it creates a tpm2_command with a command
 * buffer that should have 3 handles (per the TPMA_CC) but is only large
 * enough to hold 2.
 */
static int
tpm2_command_setup_get_cap_no_cap (void **state)
{
    test_data_t *data   = NULL;
    gint         client_fd;
    GIOStream   *iostream;
    HandleMap   *handle_map;

    data = calloc (1, sizeof (test_data_t));
    *state = data;
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    data->connection = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);

    data->buffer_size = sizeof (get_cap_no_cap);
    data->buffer = calloc (1, data->buffer_size);
    memcpy (data->buffer,
            get_cap_no_cap,
            data->buffer_size);
    data->command = tpm2_command_new (data->connection,
                                      data->buffer,
                                      data->buffer_size,
                                      (TPMA_CC)((UINT32)0x0600017a));
    return 0;
}
static void
tpm2_command_get_cap_no_cap (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_CAP cap;

    cap = tpm2_command_get_cap (data->command);
    assert_int_equal (cap, 0);
}
static void
tpm2_command_get_cap_no_prop (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    UINT32 prop;

    prop = tpm2_command_get_prop (data->command);
    assert_int_equal (prop, 0);
}
static void
tpm2_command_get_cap_no_count (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    UINT32 count;

    count = tpm2_command_get_prop_count (data->command);
    assert_int_equal (count, 0);
}

gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (tpm2_command_type_test,
                                         tpm2_command_setup,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_connection_test,
                                         tpm2_command_setup,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_buffer_test,
                                         tpm2_command_setup,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_tag_test,
                                         tpm2_command_setup,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_size_test,
                                         tpm2_command_setup,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_code_test,
                                         tpm2_command_setup,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_two_handle_count_test,
                                         tpm2_command_setup_two_handles,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_handles_test,
                                         tpm2_command_setup_two_handles,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_handle_first_test,
                                         tpm2_command_setup_two_handles,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_handle_second_test,
                                         tpm2_command_setup_two_handles,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_handle_fail_test,
                                         tpm2_command_setup_two_handles,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_set_handle_first_test,
                                         tpm2_command_setup_two_handles,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_set_handle_second_test,
                                         tpm2_command_setup_two_handles,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_set_handle_fail_test,
                                         tpm2_command_setup_two_handles,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_auth_size_test,
                                         tpm2_command_setup_with_auths,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_foreach_auth_test,
                                         tpm2_command_setup_with_auths,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_flush_context_handle_test,
                                         tpm2_command_setup_flush_context_no_handle,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_two_handles_not_three,
                                         tpm2_command_setup_two_handles_not_three,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_cap_no_cap,
                                         tpm2_command_setup_get_cap_no_cap,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_cap_no_prop,
                                         tpm2_command_setup_get_cap_no_cap,
                                         tpm2_command_teardown),
        cmocka_unit_test_setup_teardown (tpm2_command_get_cap_no_count,
                                         tpm2_command_setup_get_cap_no_cap,
                                         tpm2_command_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
