#include <errno.h>

#include "control-message.h"
#include "message-queue.h"
#include "tab.h"

#define TAB_TIMEOUT_DEQUEUE 1e6

ssize_t
tab_process_data_message (tab_t        *tab,
                          DataMessage  *msg)
{
    TSS2_RC rc;

    g_debug ("tab process_data_message: 0x%x, object", tab->out_queue, msg);
    rc = tss2_tcti_transmit (tab->tcti_context, msg->size, msg->data);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("tss2_tcti_transmit returned error: 0x%x", rc);
    rc = tss2_tcti_receive (tab->tcti_context, &msg->size, msg->data, 0);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("tss2_tcti_receive returned error: 0x%x", rc);

    message_queue_enqueue (tab->out_queue, G_OBJECT (msg));
}

gpointer
cmd_runner (gpointer data)
{
    tab_t  *tab  = (tab_t*)data;
    GObject     *obj = NULL;

    g_debug ("cmd_runner start");
    while (TRUE) {
        obj = message_queue_dequeue (tab->in_queue);
        g_debug ("cmd_runner message_queue_dequeue got obj: 0x%x", obj);
        if (IS_DATA_MESSAGE (obj))
            tab_process_data_message (tab, DATA_MESSAGE (obj));
        if (IS_CONTROL_MESSAGE (obj))
            process_control_message (CONTROL_MESSAGE (obj));
        if (obj == NULL)
            g_debug ("cmd_runner: dequeued a null object");
        pthread_testcancel ();
    }
}

/** Create new TPM access broker (TAB) object
 */
tab_t*
tab_new (TSS2_TCTI_CONTEXT *tcti_context)
{
    tab_t *tab = NULL;
    gint ret = 0;

    if (tcti_context == NULL)
        g_error ("tab_new passed NULL TSS2_TCTI_CONTEXT");
    tab = calloc (1, sizeof (tab_t));
    tab->in_queue  = message_queue_new ("TAB in queue");
    tab->out_queue = message_queue_new ("TAB out queue");
    tab->tcti_context = tcti_context;

    return tab;
}
/** Bring down the TAB as gracefully as we can.
 */
void
tab_free (tab_t *tab)
{
    gint ret = 0;

    g_debug ("tab_free: 0x%x", tab);
    if (tab == NULL)
        g_error ("tab_free passed NULL tab_t pointer");
    if (tab->thread != 0)
        g_error ("tab_free called with thread running, cancel thread first");
    g_object_unref (tab->in_queue);
    g_object_unref (tab->out_queue);
    if (tab->tcti_context)
        tss2_tcti_finalize (tab->tcti_context);
    free (tab);
}
gint
tab_start (tab_t *tab)
{
    if (tab == NULL)
        g_error ("tab_start passed NULL tab_t");
    if (tab->thread != 0)
        g_error ("tab_t cmd_runner thread already running");
    return  pthread_create (&tab->thread, NULL, cmd_runner, tab);
}
gint
tab_cancel (tab_t *tab)
{
    gint ret;
    ControlMessage *msg;

    if (tab == NULL)
        g_error ("tab_cancel passed NULL tab_t");
    if (tab->thread == 0)
        g_error ("tab_t not running, cannot cancel");
    /* cancel our internal thread before we unblock it */
    ret = pthread_cancel (tab->thread);
    /* Let anything blocked on the in_queue know that they should check to
     * see if they need to cancel too.
     */
    msg = control_message_new (CHECK_CANCEL);
    g_debug ("tab_cancel: enqueuing ControlMessage: 0x%x", msg);
    message_queue_enqueue (tab->in_queue, G_OBJECT (msg));
    /* Same for the out_queue. */
    msg = control_message_new (CHECK_CANCEL);
    g_debug ("tab_cancel: enqueuing ControlMessage: 0x%x", msg);
    message_queue_enqueue (tab->out_queue, G_OBJECT (msg));

    return ret;
}
gint
tab_join (tab_t *tab)
{
    gint ret;

    if (tab == NULL)
        g_error ("tab_join passed NULL tab_t");
    if (tab->thread == 0)
        g_error ("tab_t not running, cannot join");
    ret = pthread_join (tab->thread, NULL);
    tab->thread = 0;

    return ret;
}
/** Send a command to the TAB.
 * The command originated with the session-data object provided. The command
 * itself is in the cmd_buf parameter. If successfully added to the tab queue
 * we return true, an error will return false.
 */
void
tab_send_command (tab_t       *tab,
                  GObject     *obj)
{
    g_debug ("tab_send_command: tab_t: 0x%x obj: 0x%x", tab, obj);
    /* The TAB takes ownership of this object */
    g_object_ref (obj);
    message_queue_enqueue (tab->in_queue, obj);
}
/** Get the next response from the TAB.
 * Block for timeout microseconds waiting for the next resonse to a TPM
 * command. The caller is responsible for freeing the returned buffer.
 * The returned object is owned by the caller, we do not unref it.
 */
GObject*
tab_get_timeout_response (tab_t    *tab,
                          guint64  timeout)
{
    g_debug ("tab_get_timeout_response: tab_t: 0x%x", tab);
    return message_queue_timeout_dequeue (tab->out_queue, timeout);
}
/** Get the next response from this TAB.
 * Block indefinitely waiting for a message to come from the TAB.
 */
GObject*
tab_get_response (tab_t *tab)
{
    g_debug ("tab_get_response: tab_t: 0x%x", tab);
    return message_queue_dequeue (tab->out_queue);
}
/** Cancel pending commands for a session in the TAB
 * Cancel each pending command associated with the given session in the TAB
 * provided. The number of commands canceled is returned to the caller. If
 * there are no pending commands, 0 is returned. -1 is returned when an error
 * occurs.
 */
gint
tab_cancel_commands (tab_t          *tab,
                     session_data_t *session)
{
    g_error ("tab_cancel_commands: unimplemented");
    return 0;
}
