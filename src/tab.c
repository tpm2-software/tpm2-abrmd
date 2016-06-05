#include <errno.h>

#include "message-queue.h"
#include "tab.h"

#define TAB_TIMEOUT_DEQUEUE 1e6

gpointer
cmd_runner (gpointer data)
{
    tab_t  *tab  = (tab_t*)data;
    GObject     *obj = NULL;

    g_debug ("cmd_runner start");
    while (TRUE) {
        /* To join this thread cleanly we can't block on the MessageQueue
         * indefinitely. Waking up every second may be bad for battery
         * though. Other options are a bit ugly though:
         * Exit with this thread blocked and let the system recover the
         * resources we leave behind. This will make reconfiguring the daemon
         * dynamically difficult though I don't know we'll ever have to do
         * that.
         * Have the tab put a dummy blob in the queue to wake up the thread
         * to get it to the cancelation point.
         * This last one is probably the best option.
         */
        g_debug ("tab_cmd_runner: message_queue_timeout_dequeue: 0x%x for %e",
                 tab->in_queue,
                 TAB_TIMEOUT_DEQUEUE);
        obj = message_queue_timeout_dequeue (tab->in_queue, TAB_TIMEOUT_DEQUEUE);
        g_debug ("cmd_runner message_queue_timeout_dequeue got obj: 0x%x", obj);
        if (obj != NULL) {
       /*
        tss2_tcti_transmit (tab->tcti_context, size, blob);
        tss2_tcti_receive (tab->tcti_context, size, blob, timeout);
        */
            g_debug ("message_queue_enqueue: 0x%x, object", tab->out_queue, obj);
            message_queue_enqueue (tab->out_queue, obj);
        }
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

    if (tcti_context != NULL)
        g_error ("tab_new passed NON-NULL TSS2_TCTI_CONTEXT pointer, we don't do anything with the TCTI yet.");
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

    if (tab == NULL)
        g_error ("tab_cancel passed NULL tab_t");
    if (tab->thread == 0)
        g_error ("tab_t not running, cannot cancel");

    return pthread_cancel (tab->thread);
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
    g_debug ("tab_send_command: tab_t: 0x%x DataMessage: 0x%x", tab, obj);
    message_queue_enqueue (tab->in_queue, obj);
}
/** Get the next response from the TAB.
 * Block for timeout microseconds waiting for the next resonse to a TPM
 * command. The caller is responsible for freeing the returned buffer.
 */
GObject*
tab_get_timeout_response (tab_t    *tab,
                          guint64  timeout)
{
    g_debug ("tab_get_timeout_response: tab_t: 0x%x", tab);
    return message_queue_timeout_dequeue (tab->out_queue, timeout);
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
