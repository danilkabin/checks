#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "listhead.h"
#include <stddef.h>
#include <stdint.h>

extern struct list_head memoryPool_list;

struct memoryPool {
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

int memoryPool_isFull(struct memoryPool *pool);
int memoryPool_isFree(struct memoryPool *pool);
size_t memoryPool_getFreeSize(struct memoryPool *pool);
size_t memoryPool_getBusySize(struct memoryPool *pool);

int memoryPool_init(struct memoryPool **pool, size_t max_size, size_t block_size);

void *memoryPool_allocBlock(struct memoryPool *pool);
void memoryPool_freeBlock(struct memoryPool *pool, void *ptr);

void memoryPool_free(struct memoryPool *pool);

int memoryPools_init(void);
void memoryPools_release(void);

#endif
