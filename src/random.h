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
#ifndef RANDOM_H
#define RANDOM_H

#include <glib.h>
#include <glib-object.h>
#include <stdint.h>
#include <stdlib.h>

G_BEGIN_DECLS

#define RANDOM_ENTROPY_FILE_BLOCK   "/dev/random"
#define RANDOM_ENTROPY_FILE_NOBLOCK "/dev/urandom"
#define RANDOM_ENTROPY_FILE_DEFAULT RANDOM_ENTROPY_FILE_BLOCK

typedef struct _RandomClass {
    GObjectClass    parent;
} RandomClass;

typedef struct _Random {
    GObject             parent_instance;
    struct drand48_data rand_state;
} Random;

#define TYPE_RANDOM              (random_get_type   ())
#define RANDOM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_RANDOM, Random))
#define RANDOM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_RANDOM, RandomClass))
#define IS_RANDOM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_RANDOM))
#define IS_RANDOM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_RANDOM))
#define RANDOM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_RANDOM, RandomClass))

GType         random_get_type              (void);
Random*       random_new                   (void);
int           random_seed_from_file        (Random       *random,
                                            const char   *fname);
size_t        random_get_bytes             (Random       *random,
                                            uint8_t       dest[],
                                            size_t        count);
uint32_t      random_get_uint32            (Random       *random);
uint32_t      random_get_uint32_range      (Random       *random,
                                            uint32_t      high,
                                            uint32_t      low);
uint64_t      random_get_uint64            (Random       *random);

G_END_DECLS
#endif /* RANDOM_H */
