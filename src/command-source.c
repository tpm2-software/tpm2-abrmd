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
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "connection-manager.h"
#include "command-source.h"
#include "source-interface.h"
#include "tpm2-command.h"
#include "tpm2-header.h"
#include "util.h"

#define WAKEUP_DATA "hi"
#define WAKEUP_SIZE 2

enum {
    PROP_0,
    PROP_COMMAND_ATTRS,
    PROP_CONNECTION_MANAGER,
    PROP_SINK,
    PROP_WAKEUP_RECEIVE_FD,
    PROP_WAKEUP_SEND_FD,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
void* command_source_thread (void *data);

/**
 * Function implementing the Source interface. Adds a sink for the source
 * to pass data to.
 */
void
command_source_add_sink (Source      *self,
                         Sink        *sink)
{
    CommandSource *src = COMMAND_SOURCE (self);
    GValue value = G_VALUE_INIT;

    g_debug ("command_soruce_add_sink: CommandSource: 0x%" PRIxPTR
             " , Sink: 0x%" PRIxPTR, (uintptr_t)src, (uintptr_t)sink);
    g_value_init (&value, G_TYPE_OBJECT);
    g_value_set_object (&value, sink);
    g_object_set_property (G_OBJECT (src), "sink", &value);
    g_value_unset (&value);
}

static void
command_source_source_interface_init (gpointer g_iface)
{
    SourceInterface *source = (SourceInterface*)g_iface;
    source->add_sink = command_source_add_sink;
}

static void
command_source_init (CommandSource *source)
{ /* noop */ }

G_DEFINE_TYPE_WITH_CODE (
    CommandSource,
    command_source,
    TYPE_THREAD,
    G_IMPLEMENT_INTERFACE (TYPE_SOURCE,
                           command_source_source_interface_init)
    );

static void
command_source_set_property (GObject       *object,
                              guint          property_id,
                              GValue const  *value,
                              GParamSpec    *pspec)
{
    CommandSource *self = COMMAND_SOURCE (object);

    g_debug ("command_source_set_properties: 0x%" PRIxPTR, (uintptr_t)self);
    switch (property_id) {
    case PROP_COMMAND_ATTRS:
        self->command_attrs = COMMAND_ATTRS (g_value_dup_object (value));
        g_debug ("  command_attrs: 0x%" PRIxPTR, (uintptr_t)self->command_attrs);
        break;
    case PROP_CONNECTION_MANAGER:
        self->connection_manager = CONNECTION_MANAGER (g_value_get_object (value));
        break;
    case PROP_SINK:
        /* be rigid intially, add flexiblity later if we need it */
        if (self->sink != NULL) {
            g_warning ("  sink already set");
            break;
        }
        self->sink = SINK (g_value_get_object (value));
        g_object_ref (self->sink);
        g_debug ("  sink: 0x%" PRIxPTR, (uintptr_t)self->sink);
        break;
    case PROP_WAKEUP_RECEIVE_FD:
        self->wakeup_receive_fd = g_value_get_int (value);
        FD_SET (self->wakeup_receive_fd, &self->receive_fdset);
        if (self->wakeup_receive_fd > self->maxfd)
            self->maxfd = self->wakeup_receive_fd;
        g_debug ("  wakeup_receive_fd: %d", self->wakeup_receive_fd);
        break;
    case PROP_WAKEUP_SEND_FD:
        self->wakeup_send_fd = g_value_get_int (value);
        g_debug ("  wakeup_send_fd: %d", self->wakeup_send_fd);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
command_source_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
    CommandSource *self = COMMAND_SOURCE (object);

    g_debug ("command_source_get_properties: 0x%" PRIxPTR, (uintptr_t)self);
    switch (property_id) {
    case PROP_COMMAND_ATTRS:
        g_value_set_object (value, self->command_attrs);
        break;
    case PROP_CONNECTION_MANAGER:
        g_value_set_object (value, self->connection_manager);
        break;
    case PROP_SINK:
        g_value_set_object (value, self->sink);
        break;
    case PROP_WAKEUP_RECEIVE_FD:
        g_value_set_int (value, self->wakeup_receive_fd);
        break;
    case PROP_WAKEUP_SEND_FD:
        g_value_set_int (value, self->wakeup_send_fd);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
gint
command_source_on_new_connection (ConnectionManager   *connection_manager,
                                  Connection          *connection,
                                  CommandSource       *command_source)
{
    ssize_t ret;
    int new_fd = connection_fd (connection);

    g_info ("command_source_on_new_connection: adding new client fd: %d",
            new_fd);
    FD_SET (new_fd, &command_source->receive_fdset);
    if (new_fd > command_source->maxfd)
        command_source->maxfd = new_fd;
    g_debug ("command_source_on_new_connection: writing \"%s\" to fd: %d",
             WAKEUP_DATA, command_source->wakeup_send_fd);
    ret = write (command_source->wakeup_send_fd, WAKEUP_DATA, WAKEUP_SIZE);
    if (ret < 1)
        g_error ("failed to send wakeup signal");

    return 0;
}

/* Not doing any sanity checks here. Be sure to shut things downon your own
 * first.
 */
static void
command_source_finalize (GObject  *object)
{
    CommandSource *source = COMMAND_SOURCE (object);

    if (source->sink)
        g_object_unref (source->sink);
    if (source->connection_manager)
        g_object_unref (source->connection_manager);
    if (source->command_attrs) {
        g_object_unref (source->command_attrs);
        source->command_attrs = NULL;
    }
    close (source->wakeup_send_fd);
    close (source->wakeup_receive_fd);
    if (command_source_parent_class)
        G_OBJECT_CLASS (command_source_parent_class)->finalize (object);
}

static void
command_source_unblock (Thread *self)
{
    pthread_cancel (self->thread_id);
}

static void
command_source_class_init (CommandSourceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    ThreadClass  *thread_class = THREAD_CLASS (klass);

    g_debug ("command_source_class_init");
    if (command_source_parent_class == NULL)
        command_source_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = command_source_finalize;
    object_class->get_property = command_source_get_property;
    object_class->set_property = command_source_set_property;
    thread_class->thread_run   = command_source_thread;
    thread_class->thread_unblock = command_source_unblock;

    obj_properties [PROP_COMMAND_ATTRS] =
        g_param_spec_object ("command-attrs",
                             "CommandAttrs object",
                             "CommandAttrs instance.",
                             TYPE_COMMAND_ATTRS,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_CONNECTION_MANAGER] =
        g_param_spec_object ("connection-manager",
                             "ConnectionManager object",
                             "ConnectionManager instance.",
                             TYPE_CONNECTION_MANAGER,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SINK] =
        g_param_spec_object ("sink",
                             "Sink",
                             "Reference to a Sink object.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE);
    obj_properties [PROP_WAKEUP_RECEIVE_FD] =
        g_param_spec_int ("wakeup-receive-fd",
                          "wakeup receive file descriptor",
                          "File descriptor to receive wakeup signal.",
                          0,
                          INT_MAX,
                          0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_WAKEUP_SEND_FD] =
        g_param_spec_int ("wakeup-send-fd",
                          "wakeup send file descriptor",
                          "File descriptor to send wakeup signal.",
                          0,
                          INT_MAX,
                          0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/*
 * This function is invoked by the internal thread when a client sends a
 * command to the daemon. This is what makes the CommandSource a source (of
 * Tpm2Commands). Here we take the fd, extract the command body from the fd
 * and transform it to a Tpm2Command. Most of the details are handled by
 * utility functions further down the stack.
 * If an error occurs while getting the command from the fd the connection
 * with the client will be closed and removed from the ConnectionManager.
 */
void
process_client_fd (CommandSource      *source,
                   gint                fd)
{
    TPMA_CC      attributes;
    Tpm2Command *command;
    Connection  *connection;
    uint8_t *buf = NULL;
    uint32_t command_size = 0;
    size_t index = 0;
    int ret = 0;

    g_debug ("process_client_fd 0x%" PRIxPTR, (uintptr_t)source);
    connection = connection_manager_lookup_fd (source->connection_manager, fd);
    if (connection == NULL)
        g_error ("failed to get connection associated with fd: %d", fd);
    else
        g_debug ("connection_manager_lookup_fd for fd %d: 0x%" PRIxPTR, fd,
                 (uintptr_t)connection);
    buf = g_malloc0 (TPM_HEADER_SIZE);
    ret = read_data (fd, &index, buf, TPM_HEADER_SIZE);
    if (ret == 0) {
        g_debug_bytes (buf, index, 16, 4);
    } else {
        goto fail_out;
    }
    command_size = get_command_size (buf);
    if (command_size > TPM_HEADER_SIZE && command_size <= UTIL_BUF_MAX) {
        buf = g_realloc (buf, command_size);
        ret = read_data (fd, &index, buf, command_size - TPM_HEADER_SIZE);
        if (ret == 0) {
            g_debug_bytes (buf, index, 16, 4);
        } else {
            goto fail_out;
        }
    } else if (command_size == TPM_HEADER_SIZE) {
        /* No more data. Command has no parameters / handles / auths etc. */
    } else {
        goto fail_out;
    }
    attributes = command_attrs_from_cc (source->command_attrs,
                                        get_command_code (buf));
    command = tpm2_command_new (connection, buf, index, attributes);
    if (command != NULL) {
        sink_enqueue (source->sink, G_OBJECT (command));
        /* the sink now owns this message */
        g_object_unref (command);
    } else {
        goto fail_out;
    }
    g_object_unref (connection);
    return;
fail_out:
    if (buf != NULL) {
        g_free (buf);
    }
    g_debug ("removing connection 0x%" PRIxPTR " from connection_manager "
             "0x%" PRIxPTR,
             (uintptr_t)connection,
             (uintptr_t)source->connection_manager);
    FD_CLR (fd, &source->receive_fdset);
    if (fd == source->maxfd) {
        /* find the new maxfd in the set */
        for (; fd >= 0; fd--) {
            if (FD_ISSET(fd, &source->receive_fdset)) {
                source->maxfd = fd;
                break;
            }
        }
    }
    connection_manager_remove (source->connection_manager,
                               connection);
    g_object_unref (connection);
    return;
}

ssize_t
process_wakeup_fd (CommandSource *source)
{
    char buf[3] = { 0 };
    ssize_t ret;

    g_debug ("Got new connection, updating fd_set");
    ret = read (source->wakeup_receive_fd, buf, WAKEUP_SIZE);
    if (ret != WAKEUP_SIZE)
        g_error ("read on wakeup_receive_fd returned %zd, was expecting %d",
                 ret, WAKEUP_SIZE);
    return ret;
}

/* This function is run as a separate thread dedicated to monitoring the
 * active connections with clients. It also monitors an additional file
 * descriptor that we call the wakeup_receive_fd. This pipe is the mechanism used
 * by the code that handles dbus calls to notify the command_source that
 * it's added a new connection to the connection_manager. When this happens the
 * command_source will begin to watch for data from this new connection.
 */
void*
command_source_thread (void *data)
{
    CommandSource *source;
    gint ret, i, tmp_maxfd;
    fd_set tmp_fds;

    source = COMMAND_SOURCE (data);
    do {
        tmp_fds = source->receive_fdset;
        tmp_maxfd = source->maxfd;

        if (!FD_ISSET (source->wakeup_receive_fd, &tmp_fds)) {
            g_warning ("selecting on fd_set w/o wakeup_receive_fd set");
        }
        ret = TEMP_FAILURE_RETRY (select (tmp_maxfd + 1,
                                          &tmp_fds, NULL, NULL, NULL));
        if (ret == -1) {
            g_warning ("Error selecting on pipes: %s", strerror (errno));
            break;
        }
        /*
         * Iterate over the set of fds and figure out what we need to do.
         * This takes advantage of the fact that fds are integers so the
         * loop counter 'i' is used as a stand in for the fd.
         */
        for (i = 0; i <= tmp_maxfd; ++i) {
            if (!FD_ISSET (i, &tmp_fds)) {
                continue;
            } else if (i != source->wakeup_receive_fd) {
                g_debug ("data ready on connection fd: %d", i);
                process_client_fd (source, i);
            } else {
                g_debug ("data ready on wakeup_receive_fd");
                process_wakeup_fd (source);
            }
        }
    } while (TRUE);

    return NULL;
}

CommandSource*
command_source_new (ConnectionManager    *connection_manager,
                    CommandAttrs         *command_attrs)
{
    CommandSource *source;
    gint wakeup_fds [2] = { 0, };

    if (connection_manager == NULL)
        g_error ("command_source_new passed NULL ConnectionManager");
    g_object_ref (connection_manager);
    if (pipe2 (wakeup_fds, O_CLOEXEC) != 0)
        g_error ("failed to make wakeup pipe: %s", strerror (errno));
    source = COMMAND_SOURCE (g_object_new (TYPE_COMMAND_SOURCE,
                                             "command-attrs", command_attrs,
                                             "connection-manager", connection_manager,
                                             "wakeup-receive-fd", wakeup_fds [0],
                                             "wakeup-send-fd", wakeup_fds [1],
                                             NULL));
    g_signal_connect (connection_manager,
                      "new-connection",
                      (GCallback) command_source_on_new_connection,
                      source);
    return source;
}
