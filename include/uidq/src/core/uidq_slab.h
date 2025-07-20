#ifndef UIDQ_SLAB_H
#define UIDQ_SLAB_H

#include "misc/uidq_bitmask.h"

typedef struct {
   bool instance;
   bool initialized;

   size_t size;

   size_t block_count;
   size_t block_size;

   size_t block_busy;
   size_t block_free;

   void *data;
   uidq_bitmask_t *bitmask;
} uidq_slab_t;

int uidq_block_add(uidq_slab_t *slab, void *data, size_t size);

int uidq_slab_init(uidq_slab_t *slab, size_t size, size_t block_size);
void uidq_slab_exit(uidq_slab_t *slab);

uidq_slab_t *uidq_slab_create();
void uidq_slab_free(uidq_slab_t *slab);

uidq_slab_t *uidq_slab_create_and_init(size_t size, size_t block_size);

#endif
