/*#include "uidq_block.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

size_t uidq_bs_malloc_current_size = 0;

struct uidq_block *uidq_block_init(size_t max_size, size_t block_size) {
   int ret = -1;

   struct uidq_block *pool = malloc(sizeof(*pool));
   if (!pool) {
      DEBUG_FUNC("The pool is null!\n");
      goto free_this_trash;
   }
   pool->initialized = true;

   pool->swap = malloc(block_size);
   if (!pool->swap) {
      DEBUG_ERR("Swap initialization failed.\n");
      goto free_pool;
   }

   size_t maxSize = round_size_pow2(max_size);
   size_t blockSize = block_size;
   pool->block_max = maxSize / blockSize;

   pool->max_size = maxSize;
   pool->current_size = 0;
   pool->block_count = 0;
   pool->block_free = pool->block_max;
   pool->block_size = blockSize;

   ret = uidq_bitmask_init(&pool->bitmask, maxSize / block_size, sizeof(uint64_t));
   if (ret < 0) {
      DEBUG_ERR("Failed to init bitmask.\n");
      goto free_pool;
   }

   size_t bitmap_size = (pool->block_max + 63) / 64;
  // DEBUG_FUNC("BLOCK MAX: %zu, BIT MAP SIZE: %zu\n", blockSize, bitmap_size);

   pool->data = malloc(maxSize);
   if (!pool->data) {
      DEBUG_FUNC("The data is null!\n");
      goto free_pool;
   }

   uidq_bs_malloc_current_size = uidq_bs_malloc_current_size + maxSize;
  // DEBUG_FUNC("Allocated: %zu, size: %zu\n", uidq_bs_malloc_current_size, maxSize);
   ret = 0;
   return pool;

free_pool:
   uidq_block_exit(pool);
free_this_trash:
   return NULL;
}

void uidq_block_exit(struct uidq_block *pool) {
   if (!pool) return;
   pool->initialized = false;

   uidq_bs_malloc_current_size = uidq_bs_malloc_current_size - pool->max_size;
   if (pool->data) {
      free(pool->data);
      pool->data = NULL;
   }

   if (pool->bitmask) {
      uidq_bitmask_exit(pool->bitmask);
      pool->bitmask = NULL;
   }

   if (pool->swap) {
      free(pool->swap);
      pool->swap = NULL;
   }

   //DEBUG_FUNC("free, Allocated: %zu, max_size: %zu", uidq_bs_malloc_current_size, pool->max_size);
   free(pool);
}

void *uidq_block_get(struct uidq_block *pool, int index) {
   return (void*)((uint8_t*)pool->data + index * pool->block_size);
}

void uidq_block_swap(struct uidq_block *pool, void *block1, void *block2) {
   if (!pool || !block1 || !block2 || block1 == block2) {
      return;
   }
   void *temp = pool->swap;
   memcpy(temp, block1, pool->block_size);
   memcpy(block1, block2, pool->block_size);
   memcpy(block2, temp, pool->block_size);
}

void *uidq_block_alloc(struct uidq_block *pool, int *write) {
   if (!pool) return NULL;
   if (uidq_block_isFull(pool)) {
      DEBUG_FUNC("Memory pool is full!\n");
      return NULL;
   }

   int index = uidq_bitmask_add(pool->bitmask, 1);
   if (index < 0) {
      DEBUG_ERR("Failed to allocate bit in bitmask!\n");
      return NULL;
   }

   pool->block_free--;
   pool->block_count++;
   pool->current_size += pool->block_size;

   return (void*)((uint8_t*)pool->data + index * pool->block_size);
}

void uidq_block_free(struct uidq_block *pool, void *ptr) {
   int offset = (uint8_t*)ptr - (uint8_t*)pool->data;
   size_t index = offset / pool->block_size;

   if (index >= pool->block_max) {
      DEBUG_FUNC("index > blockmax\n");
      return;
   }

   uidq_bitmask_del(pool->bitmask, index, 1);
   pool->block_free++;
   pool->block_count--;
   pool->current_size -= pool->block_size;

}

int uidq_block_isFull(struct uidq_block *pool) {
   return pool->block_free == 0;
}

int uidq_block_isFree(struct uidq_block *pool) {
   return pool->block_free > 0;
}

size_t uidq_block_getBusySize(struct uidq_block *pool) {
   return pool->block_count * pool->block_size;
}

size_t uidq_block_getFreeSize(struct uidq_block *pool) {
   return pool->block_free * pool->block_size;
}*/
