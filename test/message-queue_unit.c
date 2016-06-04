#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "message-queue.h"

typedef struct msgq_test_data {
    MessageQueue *queue;
} msgq_test_data_t;

static void
message_queue_setup (void **state)
{
    msgq_test_data_t *data = NULL;

    data = calloc (1, sizeof (msgq_test_data_t));
    assert_non_null (data);
    data->queue = message_queue_new ("test MessageQueue");
    *state = data;
}

static void
message_queue_teardown (void **state)
{
    msgq_test_data_t *data = (msgq_test_data_t*)*state;

    g_object_unref (data->queue);
    free (data);
}

static void
message_queue_allocate_test (void **state)
{
    message_queue_setup (state);
    message_queue_teardown (state);
}

static void
message_queue_enqueue_dequeue_test (void **state)
{
    msgq_test_data_t *data = (msgq_test_data_t*)*state;
    MessageQueue *queue = data->queue;
    DataMessage *data_message_in, *data_message_out;

    data_message_in = data_message_new (NULL, NULL, 0);
    message_queue_enqueue (queue, G_OBJECT (data_message_in));
    data_message_out = DATA_MESSAGE (message_queue_dequeue (queue));
    /* ptr != int but they're the same size usually? */
    assert_int_equal (data_message_in, data_message_out);
}

static void
message_queue_dequeue_order_test (void **state)
{
    msgq_test_data_t *data = (msgq_test_data_t*)*state;
    MessageQueue *queue = data->queue;
    DataMessage *data_message_0, *data_message_1, *data_message_2, *data_message_tmp;

    data_message_0 = data_message_new (NULL, NULL, 0);
    data_message_1 = data_message_new (NULL, NULL, 0);
    data_message_2 = data_message_new (NULL, NULL, 0);

    message_queue_enqueue (queue, G_OBJECT (data_message_0));
    message_queue_enqueue (queue, G_OBJECT (data_message_1));
    message_queue_enqueue (queue, G_OBJECT (data_message_2));

    data_message_tmp = DATA_MESSAGE (message_queue_dequeue (queue));
    assert_int_equal (data_message_tmp, data_message_0);
    data_message_tmp = DATA_MESSAGE (message_queue_dequeue (queue));
    assert_int_equal (data_message_tmp, data_message_1);
    data_message_tmp = DATA_MESSAGE (message_queue_dequeue (queue));
    assert_int_equal (data_message_tmp, data_message_2);
    g_object_unref (data_message_0);
    g_object_unref (data_message_1);
    g_object_unref (data_message_2);
}

static void
message_queue_dequeue_nothing_test (void **state)
{
    msgq_test_data_t *data = (msgq_test_data_t*)*state;
    MessageQueue *queue = data->queue;
    GObject *obj = NULL;

    obj = message_queue_timeout_dequeue (queue, 0);
    assert_null (obj);
}

int
main(int argc, char* argv[])
{
    const UnitTest tests[] = {
        unit_test (message_queue_allocate_test),
        unit_test_setup_teardown (message_queue_enqueue_dequeue_test,
                                  message_queue_setup,
                                  message_queue_teardown),
        unit_test_setup_teardown (message_queue_dequeue_order_test,
                                  message_queue_setup,
                                  message_queue_teardown),
        unit_test_setup_teardown (message_queue_dequeue_nothing_test,
                                  message_queue_setup,
                                  message_queue_teardown),
    };
    return run_tests(tests);
}
