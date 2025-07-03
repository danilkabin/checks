#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "listhead.h"
#include "sup.h"
#include <stddef.h>
#include <stdint.h>

extern struct list_head onion_block_list;

struct onion_block {
   void *data;
   size_t max_size;
   size_t current_size;
   size_t block_max;
   size_t block_size;
   size_t block_count;
   size_t block_free;
   uint8_t *bitmap;
   onion_bitmask *bitmask;

   struct list_head list;
};

void *onion_block_get(struct onion_block *pool, int index);
int onion_block_isFull(struct onion_block *pool);
int onion_block_isFree(struct onion_block *pool);
size_t onion_block_getFreeSize(struct onion_block *pool);
size_t onion_block_getBusySize(struct onion_block *pool);

int onion_block_init(struct onion_block **pool, size_t max_size, size_t block_size);

void *onion_block_alloc(struct onion_block *pool, int *write);
void onion_block_free(struct onion_block *pool, void *ptr);

void onion_block_exit(struct onion_block *pool);

int onion_blocks_init(void);
void onion_blocks_release(void);

#endif
