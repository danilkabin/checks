#include "pool.h"
#include "listhead.h"
#include "sup.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

struct list_head onion_block_list = LIST_HEAD_INIT(onion_block_list);

size_t onion_bs_malloc_current_size = 0;

struct onion_block *onion_block_init(size_t max_size, size_t block_size) {
   int ret = -1;

   struct onion_block *pool = malloc(sizeof(*pool));
   if (!pool) {
      DEBUG_FUNC("The pool is null!\n");
      goto free_this_trash;
   }

   pool->swap = malloc(block_size);
   if (!pool->swap) {
      DEBUG_ERR("Swap initialization failed.\n");
      goto free_pool;
   }

   size_t maxSize = round_size_pow2(max_size);
   size_t blockSize = block_size;
   pool->block_max = maxSize / blockSize;

   pool->current_size = 0;
   pool->block_count = 0;
   pool->block_free = pool->block_max;
   pool->block_size = blockSize;

   ret = onion_bitmask_init(&pool->bitmask, max_size / block_size, sizeof(uint64_t));
   if (ret < 0) {
      DEBUG_ERR("Failed to init bitmask.\n");
      goto free_pool;
   }
   size_t bitmap_size = (pool->block_max + 63) / 64;
   DEBUG_FUNC("BLOCK MAX: %zu, BIT MAP SIZE: %zu\n", blockSize, bitmap_size);
   pool->bitmap = calloc(bitmap_size, sizeof(uint64_t));
   if (!pool->bitmap) {
      DEBUG_FUNC("The bitmap is null!\n");
      goto free_pool;
   }

   pool->data = malloc(maxSize);
   if (!pool->data) {
      DEBUG_FUNC("The data is null!\n");
      goto free_pool;
   }

   ret = 0;
   return pool;
free_pool:
   onion_block_exit(pool);
free_this_trash:
   return NULL;
}

void onion_block_exit(struct onion_block *pool) {
   list_del(&pool->list);

   if (pool->data) {
      free(pool->data);
      pool->data = NULL;
   }

   if (pool->bitmask) {
      onion_bitmask_exit(pool->bitmask);
      pool->bitmask = NULL;
   }

   if (pool->swap) {
      free(pool->swap);
      pool->swap = NULL;
   }

   free(pool);
}

void *onion_block_get(struct onion_block *pool, int index) {
   return (void*)((uint8_t*)pool->data + index * pool->block_size);
}

void onion_block_swap(struct onion_block *pool, void *block1, void *block2) {
   if (!pool || !block1 || !block2 || block1 == block2) {
      return;
   }
   void *temp = pool->swap;
   memcpy(temp, block1, pool->block_size);
   memcpy(block1, block2, pool->block_size);
   memcpy(block2, temp, pool->block_size);
}

void *onion_block_alloc(struct onion_block *pool, int *write) {
   if (!pool) return NULL;
   if (onion_block_isFull(pool)) {
      DEBUG_FUNC("Memory pool is full!\n");
      return NULL;
   }

   int index = onion_bitmask_add(pool->bitmask, 1);
   if (index < 0) {
      DEBUG_ERR("Failed to allocate bit in pidor bitmask!\n");
      return NULL;
   }

  /* int index = ffb(pool->bitmap, pool->block_max);
   if (index < 0) {
      DEBUG_FUNC("find_first_free_bit was failed!\n");
      return NULL;
   }*/

   //DEBUG_FUNC("INDEXX:%d  index nemoi: %d\n", index1, index);
   //set_bit(pool->bitmap, index);
   pool->block_free--;
   pool->block_count++;
   pool->current_size += pool->block_size;
   onion_bs_malloc_current_size = onion_bs_malloc_current_size + pool->block_size;
   //if (write) {
   //   write = &index;
   // }
   return (void*)((uint8_t*)pool->data + index * pool->block_size);
}

void onion_block_free(struct onion_block *pool, void *ptr) {
   int offset = (uint8_t*)ptr - (uint8_t*)pool->data;
   size_t index = offset / pool->block_size;

   if (index >= pool->block_max) {
      DEBUG_FUNC("index > blockmax\n");
      return;
   }

   onion_bitmask_del(pool->bitmask, index, 1);
   clear_bit(pool->bitmap, index);
   pool->block_free++;
   pool->block_count--;
   pool->current_size -= pool->block_size;

   onion_bs_malloc_current_size = onion_bs_malloc_current_size - pool->block_size;
}

int onion_block_isFull(struct onion_block *pool) {
   return pool->block_free == 0;
}

int onion_block_isFree(struct onion_block *pool) {
   return pool->block_free > 0;
}

size_t onion_block_getBusySize(struct onion_block *pool) {
   return pool->block_count * pool->block_size;
}

size_t onion_block_getFreeSize(struct onion_block *pool) {
   return pool->block_free * pool->block_size;
}

int onion_blocks_init() {
   INIT_LIST_HEAD(&onion_block_list);
   return 0;
}

void onion_blocks_release() {
   struct onion_block *node, *tmp;
   list_for_each_entry_safe(node, tmp, &onion_block_list, list) {
      DEBUG_FUNC("pool deleting! \n");
      onion_block_exit(node);
   }
}
