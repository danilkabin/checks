#include "slab.h"
#include "utils.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int find_consecutive_free_bits(uint8_t *bitmap, size_t bitmask_size, size_t blocks_needed) {
   size_t total_bits = bitmask_size * 8;
   size_t consecutive_free = 0;
   int start_index = -1;

   for (size_t i = 0; i < total_bits; i++) {
      size_t byte = i / 8;
      size_t bit = i % 8;

      if (byte >= bitmask_size || i >= bitmask_size * 8) {
         break;
      }

      if (!(bitmap[byte] & (1 << bit))) {
         if (consecutive_free == 0) {
            start_index = i; 
         }
         consecutive_free++;
         if (consecutive_free >= blocks_needed) {
            return start_index;
         }
      } else {
         consecutive_free = 0;
         start_index = -1;
      }
   }

   return -1; 
}

static void clear_consecutive_busy_bits(uint8_t *bitmap, size_t start, size_t end) {
   for (size_t bit = start; bit < end; bit++) {
      clear_bit(bitmap, bit);
   }
}


static void slab_write_blocks(struct slab *allocator, int start_bit, void *data, size_t size, size_t blocks_needed, int set_bits) {
   size_t block_size = allocator->block_size;
   size_t real_size = size;

   for (size_t index = 0; index < blocks_needed; index++) {
      size_t bit_offset = start_bit + index;
      size_t remain = real_size > block_size ? block_size : real_size;

      if (set_bits) set_bit(allocator->bitmask, bit_offset);

      allocator->blocks[bit_offset].used = remain;
      allocator->blocks[bit_offset].is_start = (index == 0);
      if (index == 0) allocator->blocks[bit_offset].allocation_size = blocks_needed;

      real_size = real_size > block_size ? real_size - block_size : 0;
   }

   if (data != NULL) {
      memcpy((uint8_t*)allocator->pool + start_bit * block_size, data, size);
   }
   allocator->memory_allocated = allocator->memory_allocated + blocks_needed;
      DEBUG_FUNC("slab allocated: %zu\n", allocator->memory_allocated);
}

void slab_del(struct slab *allocator, int start, int end) {
   if (!allocator) {
      DEBUG_FUNC("Invalid allocator\n");
      return;
   }

   if (start < 0 || end < start || (size_t)end >= allocator->block_capacity) {
      DEBUG_FUNC("Invalid start or end indices\n");
      return;
   }

   size_t del = (size_t)(end - start + 1);

   if (del > allocator->block_used) {
      DEBUG_FUNC("Trying to free more blocks than allocated\n");
      return;
   }

   clear_consecutive_busy_bits(allocator->bitmask, (size_t)start, (size_t)end + 1);
   memset((uint8_t*)allocator->pool + start * allocator->block_size, 0, del * allocator->block_size);
   allocator->block_used -= del;
      allocator->memory_allocated = allocator->memory_allocated - del;
      DEBUG_FUNC("slab deleted: %zu\n", allocator->memory_allocated);
}

void *slab_malloc(struct slab *allocator, void *ptr, size_t size) {
   if (!allocator || size == 0) {
      DEBUG_FUNC("Invalid input\n");
      goto free_this_trash;
   }

   size_t block_size = allocator->block_size;
   size_t blocks_needed = (size + block_size - 1) / block_size;

   if (blocks_needed > allocator->block_capacity - allocator->block_used) {
      DEBUG_FUNC("Not enough free blocks\n");
      goto free_this_trash;
   }

   int start_bit = find_consecutive_free_bits(allocator->bitmask, allocator->bitmask_size, blocks_needed);
   if (start_bit == -1) {
      DEBUG_FUNC("No consecutive free bits found\n");
      goto free_this_trash;
   }

   slab_write_blocks(allocator, start_bit, ptr, size, blocks_needed, 1);
   allocator->block_used += blocks_needed;
   return (uint8_t*)allocator->pool + start_bit * block_size;
free_this_trash:
   return NULL;
}

void slab_free(struct slab *allocator, void *ptr) {
   if (!allocator || !ptr) {
      DEBUG_FUNC("Invalid input\n");
      goto free_this_trash;
   }
   int start_index = ((uint8_t*)ptr - (uint8_t*)allocator->pool) / allocator->block_size;
   if (start_index < 0 || (size_t)start_index >= allocator->block_capacity) {
      DEBUG_FUNC("start_index faild\n");
      goto free_this_trash;
   }
   if (!allocator->blocks[start_index].is_start) {
      DEBUG_FUNC("start faild\n");
      goto free_this_trash;
   }

   size_t size = allocator->blocks[start_index].allocation_size;
   slab_del(allocator, start_index, start_index + size - 1);
free_this_trash:
   return;
}

void *slab_realloc(struct slab *allocator, void *ptr, size_t new_size) {
   if (!allocator) { 
      goto free_this_trash;
   }
   if (ptr == NULL) {
      return slab_malloc(allocator, ptr, new_size);
   }
   if (new_size == 0) {
      slab_free(allocator, ptr);
      return NULL;
   }
   int start_index = ((uint8_t*)ptr - (uint8_t*)allocator->pool) / allocator->block_size;
   if (start_index < 0 || (size_t)start_index >= allocator->block_capacity) {
      DEBUG_FUNC("start_index faild\n");
      goto free_this_trash;
   }

   struct slab_block *block = &allocator->blocks[start_index];
   if (!block->is_start) {
      DEBUG_FUNC("no block\n");
      goto free_this_trash;
   }

   int old_blocks = (int)block->allocation_size;
   int end_index = start_index + old_blocks - 1;
   size_t block_size = allocator->block_size;
   size_t new_blocks = (new_size + block_size - 1) / block_size;

   if (new_blocks <= (size_t)old_blocks) {
      allocator->blocks[start_index].allocation_size = new_blocks;
      for (int i = start_index + new_blocks; i <= end_index; i++) {
         clear_bit(allocator->bitmask, i);
         allocator->blocks[i].used = 0;
      }
      allocator->block_used -= (old_blocks - new_blocks);
      return ptr;
   }

   int can_expand = 1;
   for (int i = 0; i < (int)(new_blocks - old_blocks); i++) {
      int next_bit = end_index + 1 + i;
      if ((size_t)(next_bit) >= allocator->block_capacity ||
            (allocator->bitmask[next_bit / 8] & (1 << (next_bit % 8)))) {
         can_expand = 0;
         break;
      }
   }

   if (can_expand) {
      for (int i = 0; i < (int)(new_blocks - old_blocks); i++) {
         int bit_index = end_index + 1 + i;
         set_bit(allocator->bitmask, bit_index);
         allocator->blocks[bit_index].used = 0;
      }
      allocator->blocks[start_index].allocation_size = new_blocks;
      allocator->block_used += (new_blocks - old_blocks);
      return ptr;
   }

   void *new_ptr = slab_malloc(allocator, ptr, new_size);
   if (!new_ptr) return NULL;

   size_t copy_size = old_blocks * block_size;
   memcpy(new_ptr, ptr, copy_size > new_size ? new_size : copy_size);

   slab_free(allocator, ptr);
   return new_ptr;
free_this_trash:
   return NULL;
}

struct slab *slab_init(size_t total_pool_size) {
   size_t multiply = 32;
   total_pool_size = round_size_pow2(total_pool_size < multiply * 4 ? multiply * 4 : total_pool_size);
   size_t blocks = total_pool_size / multiply;
   if (blocks < multiply) {
      blocks = multiply;
   } 
   struct slab *allocator = malloc(sizeof(struct slab));
   if (!allocator) {
      DEBUG_FUNC("no allocator\n");
      goto free_this_trash;
   }

   allocator->pool = malloc(total_pool_size);
   if (!allocator->pool) {
      DEBUG_FUNC("no allocator pool\n");
      goto free_allocator;
   }

   size_t blocks_size = blocks * sizeof(struct slab_block);
   allocator->blocks = malloc(blocks_size);
   if (!allocator->blocks) {
      DEBUG_FUNC("no allocator blocks\n");
      goto free_pool;
   }
   memset(allocator->blocks, 0, blocks_size);

   allocator->bitmask_size = (blocks + 7) / 8;
   allocator->bitmask = malloc(allocator->bitmask_size);
   if (!allocator->bitmask) {
      DEBUG_FUNC("no allocator bitmask\n");
      goto free_blocks;
   }
   memset(allocator->bitmask, 0, allocator->bitmask_size);

   allocator->pool_size = total_pool_size;
   allocator->block_capacity = blocks;
   allocator->block_used = 0;
   allocator->block_size = allocator->pool_size / allocator->block_capacity;
   for (int index = 0; index < (int)allocator->block_capacity; index++) {
      allocator->blocks[index].used = 0;
   }
   return allocator;

free_blocks:
   free(allocator->blocks);
free_pool:
   free(allocator->pool);
free_allocator:
   free(allocator);
free_this_trash:
   return NULL;
}

void slab_exit(struct slab *allocator) {
   if (!allocator) return;
   if (allocator->bitmask) {
      free(allocator->bitmask);
   }
   if (allocator->pool) {
      free(allocator->pool);
   }
   if (allocator->blocks) {
      free(allocator->blocks);
   }
   free(allocator);
   DEBUG_FUNC("allocator deleted\n");
}
