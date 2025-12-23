/* opium_arena.c 
 *
 * There is no need to write anything special on top of the slab.
 * The whole point of the arena is that it:
 *
 * - Distribues the load access sets of slabs (16, 32, 64 ... bytes etc.)
 * - Knows the rules for choosing the right slab (via round of two and log)
 * - Saves the slab index in the slab header (so it can be freed later without a lookup)
 *
 * It doesn`t need any more logic (for allocation/free);
 * everything else is already hidden is the slab
 *
 */

#include "core/opium_core.h"

   int
opium_arena_init(opium_arena_t *arena, opium_log_t *log)
{   
   assert(arena != NULL);

   arena->min_shift = OPIUM_ARENA_MIN_SHIFT;
   arena->max_shift = OPIUM_ARENA_MAX_SHIFT;

   arena->min_size = 1 << (arena->min_shift);

   /*
    * For example :
    * - OPIUM_ARENA_MAX_SHIFT = 10; [1024 bytes]
    * - OPIUM_ARENA_MIN_SHIFT = 4;  [16 bytes]
    * 
    * shift_count = 10 - 4 + 1 = 7 
    *
    * This means the arena will support 7 block sizes:
    * 16, 32, 64, 128, 256, 512, 1024 bytes.
    */
   arena->shift_count = arena->max_shift - arena->min_shift + 1;

   /*
    * Allocate an array of opium_slab_t structutes of size shift_count.
    * 
    * Why use slabs?
    *
    * - Each slab can only work with fixed sizes: 16, 32, 64, etc bytes. 
    * - In a real program, arbitrary sizes are often needed.
    * Arena combines a set of slabs and automatically selects the appropriate
    * slab for any request.
    *
    * This approach:
    * 1) Avoids code duplication (slab already manages pages, slots and bitmask)
    * 2) Allows for quick memory for different sizes.
    * 3) Simplifies statistics and logging - are sees all slabs at once.
    *
    * Idea: slab = building block, arena = manager of the entire structure.
    */
   arena->slabs = opium_calloc(sizeof(opium_slab_t) * arena->shift_count, log);
   if (!arena->slabs) {
      opium_log_err(log, "Failed to allocate slabs structutes.\n");
      return OPIUM_RET_ERR;
   }

   for (size_t index = 0; index < arena->shift_count; index++) {
      opium_slab_t *slab = &arena->slabs[index];
      size_t item_size = 1 << (OPIUM_ARENA_MIN_SHIFT + index);

      opium_slab_init(slab, item_size, log);
   }

   arena->log = log;

   return OPIUM_RET_OK;
}

   void
opium_arena_exit(opium_arena_t *arena)
{
   assert(arena != NULL);

   /* Just rustle all the slabs and delete them. */
   for (size_t index = 0; index < arena->shift_count; index++) {
      opium_slab_exit(&arena->slabs[index]);
   }

   arena->min_size = 0;
   arena->shift_count = arena->min_shift = arena->max_shift = 0;
   arena->log = NULL;
}

   void * 
opium_arena_alloc(opium_arena_t *arena, size_t size)
{
   /* Protection against incorrect request */
   if (size <= 1) {
      return NULL;
   }

   /* 
    * Round the size to the nearest power of two
    * All slabs operate on fixed-size blocks. Therefore, any request
    * is rounded up to the nearest power of two.
    */
   size_t round_size = opium_round_of_two(size);

   if (round_size < arena->min_size) {
      /* Guarantee that will never allocate less than the smallest block */
      round_size = arena->min_size;
   }

   /* This is how we find which specific slab will service this request
    * Examples: OPIUM_ARENA_MIN_SHIFT = 4 then 2^4 = 16 (min_size)
    * index = log(16) - OPIUM_ARENA_MIN_SHIFT = 4 - 4 = 0
    * index = log(32) - OPIUM_ARENA_MIN_SHIFT = 5 - 4 = 1
    * index = log(64) - OPIUM_ARENA_MIN_SHIFT = 6 - 4 = 2
    */
   size_t index = opium_log2(round_size) - OPIUM_ARENA_MIN_SHIFT;

   opium_slab_t *slab = &arena->slabs[index];

   void *ptr = opium_slab_alloc(slab);

   /*
    * We store a label indicating which slab allocated this block.
    * This is necessary so that we can quickly find the slab by index
    * when deallocating it. (Otherwise, we`d have to search by size - slower)
    */
   opium_slab_header_t *header = opium_slab_slot_header(ptr);
   header->index = index;

   printf("ptr: %p, header: %p, value: %d\n",
         ptr, header, header->index);

   return ptr;
}

   void *
opium_arena_calloc(opium_arena_t *arena, size_t size)
{
   assert(arena != NULL);

   /* Default alloc with memzero */
   void *ptr = opium_arena_alloc(arena, size);
   if (!ptr) {
      return NULL;
   }

   opium_memzero(ptr, size);

   return ptr;
}

   void
opium_arena_free(opium_arena_t *arena, void *ptr)
{
   assert(arena != NULL);
   assert(ptr != NULL);

   /*
    * Read the same slab index that was saved during the alloc
    * Now know exactly which slab this block belongs to
    */

   opium_slab_header_t *header = opium_slab_slot_header(ptr);
   printf("DELETE ptr: %p, header: %p, value: %d\n",
         ptr, header, header->index);

   opium_slab_t *slab = &arena->slabs[header->index];

   void *slot_ptr = (u_char*)ptr - OPIUM_SLAB_SLOT_HEADER;

   opium_slab_free(slab, slot_ptr);
}
