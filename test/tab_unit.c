/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <stdlib.h>
#include <tpm20.h>

#include <setjmp.h>
#include <cmocka.h>

#include "source-interface.h"
#include "response-sink.h"
#include "tcti-echo.h"
#include "tab.h"

static void
tab_allocate_deallocate_test (void **state)
{
    Tcti *tcti;
    Tab  *tab;
    ResponseSink *sink;

    sink = response_sink_new ();
    tcti = TCTI (tcti_echo_new (TCTI_ECHO_MIN_BUF));
    tab  = tab_new (tcti);
    source_add_sink (SOURCE (tab), SINK (sink));
    g_object_unref (tab);
}
static void
tab_start_stop_test_setup (void **state)
{
    Tcti *tcti;
    Tab  *tab;
    ResponseSink *sink;

    sink = response_sink_new ();
    tcti = TCTI (tcti_echo_new (TCTI_ECHO_MIN_BUF));
    tab  = tab_new (tcti);
    source_add_sink (SOURCE (tab), SINK (sink));

    *state = tab;
}
static void
tab_start_stop_test_teardown (void **state)
{
    Tab *tab = *state;

    tab_cancel (tab);
    tab_join (tab);
    g_object_unref (tab);
}
static void
tab_start_stop_test (void **state)
{
    Tab *tab = *state;
    tab_start (tab);
}
static void
tab_add_no_remove_test_setup (void **state)
{
    Tcti *tcti;
    Tab  *tab  = *state;
    ResponseSink *sink;

    sink = response_sink_new ();
    tcti = TCTI (tcti_echo_new (TCTI_ECHO_MIN_BUF));
    tab = tab_new (tcti);
    source_add_sink (SOURCE (tab), SINK (sink));
    tab_start (tab);
    *state = tab;
}
/* This test adds a message to the tab but doesn't remove it.
 * The tab is expected to free this message as part of its destruction.
 */
static void
tab_add_no_remove_test (void **state)
{
    DataMessage *msg = NULL;
    Tab         *tab = *state;

    msg = data_message_new (NULL, NULL, 0);
    tab_enqueue (tab, G_OBJECT (msg));
    g_object_unref (msg);
}
gint
main (void)
{
    const UnitTest tests[] = {
        unit_test (tab_allocate_deallocate_test),
        unit_test_setup_teardown (tab_start_stop_test,
                                  tab_start_stop_test_setup,
                                  tab_start_stop_test_teardown),
        unit_test_setup_teardown (tab_add_no_remove_test,
                                  tab_add_no_remove_test_setup,
                                  tab_start_stop_test_teardown),
    };
    return run_tests (tests);
}
