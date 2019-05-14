/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "msg.h"
#include "session-data.h"

static void
check_cancel_msg_allocate_test (void **state)
{
    check_cancel_msg_t *msg;

    msg = check_cancel_msg_new ();
    assert_non_null (msg);
    assert_int_equal (MSG_TYPE (msg), CHECK_CANCEL_MSG_TYPE);
    check_cancel_msg_free (msg);
}

static void
data_msg_allocate_allnull_test (void **state)
{
    data_msg_t *msg = data_msg_new (NULL, NULL, 0);
    assert_non_null (msg);
    data_msg_free (msg);
}

gint
main(void)
{
    const UnitTest tests[] = {
        unit_test (check_cancel_msg_allocate_test),
        unit_test (data_msg_allocate_allnull_test),
    };
    return run_tests(tests);
}
