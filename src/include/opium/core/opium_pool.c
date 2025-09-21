/* @file opium_pool.c
 * @brief Memory pool implementation using bitmask tracking for allocation.
 *
 * This module provides a simple fixed-block memory pool for acceptable
 * allocation and frees. The pool uses a bitmask to track which block
 * are currently allocated. It supports allocation at specific index,
 * freeing by index or pointer, and resetting the entire pool.
 *
 * Key-features:
 *  - Fixed-size blocks for acceptable allocation
 *  - Bitmask tracking of used/free blocks
 *  - Safe access checks for invalid indices and pointers
 *
 * Why this particular approach?
 *  I think that using such a simple pool for the server is
 *  quite acceptable. Considering that the compiler processes memory
 *  in 32/64-bit packets, this approach is quite optimized and simple.
 *
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "core/opium_pool.h"

#define OPIUM_POOL_ALIGNMENT_SIZE 16

   int
opium_pool_isvalid(opium_pool_t *pool)
{
   return pool && opium_flag_istrue(pool->initialized);
}

   static inline int
opium_pool_check_index(opium_pool_t *pool, int index) 
{
   if (!opium_pool_isvalid(pool)) {
      return OPIUM_RET_ERR;
   }

   if (index < 0 || (size_t)index >= pool->capacity) {
      return OPIUM_RET_ERR;
   }

   return opium_bitmask_bit_test(pool->bitmask, index) ? OPIUM_RET_OK : OPIUM_RET_ERR;
}

   static inline int
opium_pool_check_ptr(opium_pool_t *pool, void *ptr, int *out_index)
{
   if (!opium_pool_isvalid(pool)) {
      return OPIUM_RET_ERR;
   }

   ptrdiff_t diff = (char*)ptr - (char*)pool->data;
   if (diff < 0 || (size_t)diff >= pool->capacity * pool->block_size) {
      return OPIUM_RET_ERR;
   }

   int index = diff / pool->block_size;
   if (!opium_bitmask_bit_test(pool->bitmask, index)) {
      return OPIUM_RET_ERR;
   }

   if (out_index) *out_index = index;
   return OPIUM_RET_OK;
}

   void *
opium_pool_get(opium_pool_t *pool, int index)
{
   if (opium_pool_check_index(pool, index) != OPIUM_RET_OK) {
      return NULL;
   }

   return (char*)pool->data + index * pool->block_size;
}

   int
opium_pool_index(opium_pool_t *pool, void *ptr) 
{
   int index;
   if (opium_pool_check_ptr(pool, ptr, &index) != OPIUM_RET_OK) {
      return -1;
   }

   return index;
}

   opium_pool_t *
opium_pool_create(size_t capacity, size_t block_size, opium_log_t *log) 
{
   /* Create a memory pool with fixed block size.
    * 
    * @param capacity - number of blocks in the pool.
    * @param block_size - size of each block in bytes.
    *
    */

   opium_pool_t *pool;
   size_t size = capacity * block_size;

   if (capacity == 0 || block_size == 0) {
      opium_log_err(log, "%s: Invalid parameters\n", __func__);
      return NULL;
   }

   pool = opium_calloc(sizeof(opium_pool_t), log);
   if (!pool) {
      opium_log_err(log, "%s: Failed to allocate pool\n", __func__);
      return NULL;
   }
   opium_flag_true(pool->initialized); 

   pool->data = opium_memalign(OPIUM_POOL_ALIGNMENT_SIZE, size, log);
   if (!pool->data) {
      opium_log_err(log, "Failed to allocate pool data\n");
      goto fail;
   }

   pool->bitmask = opium_bitmask_create(capacity, log);
   if (!pool->bitmask) {
      opium_log_err(log, "Failed to create bitmask\n");
      goto fail;
   }

   pool->capacity = capacity;
   pool->block_size = block_size;
   pool->log = log;

   return pool;

fail:
   opium_pool_destroy(pool);
   return NULL;
}

void
opium_pool_destroy(opium_pool_t *pool) {
   if (!opium_pool_isvalid(pool)) {
      return;
   }

   if (opium_bitmask_isvalid(pool->bitmask)) {
      opium_bitmask_destroy(pool->bitmask);
   }

   opium_free(pool->data, NULL);
   opium_flag_false(pool->initialized); 
   opium_free(pool, NULL);
}

   void 
opium_pool_reset(opium_pool_t *pool)
{
   if (!opium_pool_isvalid(pool)) {
      return;
   }

   opium_bitmask_reset(pool->bitmask);
   opium_memzero(pool->data, pool->capacity * pool->block_size);
}

   int
opium_palloc(opium_pool_t *pool, void *data)
{
   /* Allocates the first free block in the pool
    * 
    * @param data Optional data to copy into the allocated block.
    *
    */

   int result = opium_palloc_at(pool, data, -1);
   return result;
}

   int
opium_palloc_at(opium_pool_t *pool, void *data, int index)
{
   /* Allocate a single block in the pool, optionally at a specific index.
    *
    * This function tries to allocate one block for the memory pool.
    * If 'index' is negative, it finds the first free block automatically.
    * Otherwise, it attempts to allocate at the given index.
    *
    * The allocation is tracked in the pool`s bitmask.
    *
    * @param data  Optional pointer to data to copy into the block (can be NULL).
    * @param index Desired index to allocate, or -1 for automatic allocation.
    *
    */

   if (!opium_pool_isvalid(pool)) {
      return OPIUM_RET_ERR;
   }

   size_t capacity = pool->capacity;
   size_t block_size = pool->block_size;

   size_t current = opium_bitmask_count(pool->bitmask);

   if ((index >= 0 && (size_t)index >= capacity) || current >= capacity) {
      return OPIUM_RET_ERR;
   }

   int real = opium_bitmask_push(pool->bitmask, index, 1);
   if (real < 0) {
      return OPIUM_RET_ERR;
   }

   void *ptr = (char*)pool->data + real * block_size;
   if (data) {
      opium_memcpy(ptr, data, block_size);
   } else {
      opium_memzero(ptr, block_size);
   }

   return real;
}

   void 
opium_pool_free(opium_pool_t *pool, int index) 
{
   /* Free a block in the pool by its index.
    *
    * This function releases a previously allocated block at the given 'index'
    * It first check that the pool is valid and that the index is within bounds.
    * 
    * @param index Index of the block to free.
    *
    */

   if (!opium_pool_isvalid(pool) || index < 0 || (size_t)index >= pool->capacity) {
      return;
   }

   if (!opium_bitmask_bit_test(pool->bitmask, index)) {
      return;
   }

   opium_bitmask_pop(pool->bitmask, index, 1);
   opium_memset((char*)pool->data + index * pool->block_size, 0, pool->block_size);
}

   void 
opium_pool_free_ptr(opium_pool_t *pool, void *ptr) 
{
   /* Free a block in the pool by its pointer.
    *
    * @param ptr pointer of the block to free.
    *
    */

   opium_pool_free(pool, opium_pool_index(pool, ptr));
}
