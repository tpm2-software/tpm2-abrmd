/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <pthread.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "message-queue.h"
#include "connection.h"
#include "control-message.h"
#include "util.h"

typedef struct msgq_test_data {
    MessageQueue *queue;
} msgq_test_data_t;

static int
message_queue_setup (void **state)
{
    msgq_test_data_t *data = NULL;

    data = calloc (1, sizeof (msgq_test_data_t));
    assert_non_null (data);
    data->queue = message_queue_new ();
    *state = data;
    return 0;
}

static int
message_queue_teardown (void **state)
{
    msgq_test_data_t *data = (msgq_test_data_t*)*state;

    g_object_unref (data->queue);
    free (data);
    return 0;
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
    Connection  *obj_in, *obj_out;
    HandleMap    *handle_map;
    gint          client_fd;
    GIOStream *iostream;

    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    obj_in = connection_new (iostream, 0, handle_map);
    message_queue_enqueue (queue, G_OBJECT (obj_in));
    obj_out = CONNECTION (message_queue_dequeue (queue));
    /* ptr != int but they're the same size usually? */
    assert_int_equal (obj_in, obj_out);
    g_object_unref (handle_map);
    g_object_unref (iostream);
}

static void
message_queue_dequeue_order_test (void **state)
{
    msgq_test_data_t *data = (msgq_test_data_t*)*state;
    HandleMap    *map_0, *map_1, *map_2;
    MessageQueue *queue = data->queue;
    Connection *obj_0, *obj_1, *obj_2, *obj_tmp;
    gint client_fd;
    GIOStream *iostream;

    map_0 = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    map_1 = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    map_2 = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);

    iostream = create_connection_iostream (&client_fd);
    obj_0 = connection_new (iostream, 0, map_0);
    g_object_unref (iostream);
    iostream = create_connection_iostream (&client_fd);
    obj_1 = connection_new (iostream, 0, map_1);
    g_object_unref (iostream);
    iostream = create_connection_iostream (&client_fd);
    obj_2 = connection_new (iostream, 0, map_2);
    g_object_unref (iostream);

    message_queue_enqueue (queue, G_OBJECT (obj_0));
    message_queue_enqueue (queue, G_OBJECT (obj_1));
    message_queue_enqueue (queue, G_OBJECT (obj_2));

    obj_tmp = CONNECTION (message_queue_dequeue (queue));
    assert_int_equal (obj_tmp, obj_0);
    obj_tmp = CONNECTION (message_queue_dequeue (queue));
    assert_int_equal (obj_tmp, obj_1);
    obj_tmp = CONNECTION (message_queue_dequeue (queue));
    assert_int_equal (obj_tmp, obj_2);
    g_object_unref (map_0);
    g_object_unref (map_1);
    g_object_unref (map_2);
    g_object_unref (obj_0);
    g_object_unref (obj_1);
    g_object_unref (obj_2);
}
/*
 * This function is used in the thread_unblock_test function as the thread
 * that blocks on the MessageQueue waiting for a message.
 */
void*
thread_func (void* arg)
{
    MessageQueue *queue = MESSAGE_QUEUE (arg);
    GObject *obj;

    obj = message_queue_dequeue (queue);
    assert_true (CONTROL_MESSAGE (obj));
    g_object_unref (obj);

    return NULL;
}

static void
message_queue_thread_unblock_test (void **state)
{
    msgq_test_data_t *data = (msgq_test_data_t*)*state;
    ControlMessage *msg = control_message_new (CHECK_CANCEL);
    pthread_t thread_id;
    int ret;

    ret = pthread_create (&thread_id,
                          NULL,
                          thread_func,
                          data->queue);
    assert_int_equal (ret, 0);
    message_queue_enqueue (data->queue, G_OBJECT (msg));
    g_object_unref (msg);
    ret = pthread_join (thread_id, NULL);
    assert_int_equal (ret, 0);
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test (message_queue_allocate_test),
        cmocka_unit_test_setup_teardown (message_queue_enqueue_dequeue_test,
                                         message_queue_setup,
                                         message_queue_teardown),
        cmocka_unit_test_setup_teardown (message_queue_dequeue_order_test,
                                         message_queue_setup,
                                         message_queue_teardown),
        cmocka_unit_test_setup_teardown (message_queue_thread_unblock_test,
                                         message_queue_setup,
                                         message_queue_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
