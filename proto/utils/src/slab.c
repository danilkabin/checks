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

void slab_add(struct slab *allocator, void *data, size_t size, int *start, int *end) {
   if (!allocator || !data || size == 0) {
      DEBUG_FUNC("Invalid input\n");
      return;
   }

   *start = -1;
   *end = -1;

   size_t block_size = allocator->block_size; 
   size_t blocks_needed = (size + block_size - 1) / block_size;

   if (blocks_needed > allocator->block_capacity - allocator->block_used) {
      DEBUG_FUNC("Not enough free blocks\n");
      return;
   }
   DEBUG_FUNC("block_size: %zu, block need: %zu, data size: %zu\n", block_size, blocks_needed, size);
   int start_bit;
   size_t real_size = size;
   while ((start_bit = find_consecutive_free_bits(allocator->bitmask, allocator->bitmask_size, blocks_needed)) != -1) {
      for (size_t index = 0; index < blocks_needed; index++) {
         size_t bit_offset = start_bit + index;
         size_t offset = bit_offset * block_size;
         size_t remain = real_size > block_size ? block_size : real_size;

         set_bit(allocator->bitmask, bit_offset);
         real_size = real_size - block_size;
         allocator->blocks[bit_offset].used = remain; 
         DEBUG_FUNC("start_bit: %d, block offset: %zu index: %zu, used: %zu\n", start_bit, offset, index, remain); 
      }
      allocator->block_used = allocator->block_used + blocks_needed;
      memcpy((uint8_t*)allocator->pool + start_bit * block_size, data, size);
      *start = start_bit;
      *end = start_bit + blocks_needed - 1;
      return;
   }
   DEBUG_FUNC("No consecutive free bits found\n");
   return;
}

void slab_del(struct slab *allocator, int start, int end) {
   size_t del = end - start + 1;
   allocator->block_used = allocator->block_used - del;
   clear_consecutive_busy_bits(allocator->bitmask, start, end + 1);
   memset((uint8_t*)allocator->pool + start * allocator->block_size, 0, del * allocator->block_size);
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
