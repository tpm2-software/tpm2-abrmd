/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>


#include <tss2/tss2_tpm2_types.h>

#include "mock-io-stream.h"
#include "connection.h"
#include "session-entry.h"
#include "session-list.h"
#include "util.h"

#define INSERT_TEST_ID 0x1

typedef struct test_data {
    SessionList *session_list;
} test_data_t;

static GIOStream*
test_iostream_new (void)
{
    GInputStream *input;
    GOutputStream *output;
    GIOStream *iostream;
    int fds[2];

    socketpair (AF_LOCAL, SOCK_STREAM, 0, fds);
    input = g_unix_input_stream_new (fds[0], TRUE);
    output = g_unix_output_stream_new (fds[1], TRUE);
    iostream = mock_io_stream_new (input, output);
    g_object_unref (input);
    g_object_unref (output);
    return iostream;
}
static Connection*
test_connection_new (guint64 id)
{
    Connection *conn = NULL;
    GIOStream *iostream = NULL;
    HandleMap *handle_map = NULL;

    iostream = test_iostream_new ();
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    conn = connection_new (iostream, id, handle_map);

    g_clear_object (&iostream);
    g_clear_object (&handle_map);

    return conn;
}
/*
 * Allocate a test_data_t structure to hold test data.
 * Create a SessionList object for use in testing.
 * Create a Connection object for use by tests.
 */
static int
session_list_setup (void **state)
{
    test_data_t *data = NULL;

    data = calloc (1, sizeof (test_data_t));
    g_debug ("%s: data 0x%" PRIxPTR, __func__, (uintptr_t)data);
    data->session_list = session_list_new (SESSION_LIST_MAX_ENTRIES_DEFAULT,
                                           SESSION_LIST_MAX_ABANDONED_DEFAULT);

    *state = data;
    return 0;
}

static int
session_list_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_debug ("%s: start", __func__);
    if (data) {
        g_debug ("%s: data 0x%" PRIxPTR, __func__, (uintptr_t)data);
        g_clear_object (&data->session_list);
        free (data);
    }
    g_debug ("%s: end", __func__);

    return 0;
}
/*
 * Simple object type checks.
 */
static void
session_list_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (G_IS_OBJECT (data->session_list));
    assert_true (IS_SESSION_LIST (data->session_list));
}
/* Simple test of SessionList insert function */
#define INSERT_TEST_ID 0x1
#define INSERT_TEST_HANDLE (TPM2_HR_TRANSIENT + 1)
static void
session_list_insert_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Connection *conn = NULL;
    SessionEntry *entry = NULL;

    conn = test_connection_new (INSERT_TEST_ID);
    entry = session_entry_new (conn, INSERT_TEST_HANDLE);
    assert_true (session_list_insert (data->session_list, entry));
    g_clear_object (&conn);
    g_clear_object (&entry);
}
/*
 * Add 3 SessionEntry objects to a SessionList then verify size is reported
 * correctly.
 */
#define SIZE_TEST_ID 0x1
#define SIZE_TEST_HANDLE_1 (TPM2_HR_TRANSIENT + 1)
#define SIZE_TEST_HANDLE_2 (TPM2_HR_TRANSIENT + 2)
#define SIZE_TEST_HANDLE_3 (TPM2_HR_TRANSIENT + 3)
static void
session_list_size_three_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Connection *conn = NULL;
    SessionEntry *entry = NULL;

    conn = test_connection_new (SIZE_TEST_ID);
    entry = session_entry_new (conn, SIZE_TEST_HANDLE_1);
    session_list_insert (data->session_list, entry);
    g_clear_object (&entry);
    entry = session_entry_new (conn, SIZE_TEST_HANDLE_2);
    session_list_insert (data->session_list, entry);
    g_clear_object (&entry);
    entry = session_entry_new (conn, SIZE_TEST_HANDLE_3);
    g_clear_object (&conn);
    session_list_insert (data->session_list, entry);
    g_clear_object (&entry);
    assert_int_equal (3, session_list_size (data->session_list));
}

/*
 * Test removal of a SessionEntry using the handle as an identifier. This
 * relies on the session_list_insert_test to add the SessionEntry required
 * by this test.
 */
static void
session_list_remove_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    gboolean ret = FALSE;

    session_list_insert_test (state);
    ret = session_list_remove_handle (data->session_list, INSERT_TEST_HANDLE);
    assert_true (ret);
    assert_int_equal (session_list_size (data->session_list), 0);
}
/*
 * Test the error handling logic in the remove_handle function. This test
 * passes in a handle unknown to the SessionList and so the search for the
 * associated SessionEntry will fail. We should get FALSE back from the call.
 */
static void
session_list_remove_handle_bad_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_false (session_list_remove_handle (data->session_list, 0x55));
}

/*
 * Test our ability to detect handles for sessions that aren't tracked by the
 * SessionList while we're abandoning an entry. The code currently evaluates
 * the handle parameter first so the connection parameter is irrelevant.
 */
#define ABANDON_HANDLE_CONN_BAD ((Connection*)0x55)
#define ABANDON_HANDLE_HANDLE 0x45628376
static void
session_list_abandon_handle_bad_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    gboolean ret;

    ret = session_list_abandon_handle (data->session_list,
                                       ABANDON_HANDLE_CONN_BAD,
                                       ABANDON_HANDLE_HANDLE);
    assert_false (ret);
}
/*
 */
#define ABANDON_HANDLE_ID 0x1
#define ABANDON_HANDLE_ID_2 0x2
static void
session_list_abandon_handle_bad_connection_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Connection *conn = NULL;
    SessionEntry *entry = NULL;
    gboolean ret;

    conn = test_connection_new (ABANDON_HANDLE_ID);
    entry = session_entry_new (conn, ABANDON_HANDLE_HANDLE);
    session_list_insert (data->session_list, entry);
    g_clear_object (&conn);
    g_clear_object (&entry);

    conn = test_connection_new (ABANDON_HANDLE_ID_2);
    ret = session_list_abandon_handle (data->session_list,
                                       conn,
                                       ABANDON_HANDLE_HANDLE);
    g_clear_object (&conn);
    assert_false (ret);
}

static void
session_list_abandon_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Connection *conn = NULL;
    SessionEntry *entry = NULL;
    gboolean ret;

    conn = test_connection_new (ABANDON_HANDLE_ID);
    entry = session_entry_new (conn, ABANDON_HANDLE_HANDLE);
    session_list_insert (data->session_list, entry);
    g_clear_object (&entry);

    ret = session_list_abandon_handle (data->session_list,
                                       conn,
                                       ABANDON_HANDLE_HANDLE);
    g_clear_object (&conn);
    assert_true (ret);
}

#define CLAIM_CONNECTION_ID_0 0x0
#define CLAIM_CONNECTION_ID_1 0x1
#define CLAIM_HANDLE 0xdeadbee0
static void
session_list_claim_abandoned_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Connection *conn = NULL;
    SessionEntry *entry = NULL;
    gboolean ret;

    conn = test_connection_new (CLAIM_CONNECTION_ID_0);
    entry = session_entry_new (conn, CLAIM_HANDLE);
    session_list_insert (data->session_list, entry);

    ret = session_list_abandon_handle (data->session_list, conn, CLAIM_HANDLE);
    assert_true (ret);
    assert_true (session_entry_get_state (entry) == SESSION_ENTRY_SAVED_CLIENT_CLOSED);
    g_clear_object (&conn);

    /* claim abandoned handle from different connection */
    conn = test_connection_new (CLAIM_CONNECTION_ID_1);
    ret = session_list_claim (data->session_list, entry, conn);
    assert_true (ret);
}

static void
session_list_claim_saved_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Connection *conn = NULL;
    SessionEntry *entry = NULL;
    gboolean ret;

    conn = test_connection_new (CLAIM_CONNECTION_ID_0);
    entry = session_entry_new (conn, CLAIM_HANDLE);
    session_list_insert (data->session_list, entry);
    session_entry_set_state (entry, SESSION_ENTRY_SAVED_CLIENT);

    /* claim abandoned handle from different connection */
    conn = test_connection_new (CLAIM_CONNECTION_ID_1);
    ret = session_list_claim (data->session_list, entry, conn);
    assert_true (ret);
    assert_true (session_entry_get_state (entry) == SESSION_ENTRY_LOADED);
}

static void
session_list_claim_fail_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Connection *conn = NULL;
    SessionEntry *entry = NULL;
    gboolean ret;

    conn = test_connection_new (CLAIM_CONNECTION_ID_0);
    entry = session_entry_new (conn, CLAIM_HANDLE);

    ret = session_list_claim (data->session_list, entry, conn);
    assert_false (ret);
}

gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (session_list_type_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_insert_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_size_three_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_remove_handle_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_remove_handle_bad_handle_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_abandon_handle_bad_handle_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_abandon_handle_bad_connection_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_abandon_handle_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_claim_abandoned_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_claim_saved_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_claim_fail_test,
                                         session_list_setup,
                                         session_list_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
