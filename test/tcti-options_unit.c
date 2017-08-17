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

#include <setjmp.h>
#include <cmocka.h>

#ifdef HAVE_TCTI_DEVICE
#include "tcti-device.h"
#endif
#ifdef HAVE_TCTI_SOCKET
#include "tcti-socket.h"
#endif
#include "tcti-options.h"

/**
 * Very simple setup / teardown functions to instantiate a TctiOptions
 * object and unref it.
 */
static int
tcti_options_setup (void **state)
{
    *state = tcti_options_new ();
    return 0;
}
static int
tcti_options_teardown (void **state)
{
    TctiOptions *tcti_options = TCTI_OPTIONS (*state);

    g_object_unref (tcti_options);
    return 0;
}
/**
 * Test the object lifecycle. The setup /teardown functions must be invoked
 * for this to be a valid test. This is mostly useful when combined with
 * valgrind.
 */
static void
tcti_options_new_unref_test (void **state)
{
    TctiOptions *tcti_opts = TCTI_OPTIONS (*state);

    assert_non_null (tcti_opts);
}
/**
 * Tests to ensure that the default values for the TCTI specific
 * configuration options are set properly. These test are conditional
 * and should only be invoked if the TCTI is enalbed by ./configure.
 */
#ifdef HAVE_TCTI_DEVICE
static void
tcti_options_defaults_device_test (void **state)
{
    TctiOptions *tcti_options = TCTI_OPTIONS (*state);

    assert_string_equal (tcti_options->device_name, TCTI_DEVICE_DEFAULT_FILE);
}
#endif
#ifdef HAVE_TCTI_SOCKET
static void
tcti_options_defaults_socket_address_test (void **state)
{
    TctiOptions *tcti_options = TCTI_OPTIONS (*state);

    assert_string_equal (tcti_options->socket_address,
                         TCTI_SOCKET_DEFAULT_HOST);
}
static void
tcti_options_defaults_socket_port_test (void **state)
{
    TctiOptions *tcti_options = TCTI_OPTIONS (*state);

    assert_int_equal (tcti_options->socket_port,
                      TCTI_SOCKET_DEFAULT_PORT);
}
#endif

gint
main (gint     argc,
      gchar   *argv[])
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (tcti_options_new_unref_test,
                                         tcti_options_setup,
                                         tcti_options_teardown),
#ifdef HAVE_TCTI_DEVICE
        cmocka_unit_test_setup_teardown (tcti_options_defaults_device_test,
                                         tcti_options_setup,
                                         tcti_options_teardown),
#endif
#ifdef HAVE_TCTI_SOCKET
        cmocka_unit_test_setup_teardown (tcti_options_defaults_socket_address_test,
                                         tcti_options_setup,
                                         tcti_options_teardown),
        cmocka_unit_test_setup_teardown (tcti_options_defaults_socket_port_test,
                                         tcti_options_setup,
                                         tcti_options_teardown),
#endif
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
