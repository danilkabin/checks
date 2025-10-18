#include "core/opium_core.h"

#define OPIUM_BITMASK_PTR_SIZE OPIUM_PTR_SIZE

   opium_bitmask_t *
opium_bitmask_create(opium_arena_t *arena, size_t max_elts)
{
   assert(arena != NULL);
   assert(max_elts > 0);

   size_t elt_size = (max_elts + OPIUM_BITMASK_PTR_SIZE - 1) / OPIUM_BITMASK_PTR_SIZE;

   opium_bitmask_t *bitmask = opium_arena_alloc(arena, sizeof(opium_bitmask_t));
   if (opium_unlikely(!bitmask)) {
      return NULL;
   }

   bitmask->mask = opium_arena_alloc(arena, elt_size * sizeof(opium_mask_t));
   if (opium_unlikely(!bitmask->mask)) {
      goto free_bitmask;
   }

   bitmask->nelts = 0;
   bitmask->max_elts = max_elts;
   bitmask->elt_size = elt_size;
   bitmask->arena = arena;

   return bitmask;

free_bitmask:
   opium_arena_free(arena, bitmask);
   return NULL;
}

   void
opium_bitmask_destroy(opium_bitmask_t *bitmask)
{
   assert(bitmask != NULL);

   opium_arena_t *arena = bitmask->arena;
   if (opium_unlikely(!arena)) {
      return;
   }

   opium_arena_free(arena, bitmask->mask);
   opium_arena_free(arena, bitmask);
}

   int
opium_bitmask_bit_is_set(opium_bitmask_t *bitmask, size_t index)
{
   assert(bitmask != NULL);
   assert(index < bitmask->max_elts);

   size_t word_index = index / OPIUM_BITMASK_PTR_SIZE;
   size_t bit_index  = index % OPIUM_BITMASK_PTR_SIZE;
   opium_mask_t mask = 1ULL << bit_index;

   return (bitmask->mask[word_index] & mask) != 0;
}

   int 
opium_bitmask_set(opium_bitmask_t *bitmask, size_t index)
{
   assert(bitmask != NULL);
   assert(index < bitmask->max_elts);

   size_t byte = index / OPIUM_BITMASK_PTR_SIZE;
   opium_mask_t mask = 1ULL << (index % OPIUM_BITMASK_PTR_SIZE);

   if (!opium_bitmask_bit_is_set(bitmask, index)) {
      bitmask->mask[byte] = bitmask->mask[byte] | mask;
      bitmask->nelts = bitmask->nelts + 1;
   }

   return OPIUM_RET_OK;
}

   int
opium_bitmask_clear(opium_bitmask_t *bitmask, size_t index)
{
   assert(bitmask != NULL);
   assert(index < bitmask->max_elts);

   size_t byte = index / OPIUM_BITMASK_PTR_SIZE;
   opium_mask_t mask = 1ULL << (index % OPIUM_BITMASK_PTR_SIZE);

   if (opium_bitmask_bit_is_set(bitmask, index)) {
      bitmask->mask[byte] = bitmask->mask[byte] & ~mask;
      bitmask->nelts = bitmask->nelts - 1;
   }

   return OPIUM_RET_OK;
}

   int
opium_bitmask_ffb(opium_bitmask_t *bitmask, int type)
{
   assert(bitmask != NULL);
   assert(type == 0 || type == 1);


   for (size_t index = 0; index < bitmask->elt_size; index++) {
      opium_mask_t word = type ? bitmask->mask[index] : ~bitmask->mask[index];
      if (word) {

#if OPIUM_BITMASK_PTR_SIZE == 8
         size_t pos_in_word = __builtin_ctzll(word);
#else
         size_t pos_in_word = __builtin_ctz(word);
#endif

         size_t pos = index * OPIUM_BITMASK_PTR_SIZE + pos_in_word;
         if (pos >= bitmask->max_elts) {
            return OPIUM_RET_ERR;
         }
         return pos;
      }
   }


   return OPIUM_RET_ERR;
}
