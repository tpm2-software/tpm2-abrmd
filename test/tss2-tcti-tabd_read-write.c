#include <glib.h>
#include <tss2-tcti-tabd.h>
#include <unistd.h>

int
main (void)
{
    TSS2_TCTI_CONTEXT *tcti_context = NULL;
    TSS2_RC ret;
    size_t context_size;

    ret = tss2_tcti_tabd_init (NULL, &context_size);
    if (ret != TSS2_RC_SUCCESS)
        g_error ("failed to get size of tcti context");
    g_debug ("tcti size is: %d", context_size);
    tcti_context = calloc (1, context_size);
    if (tcti_context == NULL)
        g_error ("failed to allocate memory for tcti context");
    g_debug ("context structure allocated successfully");
    ret = tss2_tcti_tabd_init (tcti_context, NULL);
    if (ret != TSS2_RC_SUCCESS)
        g_error ("failed to initialize tcti context");
    g_debug ("initialized");
    /* Wait for initialization thread in the TCTI to setup a connection to the
     * tabd. The right thing to do is for the TCTI to cause callers to block
     * on a mutex till the setup thread is done.
     */
    sleep (5);
    g_debug ("transmitting ...");
    /* send / receive */
    char *xmit_str = "test";
    ret = tss2_tcti_transmit (tcti_context, strlen (xmit_str), (uint8_t*)xmit_str);
    if (ret != TSS2_RC_SUCCESS)
        g_debug ("tss2_tcti_transmit failed");
    char recv_str[10] = {0};
    size_t recv_size = 10;
    ret = tss2_tcti_receive (tcti_context, &recv_size, recv_str, 0);
    if (ret != TSS2_RC_SUCCESS)
        g_debug ("tss2_tcti_receive failed");
    g_debug ("received string: %s", recv_str);
}
