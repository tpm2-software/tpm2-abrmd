#include <glib.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "session-data.h"
#include "session-manager.h"

static void
session_manager_allocate_test (void **state)
{
    SessionManager *manager = NULL;

    manager = session_manager_new (MAX_SESSIONS_DEFAULT);
    assert_non_null (manager);
    g_object_unref (manager);
}

static void
session_manager_setup (void **state)
{
    SessionManager *manager = NULL;

    manager = session_manager_new (MAX_SESSIONS_DEFAULT);
    assert_non_null (manager);
    *state = manager;
}

static void
session_manager_teardown (void **state)
{
    SessionManager *manager = SESSION_MANAGER (*state);

    g_object_unref (manager);
}

static void
session_manager_insert_test (void **state)
{
    SessionManager *manager = SESSION_MANAGER (*state);
    SessionData *session = NULL;
    HandleMap   *handle_map = NULL;
    gint ret, receive_fd, send_fd;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    session = session_data_new (&receive_fd, &send_fd, 5, handle_map);
    g_object_unref (handle_map);
    ret = session_manager_insert (manager, session);
    assert_int_equal (ret, 0);
}

static void
session_manager_lookup_fd_test (void **state)
{
    SessionManager *manager = SESSION_MANAGER (*state);
    SessionData *session = NULL, *session_lookup = NULL;
    HandleMap   *handle_map = NULL;
    gint ret, receive_fd, send_fd;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    session = session_data_new (&receive_fd, &send_fd, 5, handle_map);
    g_object_unref (handle_map);
    ret = session_manager_insert (manager, session);
    assert_int_equal (ret, TSS2_RC_SUCCESS);
    session_lookup = session_manager_lookup_fd (manager, *(int*)session_data_key_fd (session));
    assert_int_equal (session, session_lookup);
    g_object_unref (session_lookup);
}

static void
session_manager_lookup_id_test (void **state)
{
    SessionManager *manager = SESSION_MANAGER (*state);
    SessionData *session = NULL, *session_lookup = NULL;
    HandleMap   *handle_map = NULL;
    gint ret, receive_fd, send_fd;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    session = session_data_new (&receive_fd, &send_fd, 5, handle_map);
    g_object_unref (handle_map);
    ret = session_manager_insert (manager, session);
    assert_int_equal (ret, TSS2_RC_SUCCESS);
    session_lookup = session_manager_lookup_id (manager, *(int*)session_data_key_id (session));
    assert_int_equal (session, session_lookup);
}

static void
session_manager_remove_test (void **state)
{
    SessionManager *manager = SESSION_MANAGER (*state);
    SessionData *session = NULL;
    HandleMap   *handle_map = NULL;
    gint ret_int, receive_fd, send_fd;
    gboolean ret_bool;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    session = session_data_new (&receive_fd, &send_fd, 5, handle_map);
    g_object_unref (handle_map);
    ret_int = session_manager_insert (manager, session);
    assert_int_equal (ret_int, 0);
    ret_bool = session_manager_remove (manager, session);
    assert_true (ret_bool);
}

static void
session_manager_set_fds_test (void **state)
{
    SessionManager *manager = SESSION_MANAGER (*state);
    SessionData *first_session = NULL, *second_session = NULL;
    HandleMap   *first_handle_map = NULL, *second_handle_map = NULL;
    gint ret_int, receive_fd0, send_fd0, receive_fd1, send_fd1;
    fd_set manager_fds = { 0 };

    first_handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    first_session = session_data_new (&receive_fd0, &send_fd0, 5, first_handle_map);
    g_object_unref (first_handle_map);
    ret_int = session_manager_insert (manager, first_session);
    assert_int_equal (ret_int, 0);
    second_handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    second_session = session_data_new (&receive_fd1, &send_fd1, 5, second_handle_map);
    g_object_unref (second_handle_map);
    ret_int = session_manager_insert (manager, second_session);
    assert_int_equal (ret_int, 0);
    session_manager_set_fds (manager, &manager_fds);
    for (ret_int = 0; ret_int < FD_SETSIZE; ++ret_int) {
        if (ret_int == first_session->receive_fd) {
            assert_true (FD_ISSET(first_session->receive_fd, &manager_fds) != 0);
        } else if (ret_int == second_session->receive_fd) {
            assert_true (FD_ISSET (second_session->receive_fd, &manager_fds) != 0);
        } else {
            assert_true (FD_ISSET (ret_int, &manager_fds) == 0);
        }
    }
    g_object_unref (G_OBJECT (first_session));
    g_object_unref (G_OBJECT (second_session));
}

int
main(int argc, char* argv[])
{
    const UnitTest tests[] = {
        unit_test (session_manager_allocate_test),
        unit_test_setup_teardown (session_manager_insert_test,
                                  session_manager_setup,
                                  session_manager_teardown),
        unit_test_setup_teardown (session_manager_lookup_fd_test,
                                  session_manager_setup,
                                  session_manager_teardown),
        unit_test_setup_teardown (session_manager_lookup_id_test,
                                  session_manager_setup,
                                  session_manager_teardown),
        unit_test_setup_teardown (session_manager_remove_test,
                                  session_manager_setup,
                                  session_manager_teardown),
        unit_test_setup_teardown (session_manager_set_fds_test,
                                  session_manager_setup,
                                  session_manager_teardown),
    };
    return run_tests(tests);
}
