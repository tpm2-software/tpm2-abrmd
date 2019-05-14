/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
/*
 * The g_error function provided by GLib message logging module:
 * https://developer.gnome.org/glib/stable/glib-Message-Logging.html#g-error
 * is particularly difficult for Coverity since it doesn't know that this
 * function *always* aborts the program. Without this model for Coverity we
 * get piles of "Dereference after null check" false positives.
 */
void g_error (const char *format, ...) {
    __coverity_panic__ ();
}
