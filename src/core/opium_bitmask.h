#ifndef OPIUM_BITMASK_INCLUDE_H
#define OPIUM_BITMASK_INCLUDE_H

#include "core/opium_core.h"

#if OPIUM_PTR_SIZE == 8
typedef uint64_t opium_mask_t;
#else
typedef uint32_t opium_mask_t;
#endif

struct opium_bitmask_s {
   size_t nelts, max_elts;
   size_t elt_size; 

   opium_mask_t *mask;

   opium_arena_t *arena;
};

opium_bitmask_t *opium_bitmask_create(opium_arena_t *arena, size_t max_elts);
void opium_bitmask_destroy(opium_bitmask_t *bitmask);

int opium_bitmask_bit_is_set(opium_bitmask_t *bitmask, size_t index);

int opium_bitmask_set(opium_bitmask_t *bitmask, size_t index);
int opium_bitmask_clear(opium_bitmask_t *bitmask, size_t index);

int opium_bitmask_ffb(opium_bitmask_t *bitmask, int type);

#endif /* OPIUM_BITMASK_INCLUDE_H */
