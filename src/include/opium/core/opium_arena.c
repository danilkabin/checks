/* opium_arena.c 
 * 
 */

#include "core/opium_arena.h"
#include "core/opium_core.h"
#include "core/opium_slab.h"
#include "opium.h"
#include "opium_alloc.h"
#include "opium_arena.h"
#include "opium_log.h"
#include "opium_slab.h"
#include <assert.h>
#include <stdio.h>

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
}

   void * 
opium_arena_alloc(opium_arena_t *arena, size_t size)
{
   if (size <= 1) {
      return NULL;
   }

   size_t round_size = opium_round_of_two(size);

   if (round_size < arena->min_size) {
      round_size = arena->min_size;
   }

   size_t index = opium_log2(round_size) - OPIUM_ARENA_MIN_SHIFT;

   opium_slab_t *slab = &arena->slabs[index];

   void *ptr = opium_slab_alloc(slab);
   opium_slab_slot_header_set(ptr, index);   

   opium_log_debug(arena->log, "ptr header: %zu, size: %zu, index: %zu\n", 
         *(size_t*)opium_slab_slot_header(ptr), round_size, index); 

   return ptr;
}

void
opium_arena_free(opium_arena_t *arena, void *ptr)
{
   assert(arena != NULL);
   assert(ptr != NULL);

   size_t index = *(size_t*)opium_slab_slot_header(ptr);

   opium_slab_t *slab = &arena->slabs[index];

   printf("Deleting: %zu\n", index);

   opium_slab_free(slab, ptr);
}
