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

#include "session.h"
#include "session-manager.h"

static void
session_manager_allocate_test (void **state)
{
    session_manager_t *manager = NULL;

    manager = session_manager_new ();
    assert_non_null (manager);
    session_manager_free (manager);
}

static void
session_manager_setup (void **state)
{
    session_manager_t *manager = NULL;

    manager = session_manager_new ();
    *state = manager;
}

static void
session_manager_teardown (void **state)
{
    session_manager_t *manager = (session_manager_t*)*state;

    session_manager_free (manager);
}

static void
session_manager_insert_test (void **state)
{
    session_manager_t *manager = (session_manager_t*)*state;
    session_t *session = NULL;
    gint ret;

    session = session_new ();
    ret = session_manager_insert (manager, session);
    assert_int_equal (ret, 0);
}

static void
session_manager_lookup_test (void **state)
{
    session_manager_t *manager = (session_manager_t*)*state;
    session_t *session = NULL, *session_lookup = NULL;
    gint ret;

    session = session_new ();
    ret = session_manager_insert (manager, session);
    session_lookup = session_manager_lookup (manager, *(int*)session_key (session));
    assert_int_equal (session, session_lookup);
}

static void
session_manager_remove_test (void **state)
{
    session_manager_t *manager = (session_manager_t*)*state;
    session_t *session = NULL;
    gint ret_int;
    gboolean ret_bool;

    session = session_new ();
    ret_int = session_manager_insert (manager, session);
    assert_int_equal (ret_int, 0);
    ret_bool = session_manager_remove (manager, session);
    assert_true (ret_bool);
}

static void
session_manager_set_fds_test (void **state)
{
    session_manager_t *manager = (session_manager_t*)*state;
    session_t *first_session = NULL, *second_session = NULL;
    gint ret_int;
    fd_set manager_fds = { 0 };

    first_session = session_new ();
    ret_int = session_manager_insert (manager, first_session);
    assert_int_equal (ret_int, 0);
    second_session = session_new ();
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
    session_free (first_session);
    session_free (second_session);
}

int
main(int argc, char* argv[])
{
    const UnitTest tests[] = {
        unit_test (session_manager_allocate_test),
        unit_test_setup_teardown (session_manager_insert_test,
                                  session_manager_setup,
                                  session_manager_teardown),
        unit_test_setup_teardown (session_manager_lookup_test,
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
