/*#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct uidq_block {
   void *data;
   void *swap;
   size_t max_size;
   size_t current_size;
   size_t block_max;
   size_t block_size;
   size_t block_count;
   size_t block_free;
   uidq_bitmask *bitmask;
   bool initialized;
};

void *uidq_block_get(struct uidq_block *pool, int index);
int uidq_block_isFull(struct uidq_block *pool);
int uidq_block_isFree(struct uidq_block *pool);
size_t uidq_block_getFreeSize(struct uidq_block *pool);
size_t uidq_block_getBusySize(struct uidq_block *pool);

struct uidq_block *uidq_block_init(size_t max_size, size_t block_size);

void uidq_block_swap(struct uidq_block *pool, void *block1, void *block2);
void *uidq_block_alloc(struct uidq_block *pool, int *write);
void uidq_block_free(struct uidq_block *pool, void *ptr);

void uidq_block_exit(struct uidq_block *pool);

int uidq_blocks_init(void);
void uidq_blocks_release(void);

#endif*/
