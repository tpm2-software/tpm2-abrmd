/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#ifndef RANDOM_H
#define RANDOM_H

#include <glib.h>
#include <glib-object.h>
#include <stdint.h>
#include <stdlib.h>

G_BEGIN_DECLS

typedef struct _RandomClass {
    GObjectClass    parent;
} RandomClass;

typedef struct _Random {
    GObject             parent_instance;
    unsigned short      rand_state[3];
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
