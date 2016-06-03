#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "data-message.h"

static void
data_message_new_unref_with_data_test (void **state)
{
    DataMessage     *msg     = NULL;
    guint8          *data    = NULL;
    size_t           size    = 0;
    session_data_t  *session = NULL;

    size = 10;
    data = calloc (1, size);

    msg = data_message_new (session, data, size);
    assert_int_equal (session, msg->session);
    assert_int_equal (data, msg->data);
    assert_int_equal (size, msg->size);

    g_object_unref (msg);
}

static void
data_message_new_unref_test (void **state)
{
    DataMessage     *msg     = NULL;
    guint8          *data    = NULL;
    size_t           size    = 0;
    session_data_t  *session = NULL;

    msg = data_message_new (session, data, size);
    assert_int_equal (session, msg->session);
    assert_int_equal (data, msg->data);
    assert_int_equal (size, msg->size);

    g_object_unref (msg);
}

gint
main (gint    argc,
      gchar  *argv[])
{
    const UnitTest tests[] = {
        unit_test (data_message_new_unref_test),
        unit_test (data_message_new_unref_with_data_test),
    };
    return run_tests (tests);
}
