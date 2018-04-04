/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
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
