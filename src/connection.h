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
#ifndef CONNECTION_H
#define CONNECTION_H

#include <glib.h>
#include <glib-object.h>

#include "handle-map.h"

G_BEGIN_DECLS

typedef struct _ConnectionClass {
    GObjectClass        parent;
} ConnectionClass;

typedef struct _Connection {
    GObject             parent_instance;
    gint                fd;
    guint64             id;
    HandleMap          *transient_handle_map;
} Connection;

#define TYPE_CONNECTION              (connection_get_type ())
#define CONNECTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CONNECTION, Connection))
#define CONNECTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST  ((klass), TYPE_CONNECTION, ConnectionClass))
#define IS_CONNECTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CONNECTION))
#define IS_CONNECTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_CONNECTION))
#define CONNECTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_CONNECTION, ConnectionClass))

GType            connection_get_type     (void);
Connection*      connection_new          (gint            *client_fd,
                                          guint64          id,
                                          HandleMap       *transient_handle_map);
gboolean         connection_equal_fd     (gconstpointer    a,
                                          gconstpointer    b);
gboolean         connection_equal_id     (gconstpointer    a,
                                          gconstpointer    b);
gpointer         connection_key_fd       (Connection      *session);
gpointer         connection_key_id       (Connection      *session);
gint             connection_fd           (Connection      *session);
HandleMap*       connection_get_trans_map(Connection      *session);
/* not part of the public API but included here for testing */
int              create_fd_pair (int *client_fd,
                                 int *server_fd,
                                 int  flags);

#endif /* CONNECTION_H */
