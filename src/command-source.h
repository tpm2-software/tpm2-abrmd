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
#ifndef COMMAND_SOURCE_H
#define COMMAND_SOURCE_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <sys/select.h>

#include "command-attrs.h"
#include "connection-manager.h"
#include "sink-interface.h"
#include "thread.h"

G_BEGIN_DECLS

/* Maximum buffer size for client data. Connections that send a single
 * command larger than this size will be closed.
 */
#define BUF_MAX  PIPE_BUF

typedef struct _CommandSourceClass {
    ThreadClass       parent;
} CommandSourceClass;

typedef struct _CommandSource {
    Thread             parent_instance;
    ConnectionManager *connection_manager;
    CommandAttrs      *command_attrs;
    gint               wakeup_receive_fd;
    gint               wakeup_send_fd;
    fd_set             receive_fdset;
    Sink              *sink;
    int                maxfd;
} CommandSource;

#define TYPE_COMMAND_SOURCE              (command_source_get_type   ())
#define COMMAND_SOURCE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_COMMAND_SOURCE, CommandSource))
#define COMMAND_SOURCE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_COMMAND_SOURCE, CommandSourceClass))
#define IS_COMMAND_SOURCE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_COMMAND_SOURCE))
#define IS_COMMAND_SOURCE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_COMMAND_SOURCE))
#define COMMAND_SOURCE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_COMMAND_SOURCE, CommandSourceClass))

GType           command_source_get_type          (void);
CommandSource*  command_source_new               (ConnectionManager  *connection_manager,
                                                  CommandAttrs       *command_attrs);
gint            command_source_on_new_connection (ConnectionManager  *connection_manager,
                                                  Connection         *connection,
                                                  CommandSource      *command_source);
/*
 * The following are private functions. They are exposed here for unit
 * testing. Do not call these from anywhere else.
 */
void            process_client_fd                (CommandSource      *source,
                                                  gint                fd);

G_END_DECLS
#endif /* COMMAND_SOURCE_H */
