#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "blob-queue.h"
#include "session-data.h"

typedef struct blob_unit_data {
    blob_t          *blob;
    session_data_t  *session;
    guint8          *data;
    size_t           data_size;
} blob_unit_data_t;

static void
blob_allocate_deallocate_allnull_test (void **state)
{
    blob_t *blob = blob_new (NULL, NULL, 0);
    assert_non_null (blob);
    blob_free (blob);
}

static void
blob_allocate_deallocate_with_session_test (void **state)
{
    gint receive_fd, send_fd;
    session_data_t *session = session_data_new (&receive_fd, &send_fd, 0);
    blob_t *blob = blob_new (session, NULL, 0);
    assert_non_null (blob);
    blob_free (blob);
    /* freeing the blob should not free the session */
    assert_non_null (session);
    session_data_free (session);
    close (receive_fd);
    close (send_fd);
}

static void
blob_allocate_deallocate_with_data_test (void **state)
{
    guint8 *data = calloc (1, 5);
    blob_t *blob = blob_new (NULL, data, 5);
    assert_non_null (blob);
    blob_free (blob);
}
static void
blob_setup (void **state)
{
    gint receive_fd, send_fd;
    blob_unit_data_t *blob_unit_data = calloc (1, sizeof (blob_unit_data_t));
    blob_unit_data->data_size = 5;
    blob_unit_data->session = session_data_new (&receive_fd, &send_fd, 0);
    blob_unit_data->data = calloc (1, blob_unit_data->data_size);
    blob_unit_data->blob = blob_new (blob_unit_data->session,
                                     blob_unit_data->data,
                                     blob_unit_data->data_size);
    *state = blob_unit_data;
}
static void
blob_teardown (void **state)
{
    blob_unit_data_t *blob_unit_data = *state;
    blob_free (blob_unit_data->blob);
    session_data_free (blob_unit_data->session);
    free (blob_unit_data);
}
static void
blob_session_match_test (void **state)
{
    blob_unit_data_t *blob_unit_data = *state;
    assert_int_equal (blob_unit_data->session,
                      blob_get_session (blob_unit_data->blob));
}
static void
blob_data_match_test (void **state)
{
    blob_unit_data_t *blob_unit_data = *state;
    assert_int_equal (blob_unit_data->data,
                      blob_get_data (blob_unit_data->blob));
}
static void
blob_data_size_match_test (void **state)
{
    blob_unit_data_t *blob_unit_data = *state;
    assert_int_equal (blob_unit_data->data_size,
                      blob_get_size (blob_unit_data->blob));
}
gint
main(gint argc, gchar* argv[])
{
    const UnitTest tests[] = {
        unit_test (blob_allocate_deallocate_allnull_test),
        unit_test (blob_allocate_deallocate_with_session_test),
        unit_test (blob_allocate_deallocate_with_data_test),
        unit_test_setup_teardown (blob_session_match_test,
                                  blob_setup,
                                  blob_teardown),
        unit_test_setup_teardown (blob_data_match_test,
                                  blob_setup,
                                  blob_teardown),
        unit_test_setup_teardown (blob_data_size_match_test,
                                  blob_setup,
                                  blob_teardown),
    };
    return run_tests(tests);
}
