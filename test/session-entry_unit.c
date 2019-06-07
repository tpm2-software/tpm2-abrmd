/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "session-entry.h"
#include "util.h"

#define CLIENT_ID        1ULL
#define TEST_HANDLE      0x03000000

typedef struct {
    Connection   *connection;
    gint          client_fd;
    HandleMap    *handle_map;
    SessionEntry *session_entry;
} test_data_t;
/*
 * Setup function:
 * Creates a structure to hold test data.
 * Creates supporting objects for tests: HandleMap, IOStream, Connection.
 * Creates a SessionEntry (the object under test).
 */
static int
session_entry_setup (void **state)
{
    test_data_t *data   = NULL;
    GIOStream *iostream;

    data = calloc (1, sizeof (test_data_t));
    data->handle_map = handle_map_new (TPM2_HT_TRANSIENT, 100);
    iostream = create_connection_iostream (&data->client_fd);
    data->connection = connection_new (iostream,
                                       CLIENT_ID,
                                       data->handle_map);
    data->session_entry = session_entry_new (data->connection, TEST_HANDLE);
    g_object_unref (iostream);

    *state = data;
    return 0;
}
/**
 * Tear down all of the data from the setup function. We don't have to
 * free the data buffer (data->buffer) since the Tpm2Command frees it as
 * part of its finalize function.
 */
static int
session_entry_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_object_unref (data->connection);
    g_object_unref (data->handle_map);
    g_object_unref (data->session_entry);
    free (data);
    return 0;
}
/*
 * This is a test for memory management / reference counting. The setup
 * function does exactly that so when we get the Tpm2Command object we just
 * check to be sure it's a GObject and then we unref it. This test will
 * probably only fail when run under valgrind if the reference counting is
 * off.
 */
static void
session_entry_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (G_IS_OBJECT (data->session_entry));
    assert_true (IS_SESSION_ENTRY (data->session_entry));
}

static void
session_entry_get_context_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    size_buf_t *size_buf;

    size_buf = session_entry_get_context (data->session_entry);
    assert_non_null (size_buf);
}

static void
session_entry_get_connection_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    Connection *connection;
    connection = session_entry_get_connection (data->session_entry);
    assert_true (IS_CONNECTION (connection));
    g_object_unref (connection);
}

static void
session_entry_get_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    TPM2_HANDLE handle;
    handle = session_entry_get_handle (data->session_entry);
    assert_int_equal (handle, TEST_HANDLE);
}

gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (session_entry_type_test,
                                         session_entry_setup,
                                         session_entry_teardown),
        cmocka_unit_test_setup_teardown (session_entry_get_context_test,
                                         session_entry_setup,
                                         session_entry_teardown),
        cmocka_unit_test_setup_teardown (session_entry_get_connection_test,
                                         session_entry_setup,
                                         session_entry_teardown),
        cmocka_unit_test_setup_teardown (session_entry_get_handle_test,
                                         session_entry_setup,
                                         session_entry_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
