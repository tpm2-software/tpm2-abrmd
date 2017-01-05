#include <glib.h>
#include <stdlib.h>
#include <stdio.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tpm2-response.h"

#define HANDLE_TEST 0xdeadbeef
#define HANDLE_TYPE 0xde

typedef struct {
    Tpm2Response *response;
    guint8       *buffer;
    SessionData  *session;
} test_data_t;
/*
 * This function does all of the setup required to run a test *except*
 * for instantiating the Tpm2Response object.
 */
static void
tpm2_response_setup_base (void **state)
{
    test_data_t *data   = NULL;
    gint         fds[2] = { 0, };

    data = calloc (1, sizeof (test_data_t));
    /* allocate a buffer large enough to hold a TPM2 header and a handle */
    data->buffer   = calloc (1, TPM_RESPONSE_HEADER_SIZE + sizeof (TPM_HANDLE));
    data->session  = session_data_new (&fds[0], &fds[1], 0);

    *state = data;
}
/*
 * This function sets up all required test data with a Tpm2Response
 * object that has no attributes set.
 */
static void
tpm2_response_setup (void **state)
{
    test_data_t *data;

    tpm2_response_setup_base (state);
    data = (test_data_t*)*state;
    data->response = tpm2_response_new (data->session,
                                        data->buffer,
                                        (TPMA_CC){ 0, });
}
/*
 * This function sets up all required test data with a Tpm2Response
 * object that has attributes indicating the response has a handle
 * in it.
 */
static void
tpm2_response_setup_with_handle (void **state)
{
    test_data_t *data;
    TPMA_CC attributes = {
        .val = 1 << 28,
    };

    tpm2_response_setup_base (state);
    data = (test_data_t*)*state;
    data->response = tpm2_response_new (data->session,
                                        data->buffer,
                                        attributes);
    data->buffer [TPM_RESPONSE_HEADER_SIZE]     = 0xde;
    data->buffer [TPM_RESPONSE_HEADER_SIZE + 1] = 0xad;
    data->buffer [TPM_RESPONSE_HEADER_SIZE + 2] = 0xbe;
    data->buffer [TPM_RESPONSE_HEADER_SIZE + 3] = 0xef;
}
/**
 * Tear down all of the data from the setup function. We don't have to
 * free the data buffer (data->buffer) since the Tpm2Response frees it as
 * part of its finalize function.
 */
static void
tpm2_response_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_object_unref (data->session);
    g_object_unref (data->response);
    free (data);
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
 * In the setup function we save a reference to the SessionData object
 * instantiated as well as passing it into the Tpm2Response object. Here we
 * check to be sure that the SessionData object returned by the Tpm2Response
 * object is the same one that we passed it.
 */
static void
tpm2_response_get_session_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    SessionData *session;

    session = tpm2_response_get_session (data->response);
    assert_int_equal (data->session, session);
    g_object_unref (session);
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
 * Here we retrieve the data buffer from the Tpm2Resposne and manually set
 * the tag part of the response buffer to TPM_ST_SESSIONS in network byte
 * order (aka big endian). We then use the tpm2_response_get_tag function
 * to retrieve this data and we compare it to TPM_ST_SESSIONS to be sure we
 * got the value in host byte order.
 */
static void
tpm2_response_get_tag_test (void **state)
{
    test_data_t  *data   = (test_data_t*)*state;
    guint8       *buffer = tpm2_response_get_buffer (data->response);
    TPM_ST        tag_ret;

    /* this is tpm_st_sessions in network byte order */
    buffer[0] = 0x80;
    buffer[1] = 0x02;

    tag_ret = tpm2_response_get_tag (data->response);
    assert_int_equal (tag_ret, TPM_ST_SESSIONS);
}
/**
 * Again we're getting the response buffer from the Tpm2Resposne object
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

    /* this is tpm_st_sessions in network byte order */
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
    TPM_CC       response_code;

    /**
     * This is TPM_ST_SESSIONS + a size of 0x0a + the response code for
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
    assert_int_equal (response_code, TPM_RC_INITIALIZE);
}
/**
 * This is a setup function for testing the "short-cut" constructor for the
 * Tpm2Response object that creates the response buffer with the provided RC.
 */
static void
tpm2_response_new_rc_setup (void **state)
{
    test_data_t *data   = NULL;
    gint         fds[2] = { 0, };

    data = calloc (1, sizeof (test_data_t));
    /* allocate a buffer large enough to hold a TPM2 header */
    data->session  = session_data_new (&fds[0], &fds[1], 0);
    data->response = tpm2_response_new_rc (data->session, TPM_RC_BINDING);

    *state = data;
}
/**
 * The tpm2_response_new_rc sets the TPM_ST_NO_SESSIONS tag for us since
 * it's just returning an RC and cannot have sessions. Here we check to be
 * sure we can retrieve this tag and that's it's returned in host byte order.
 */
static void
tpm2_response_new_rc_tag_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM_ST       tag  = 0;

    tag = tpm2_response_get_tag (data->response);
    assert_int_equal (tag, TPM_ST_NO_SESSIONS);
}
/**
 * The tpm2_response_new_rc sets the size to the appropriate
 * TPM_RESONSE_HEADER size (which is 10 bytes) for us since the resonse
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
 * as a parameter. In the paried setup function we passed it TPM_RC_BINDING.
 * This function ensures that we get the same value back from the
 * tpm2_response_get_code function in the proper host byte order.
 */
static void
tpm2_response_new_rc_code_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM_RC       rc   = TPM_RC_SUCCESS;

    rc = tpm2_response_get_code (data->response);
    assert_int_equal (rc, TPM_RC_BINDING);
}
/**
 * The tpm2_resonse_new_rc takes a session as a parameter. We save a
 * reference to this in the test_data_t structure. Here we ensure that the
 * tpm2_response_get_session function returns the same session to us.
 */
static void
tpm2_response_new_rc_session_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    SessionData *session = data->session;

    assert_int_equal (session, tpm2_response_get_session (data->response));
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
    TPM_HANDLE handle;

    handle = tpm2_response_get_handle (data->response);
    assert_int_equal (handle, HANDLE_TEST);
}
/*
 * This tests the Tpm2Response objects ability to return the handle type
 * from the handle we set in the setup function.
 */
static void
tpm2_response_get_handle_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM_HT handle_type;

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
    TPM_HANDLE handle_in = 0x80fffffe, handle_out = 0;

    tpm2_response_set_handle (data->response, handle_in);
    handle_out = tpm2_response_get_handle (data->response);

    assert_int_equal (handle_in, handle_out);
}
gint
main (gint    argc,
      gchar  *argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (tpm2_response_type_test,
                                  tpm2_response_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_get_session_test,
                                  tpm2_response_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_get_buffer_test,
                                  tpm2_response_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_get_tag_test,
                                  tpm2_response_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_get_size_test,
                                  tpm2_response_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_get_code_test,
                                  tpm2_response_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_new_rc_tag_test,
                                  tpm2_response_new_rc_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_new_rc_size_test,
                                  tpm2_response_new_rc_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_new_rc_code_test,
                                  tpm2_response_new_rc_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_new_rc_session_test,
                                  tpm2_response_new_rc_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_no_handle_test,
                                  tpm2_response_setup,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_has_handle_test,
                                  tpm2_response_setup_with_handle,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_get_handle_test,
                                  tpm2_response_setup_with_handle,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_get_handle_type_test,
                                  tpm2_response_setup_with_handle,
                                  tpm2_response_teardown),
        unit_test_setup_teardown (tpm2_response_set_handle_test,
                                  tpm2_response_setup_with_handle,
                                  tpm2_response_teardown),
    };
    return run_tests (tests);
}
