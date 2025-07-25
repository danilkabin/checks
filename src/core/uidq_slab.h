#ifndef UIDQ_SLAB_H
#define UIDQ_SLAB_H

#include "misc/uidq_bitmask.h"

typedef struct {
   size_t count;
} uidq_slab_block_t;

typedef struct {
   bool initialized;

   size_t size;

   size_t block_count;
   size_t block_size;
   size_t block_busy;
   size_t block_free;

   void *data;
   uidq_slab_block_t *block_data;
   uidq_bitmask_t *bitmask;
} uidq_slab_t;

void *uidq_slab_get(uidq_slab_t *slab, int start_pos);
void uidq_slab_info(uidq_slab_t *slab);
void uidq_slab_info_block(uidq_slab_t *slab, int start_pos);

int uidq_block_realloc(uidq_slab_t *slab, int start_pos, void *new_data, size_t new_size);
int uidq_slab_alloc(uidq_slab_t *slab, void *data, size_t size);
void uidq_slab_dealloc(uidq_slab_t *slab, int start_pos);
void uidq_slab_dealloc_mound(uidq_slab_t *slab, int pos, int offset);

int uidq_slab_init(uidq_slab_t *slab, size_t size, size_t block_size);
void uidq_slab_exit(uidq_slab_t *slab);

uidq_slab_t *uidq_slab_create();
void uidq_slab_free(uidq_slab_t *slab);

uidq_slab_t *uidq_slab_create_and_init(size_t size, size_t block_size);

#endif
