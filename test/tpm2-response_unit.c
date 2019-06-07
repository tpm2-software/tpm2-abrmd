/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tpm2-header.h"
#include "tpm2-response.h"
#include "util.h"

#define HANDLE_TEST 0xdeadbeef
#define HANDLE_TYPE 0xde

typedef struct {
    Tpm2Response *response;
    guint8       *buffer;
    size_t        buffer_size;
    Connection   *connection;
} test_data_t;
/*
 * This function does all of the setup required to run a test *except*
 * for instantiating the Tpm2Response object.
 */
static int
tpm2_response_setup_base (void **state)
{
    test_data_t *data   = NULL;
    gint         client_fd;
    HandleMap   *handle_map;
    GIOStream   *iostream;

    data = calloc (1, sizeof (test_data_t));
    /* allocate a buffer large enough to hold a TPM2 header and a handle */
    data->buffer_size = TPM_RESPONSE_HEADER_SIZE + sizeof (TPM2_HANDLE);
    data->buffer   = calloc (1, data->buffer_size);
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    data->connection  = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);

    *state = data;
    return 0;
}
/*
 * This function sets up all required test data with a Tpm2Response
 * object that has no attributes set.
 */
static int
tpm2_response_setup (void **state)
{
    test_data_t *data;

    tpm2_response_setup_base (state);
    data = (test_data_t*)*state;
    data->response = tpm2_response_new (data->connection,
                                        data->buffer,
                                        data->buffer_size,
                                        (TPMA_CC){ 0, });
    return 0;
}
/*
 * This function sets up all required test data with a Tpm2Response
 * object that has attributes indicating the response has a handle
 * in it.
 */
static int
tpm2_response_setup_with_handle (void **state)
{
    test_data_t *data;
    TPMA_CC attributes = 1 << 28;

    tpm2_response_setup_base (state);
    data = (test_data_t*)*state;
    data->buffer_size = TPM_RESPONSE_HEADER_SIZE + sizeof (TPM2_HANDLE);
    data->response = tpm2_response_new (data->connection,
                                        data->buffer,
                                        data->buffer_size,
                                        attributes);
    set_response_size (data->buffer, data->buffer_size);
    data->buffer [TPM_RESPONSE_HEADER_SIZE]     = 0xde;
    data->buffer [TPM_RESPONSE_HEADER_SIZE + 1] = 0xad;
    data->buffer [TPM_RESPONSE_HEADER_SIZE + 2] = 0xbe;
    data->buffer [TPM_RESPONSE_HEADER_SIZE + 3] = 0xef;
    return 0;
}
/**
 * Tear down all of the data from the setup function. We don't have to
 * free the data buffer (data->buffer) since the Tpm2Response frees it as
 * part of its finalize function.
 */
static int
tpm2_response_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_object_unref (data->connection);
    g_object_unref (data->response);
    free (data);
    return 0;
}
/**
 * Here we check to be sure that the object instantiated in the setup
 * function is actually the type that we expect. This is a test to be sure
 * the type is registered properly with the type system.
 */
static void
tpm2_response_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (G_IS_OBJECT (data->response));
    assert_true (IS_TPM2_RESPONSE (data->response));
}
/**
 * In the setup function we save a reference to the Connection object
 * instantiated as well as passing it into the Tpm2Response object. Here we
 * check to be sure that the Connection object returned by the Tpm2Response
 * object is the same one that we passed it.
 */
static void
tpm2_response_get_connection_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Connection *connection;

    connection = tpm2_response_get_connection (data->response);
    assert_int_equal (data->connection, connection);
    g_object_unref (connection);
}
/**
 * In the setup function we passed the Tpm2Response object a data buffer.
 * Here we check to be sure it passes the same one back to us when we ask
 * for it.
 */
static void
tpm2_response_get_buffer_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (data->buffer, tpm2_response_get_buffer (data->response));
}
/**
 * Here we retrieve the data buffer from the Tpm2Response and manually set
 * the tag part of the response buffer to TPM2_ST_SESSIONS in network byte
 * order (aka big endian). We then use the tpm2_response_get_tag function
 * to retrieve this data and we compare it to TPM2_ST_SESSIONS to be sure we
 * got the value in host byte order.
 */
static void
tpm2_response_get_tag_test (void **state)
{
    test_data_t  *data   = (test_data_t*)*state;
    guint8       *buffer = tpm2_response_get_buffer (data->response);
    TPM2_ST        tag_ret;

    /* this is TPM2_ST_SESSIONS in network byte order */
    buffer[0] = 0x80;
    buffer[1] = 0x02;

    tag_ret = tpm2_response_get_tag (data->response);
    assert_int_equal (tag_ret, TPM2_ST_SESSIONS);
}
/**
 * Again we're getting the response buffer from the Tpm2Response object
 * but this time we're setting the additional size field (uint32) to 0x6
 * in big endian format. We then use the tpm2_response_get_size function
 * to get this value back and we compare it to 0x6 to be sure it's returned
 * in host byte order.
 */
static void
tpm2_response_get_size_test (void **state)
{
    test_data_t *data     = (test_data_t*)*state;
    guint8      *buffer   = tpm2_response_get_buffer (data->response);
    guint32      size_ret = 0;

    /* this is TPM2_ST_SESSIONS in network byte order */
    buffer[0] = 0x80;
    buffer[1] = 0x02;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x06;

    size_ret = tpm2_response_get_size (data->response);
    assert_int_equal (0x6, size_ret);
}
/**
 * Again we're getting the response buffer from the Tpm2Response object
 * but this time we're setting the additional response code field (TPM_RC)
 * to a known value in big endian format. We then use tpm2_response_get_code
 * to retrieve the RC and compare it to the appropriate constant to be sure
 * it's sent back to us in host byte order.
 */
static void
tpm2_response_get_code_test (void **state)
{
    test_data_t *data     = (test_data_t*)*state;
    guint8      *buffer   = tpm2_response_get_buffer (data->response);
    TPM2_CC       response_code;

    /**
     * This is TPM2_ST_SESSIONS + a size of 0x0a + the response code for
     * GetCapability in network byte order
     */
    buffer[0] = 0x80;
    buffer[1] = 0x02;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x0a;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x01;
    buffer[9] = 0x00;

    response_code = tpm2_response_get_code (data->response);
    assert_int_equal (response_code, TPM2_RC_INITIALIZE);
}
/**
 * This is a setup function for testing the "short-cut" constructor for the
 * Tpm2Response object that creates the response buffer with the provided RC.
 */
static int
tpm2_response_new_rc_setup (void **state)
{
    test_data_t *data   = NULL;
    gint         client_fd;
    GIOStream   *iostream;
    HandleMap   *handle_map;

    data = calloc (1, sizeof (test_data_t));
    /* allocate a buffer large enough to hold a TPM2 header */
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    data->connection  = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    data->response = tpm2_response_new_rc (data->connection, TPM2_RC_BINDING);

    *state = data;
    return 0;
}
/**
 * The tpm2_response_new_rc sets the TPM2_ST_NO_SESSIONS tag for us since
 * it's just returning an RC and cannot have connections. Here we check to be
 * sure we can retrieve this tag and that's it's returned in host byte order.
 */
static void
tpm2_response_new_rc_tag_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_ST       tag  = 0;

    tag = tpm2_response_get_tag (data->response);
    assert_int_equal (tag, TPM2_ST_NO_SESSIONS);
}
/**
 * The tpm2_response_new_rc sets the size to the appropriate
 * TPM_RESPONSE_HEADER size (which is 10 bytes) for us since the response
 * buffer is just a header. Here we check to be sure that we get this value
 * back in the proper host byte order.
 */
static void
tpm2_response_new_rc_size_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    guint32      size = 0;

    size = tpm2_response_get_size (data->response);
    assert_int_equal (size, TPM_RESPONSE_HEADER_SIZE);
}
/**
 * The tpm2_response_new_rc sets the response code to whatever RC is passed
 * as a parameter. In the paired setup function we passed it TPM2_RC_BINDING.
 * This function ensures that we get the same value back from the
 * tpm2_response_get_code function in the proper host byte order.
 */
static void
tpm2_response_new_rc_code_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC       rc   = TPM2_RC_SUCCESS;

    rc = tpm2_response_get_code (data->response);
    assert_int_equal (rc, TPM2_RC_BINDING);
}
/**
 * The tpm2_response_new_rc takes a connection as a parameter. We save a
 * reference to this in the test_data_t structure. Here we ensure that the
 * tpm2_response_get_connection function returns the same connection to us.
 */
static void
tpm2_response_new_rc_connection_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Connection *connection = data->connection;

    assert_int_equal (connection, tpm2_response_get_connection (data->response));
}
/*
 * This test ensures that a tpm2_response_has_handle reports the
 * actual value in the TPMA_CC attributes field when the rHandle bit
 * in the attributes is *not* set.
 */
static void
tpm2_response_no_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    gboolean has_handle;

    has_handle = tpm2_response_has_handle (data->response);
    assert_false (has_handle);
}
/*
 * This test ensures that a tpm2_response_has_handle reports the
 * actual value in the TPMA_CC attributes field when the rHandle bit
 * in the attributes *is* set.
 */
static void
tpm2_response_has_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    gboolean has_handle = FALSE;

    has_handle = tpm2_response_has_handle (data->response);
    assert_true (has_handle);
}
/*
 * This tests the Tpm2Response objects ability to return the handle that
 * we set in the setup function.
 */
static void
tpm2_response_get_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE handle;

    handle = tpm2_response_get_handle (data->response);
    assert_int_equal (handle, HANDLE_TEST);
}
/*
 * This tests the Tpm2Response object's ability detect an attempt to read
 * beyond the end of its internal buffer. We fake this by changing the
 * buffer_size field manually and then trying to access the handle.
 */
static void
tpm2_response_get_handle_no_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE handle;

    data->response->buffer_size = TPM_HEADER_SIZE;
    handle = tpm2_response_get_handle (data->response);
    assert_int_equal (handle, 0);
}
/*
 * This tests the Tpm2Response objects ability to return the handle type
 * from the handle we set in the setup function.
 */
static void
tpm2_response_get_handle_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HT handle_type;

    handle_type = tpm2_response_get_handle_type (data->response);
    assert_int_equal (handle_type, HANDLE_TYPE);
}
/*
 * This tests the Tpm2Response objects ability to set the response handle
 * field. We do this by first setting the handle to some value and then
 * querying the Tpm2Response object for the handle. The value returned
 * should be the handle that we set in the previous call.
 */
static void
tpm2_response_set_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE handle_in = 0x80fffffe, handle_out = 0;

    tpm2_response_set_handle (data->response, handle_in);
    handle_out = tpm2_response_get_handle (data->response);

    assert_int_equal (handle_in, handle_out);
}
/*
 * This tests the Tpm2Response object's ability detect an attempt to write
 * beyond the end of its internal buffer. We fake this by changing the
 * buffer_size field manually and then trying to set the handle.
 */
static void
tpm2_response_set_handle_no_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE handle_in = 0x80fffffe, handle_out = 0;

    data->response->buffer_size = TPM_HEADER_SIZE;
    tpm2_response_set_handle (data->response, handle_in);
    handle_out = tpm2_response_get_handle (data->response);

    assert_int_equal (handle_out, 0);
}
gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (tpm2_response_type_test,
                                         tpm2_response_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_get_connection_test,
                                         tpm2_response_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_get_buffer_test,
                                         tpm2_response_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_get_tag_test,
                                         tpm2_response_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_get_size_test,
                                         tpm2_response_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_get_code_test,
                                         tpm2_response_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_new_rc_tag_test,
                                         tpm2_response_new_rc_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_new_rc_size_test,
                                         tpm2_response_new_rc_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_new_rc_code_test,
                                         tpm2_response_new_rc_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_new_rc_connection_test,
                                         tpm2_response_new_rc_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_no_handle_test,
                                         tpm2_response_setup,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_has_handle_test,
                                         tpm2_response_setup_with_handle,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_get_handle_test,
                                         tpm2_response_setup_with_handle,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_get_handle_no_handle_test,
                                         tpm2_response_setup_with_handle,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_get_handle_type_test,
                                         tpm2_response_setup_with_handle,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_set_handle_test,
                                         tpm2_response_setup_with_handle,
                                         tpm2_response_teardown),
        cmocka_unit_test_setup_teardown (tpm2_response_set_handle_no_handle_test,
                                         tpm2_response_setup_with_handle,
                                         tpm2_response_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
