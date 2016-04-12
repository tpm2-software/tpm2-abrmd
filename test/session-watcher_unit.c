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

#include "session-manager.h"
#include "session-watcher.h"

static void
session_watcher_allocate_test (void **state)
{
    session_manager_t *manager = (session_manager_t*)*state;
    session_watcher_t *watcher = NULL;

    watcher = session_watcher_new (manager, 0);
    assert_non_null (watcher);
    session_watcher_free (watcher);
}

static void
session_watcher_allocate_setup (void **state)
{
    session_manager_t *manager = NULL;

    manager = session_manager_new ();
    *state = manager;
}

static void
session_watcher_allocate_teardown (void **state)
{
    session_manager_t *manager = (session_manager_t*)*state;

    session_manager_free (manager);
}

struct watcher_start_data {
    session_manager_t *manager;
    session_watcher_t *watcher;
    gint wakeup_send_fd;
};

void
session_watcher_start_test (void **state)
{
    struct watcher_start_data *data;
    gint ret;

    data = (struct watcher_start_data*)*state;
    session_watcher_start(data->watcher);
    assert_true (data->watcher->running);
    ret = session_watcher_cancel (data->watcher);
    assert_int_equal (ret, 0);
    ret = session_watcher_join (data->watcher);
    assert_int_equal (ret, 0);
}

static void
session_watcher_start_setup (void **state)
{
    struct watcher_start_data *data;
    gint fds[2] = { 0 }, ret;

    data = calloc (1, sizeof (struct watcher_start_data));
    data->manager = session_manager_new ();
    if (data->manager == NULL)
        g_error ("failed to allocate new session_manager");
    ret = pipe2 (fds, O_CLOEXEC);
    if (ret != 0)
        g_error ("failed to get pipe2s");
    data->wakeup_send_fd = fds[1];
    data->watcher = session_watcher_new (data->manager, fds[0]);
    if (data->watcher == NULL)
        g_error ("failed to allocate new session_watcher");

    *state = data;
}

static void
session_watcher_start_teardown (void **state)
{
    struct watcher_start_data *data;

    data = (struct watcher_start_data*)*state;
    close (data->watcher->wakeup_receive_fd);
    session_watcher_free (data->watcher);
    session_manager_free (data->manager);
    close (data->wakeup_send_fd);
}

int
main (int argc,
      char* argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (session_watcher_allocate_test,
                                  session_watcher_allocate_setup,
                                  session_watcher_allocate_teardown),
        unit_test_setup_teardown (session_watcher_start_test,
                                  session_watcher_start_setup,
                                  session_watcher_start_teardown),
    };
    return run_tests (tests);
}
