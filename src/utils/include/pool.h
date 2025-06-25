#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "listhead.h"
#include <stddef.h>
#include <stdint.h>

extern struct list_head onion_memoryPool_list;

struct onion_memoryPool {
   void *data;
   size_t max_size;
   size_t current_size;
   size_t block_max;
   size_t block_size;
   size_t block_count;
   size_t block_free;
   uint8_t *bitmap;

   struct list_head list;
};

int onion_memoryPool_isFull(struct onion_memoryPool *pool);
int onion_memoryPool_isFree(struct onion_memoryPool *pool);
size_t onion_memoryPool_getFreeSize(struct onion_memoryPool *pool);
size_t onion_memoryPool_getBusySize(struct onion_memoryPool *pool);

int onion_memoryPool_init(struct onion_memoryPool **pool, size_t max_size, size_t block_size);

void *onion_memoryPool_allocBlock(struct onion_memoryPool *pool);
void onion_memoryPool_freeBlock(struct onion_memoryPool *pool, void *ptr);

void onion_memoryPool_free(struct onion_memoryPool *pool);

int onion_memoryPools_init(void);
void onion_memoryPools_release(void);

#endif
