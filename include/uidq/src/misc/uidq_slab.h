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


bool uidq_slab_is_valid(const uidq_slab_t *slab, int pos, size_t size);
int uidq_slab_get_block_free(const uidq_slab_t *slab);
int uidq_slab_get_block_busy(const uidq_slab_t *slab);
void uidq_slab_update_counts(uidq_slab_t *slab, int64_t busy_delta, int64_t free_delta);
int uidq_slab_is_block_allocated(const uidq_slab_t *slab, int pos);

uidq_slab_block_t *uidq_slab_get_block(const uidq_slab_t *slab, int pos);

int uidq_slab_block_copy(uidq_slab_t *src, int src_pos, uidq_slab_t *dst, int dst_pos);
void *uidq_slab_get(uidq_slab_t *slab, int pos);

void uidq_slab_info(uidq_slab_t *slab);
void uidq_slab_info_block(uidq_slab_t *slab, int pos);

int uidq_block_realloc(uidq_slab_t *slab, int pos, void *new_data, size_t new_size);
int uidq_slab_alloc_at_pos(uidq_slab_t *slab, void *data, size_t size, int pos, size_t block_demand);
int uidq_slab_alloc_to_pos(uidq_slab_t *slab, void *data, size_t size, int pos);

int uidq_slab_alloc(uidq_slab_t *slab, void *data, size_t size);
void uidq_slab_dealloc(uidq_slab_t *slab, int pos);

int uidq_slab_init(uidq_slab_t *slab, size_t count, size_t block_size);
void uidq_slab_exit(uidq_slab_t *slab);

uidq_slab_t *uidq_slab_create();
void uidq_slab_free(uidq_slab_t *slab);
uidq_slab_t *uidq_slab_create_and_init(size_t count, size_t block_size);

#endif
