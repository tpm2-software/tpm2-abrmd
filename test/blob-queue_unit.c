#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "blob-queue.h"

typedef struct blobq_test_data {
    blob_queue_t *queue;
} blobq_test_data_t;

static void
blob_queue_setup (void **state)
{
    blobq_test_data_t *data = NULL;

    data = calloc (1, sizeof (blobq_test_data_t));
    assert_non_null (data);
    data->queue = blob_queue_new ("test blob_queue_t");
    *state = data;
}

static void
blob_queue_teardown (void **state)
{
    blobq_test_data_t *data = (blobq_test_data_t*)*state;

    blob_queue_free (data->queue);
    free (data);
}

static void
blob_queue_allocate_test (void **state)
{
    blob_queue_setup (state);
    blob_queue_teardown (state);
}

static void
blob_queue_enqueue_dequeue_test (void **state)
{
    blobq_test_data_t *data = (blobq_test_data_t*)*state;
    blob_queue_t *queue = data->queue;
    blob_t *blob_in, *blob_out;

    blob_in = blob_new (NULL, NULL, 0);
    blob_queue_enqueue (queue, blob_in);
    blob_out = blob_queue_dequeue (queue);
    /* ptr != int but they're the same size usually? */
    assert_int_equal (blob_in, blob_out);
    blob_free (blob_in);
}

static void
blob_queue_dequeue_order_test (void **state)
{
    blobq_test_data_t *data = (blobq_test_data_t*)*state;
    blob_queue_t *queue = data->queue;
    blob_t *blob_0, *blob_1, *blob_2, *blob_tmp;

    blob_0 = blob_new (NULL, NULL, 0);
    blob_1 = blob_new (NULL, NULL, 0);
    blob_2 = blob_new (NULL, NULL, 0);

    blob_queue_enqueue (queue, blob_0);
    blob_queue_enqueue (queue, blob_1);
    blob_queue_enqueue (queue, blob_2);

    blob_tmp = blob_queue_dequeue (queue);
    assert_int_equal (blob_tmp, blob_0);
    blob_tmp = blob_queue_dequeue (queue);
    assert_int_equal (blob_tmp, blob_1);
    blob_tmp = blob_queue_dequeue (queue);
    assert_int_equal (blob_tmp, blob_2);
    blob_free (blob_0);
    blob_free (blob_1);
    blob_free (blob_2);
}

int
main(int argc, char* argv[])
{
    const UnitTest tests[] = {
        unit_test (blob_queue_allocate_test),
        unit_test_setup_teardown (blob_queue_enqueue_dequeue_test,
                                  blob_queue_setup,
                                  blob_queue_teardown),
        unit_test_setup_teardown (blob_queue_dequeue_order_test,
                                  blob_queue_setup,
                                  blob_queue_teardown),
    };
    return run_tests(tests);
}
