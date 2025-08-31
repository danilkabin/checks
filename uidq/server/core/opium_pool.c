#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "opium_pool.h"
#include "core/opium_bitmask.h"
#include "opium_alloc.h"
#include "opium_list.h"
#include "opium_log.h"
#include "core/opium_core.h"

#define OPIUM_PL_DEFAULT_CAPACITY 64
#define OPIUM_PL_DEFAULT_SIZE     16

#define OPIUM_POOL_ALIGNMENT_SIZE 16
#define OPIUM_POOL_PAGE_SIZE      64

static int opium_pool_push_ex(opium_pool_t *pool, void *data, int preferred_index);

   int
opium_pool_isvalid(opium_pool_t *pool)
{
   return pool && pool->initialized == 1;
}

static size_t
opium_pool_get_size(opium_pool_t *pool) {
   if (!opium_pool_isvalid(pool)) {
      return OPIUM_RET_ERR;
   }

   size_t size = pool->block_size * pool->capacity;
   return size;
}

void *
opium_pool_get(opium_pool_t *pool, int index) {
   if (!opium_pool_isvalid(pool)) {
      return NULL;
   }

   size_t capacity = pool->capacity;
   size_t block_size = pool->block_size;

   if (index < 0 || (size_t)index >= capacity) {
      opium_err(pool->log, "Index %d exceeds capacity %zu\n", index, capacity);
      return NULL;
   }

   if (!opium_pool_block_check(pool, index)) {
      return NULL;
   }

   void *ptr = (char*)pool->data + index * block_size;
   return ptr;
}

int
opium_pool_block_check(opium_pool_t *pool, int index) {
   if (!opium_pool_isvalid(pool)) {
      return OPIUM_RET_ERR;
   }

   size_t capacity = pool->capacity;

   if (index < 0 || (size_t)index >= capacity) {
      opium_err(pool->log, "Index %d exceeds capacity %zu\n", index, capacity);
      return OPIUM_RET_ERR;
   }

   index = opium_bitmask_bit_test(pool->bitmask, index); 
   return index;
}

opium_pool_t *
opium_pool_create(size_t capacity, size_t block_size, opium_log_t *log) {
   opium_pool_t *pool = opium_calloc(sizeof(opium_pool_t), log);
   if (!pool) {
      opium_err(log, "Failed to allocate pool struct\n");
      return NULL;
   }
   pool->initialized = 1;

   if (opium_pool_init(pool, capacity, block_size, log) != OPIUM_RET_OK) {
      opium_err(log, "Failed to init pool struct\n");
      opium_pool_abort(pool);
      return NULL;
   }

   return pool;
}

void
opium_pool_abort(opium_pool_t *pool) {
   if (!opium_pool_isvalid(pool)) {
      return;
   }

   opium_pool_exit(pool);
   pool->initialized = 0;
   opium_free(pool, NULL);
}

int
opium_pool_init(opium_pool_t *pool, size_t capacity, size_t block_size, opium_log_t *log) {
   if (!opium_pool_isvalid(pool)) {
      return OPIUM_RET_ERR;
   }

   if (capacity == 0 || block_size == 0) {
      opium_err(log, "%s: Invalid parameters\n", __func__);
      return OPIUM_RET_ERR;
   }

   pool->capacity = capacity;
   pool->block_size = block_size;

   size_t data_size = opium_pool_get_size(pool);

   pool->data = opium_memalign(OPIUM_POOL_ALIGNMENT_SIZE, data_size, log);
   if (!pool->data) {
      opium_err(log, "Failed to allocate pool data\n");
      goto fail;
   }

   pool->bitmask = opium_bitmask_create(pool->capacity, log);
   if (!pool->bitmask) {
      opium_err(log, "Failed to create bitmask\n");
      goto fail;
   }

   pool->log = log;
   return OPIUM_RET_OK;

fail:
   opium_pool_exit(pool);
   return OPIUM_RET_ERR;
}

void
opium_pool_exit(opium_pool_t *pool) {
   if (!opium_pool_isvalid(pool)) {
      return;
   }

   pool->capacity = 0;
   pool->block_size = 0;

   pool->log = NULL;

   if (opium_bitmask_isvalid(pool->bitmask)) {
      opium_bitmask_abort(pool->bitmask);
   }

   if (pool->data) {
      opium_free(pool->data, NULL);
      pool->data = NULL;
   }
}

int
opium_pool_realloc(opium_pool_t *pool, size_t new_capacity) {
   if (!opium_pool_isvalid(pool)) {
      return OPIUM_RET_ERR;
   }

   if (new_capacity == 0) {
      opium_err(pool->log, "%s: Invalid parameters\n", __func__);
      return OPIUM_RET_ERR;
   }

   size_t block_size = pool->block_size;
   size_t old_capacity = pool->capacity; 

   int bm_result = opium_bitmask_realloc(pool->bitmask, new_capacity);
   if (bm_result != OPIUM_RET_OK) {
      opium_err(pool->log, "%s: Failed to realloc bitmask\n", __func__);
      return OPIUM_RET_ERR;
   }

   size_t new_size = new_capacity * block_size;
   void *new_data = opium_memalign(OPIUM_POOL_ALIGNMENT_SIZE, new_size, pool->log);
   if (!new_data) {
      opium_err(pool->log, "Failed to allocate new data in opium_pool_realloc\n");
      return OPIUM_RET_ERR;
   }

   if (pool->data) {
      for (size_t index = 0; index < old_capacity; index++) {
         if (opium_pool_block_check(pool, index)) {
            memcpy((char*)new_data + index * block_size, (char*)pool->data + index * block_size, block_size);
            //opium_debug(pool->log, "Copied data at index=%zu\n", index);
         }
      }
      opium_free(pool->data, pool->log);
   }

   pool->data = new_data;
   pool->capacity = new_capacity;
   return OPIUM_RET_OK;
}

int 
opium_pool_push(opium_pool_t *pool, void *data) {
   int result = opium_pool_push_ex(pool, data, -1);
   return result;
}

int
opium_pool_pushim(opium_pool_t *pool, void *data, int index) {
   int result = opium_pool_push_ex(pool, data, index);
   return result;
}

int 
opium_pool_pop(opium_pool_t *pool, int index) {
   if (!opium_pool_isvalid(pool)) {
      return OPIUM_RET_ERR;
   }

   size_t capacity = pool->capacity;
   size_t block_size = pool->block_size;

   if (index < 0 || (size_t)index >= capacity) {
      opium_err(pool->log, "Index %d exceeds capacity %zu\n", index, capacity);
      return OPIUM_RET_ERR;
   }

   if (opium_bitmask_bit_test(pool->bitmask, index) == 0) {
      return OPIUM_RET_ERR;
   }

   opium_bitmask_pop(pool->bitmask, index, 1);
   void *ptr = (char*)pool->data + index * block_size;
   memset(ptr, 0, block_size);

   return OPIUM_RET_OK;
}

void
opium_pool_debug(opium_pool_t *pool) {
   if (!opium_pool_isvalid(pool)) {
      return;
   }

   size_t used = 0;
   for (size_t i = 0; i < pool->capacity; i++) {
      if (opium_bitmask_bit_test(pool->bitmask, i)) {
         used++;
      }
   }

   opium_debug(pool->log, "[POOL DEBUG] capacity=%zu block_size=%zu used=%zu free=%zu\n",
         pool->capacity,
         pool->block_size,
         used,
         pool->capacity - used);

   opium_debug(pool->log, "[POOL DEBUG] bitmask dump:");
   for (size_t i = 0; i < pool->capacity; i++) {
      int state = opium_bitmask_bit_test(pool->bitmask, i);
      opium_debug_inline(pool->log, "%c", state ? '1' : '0'); 
   }
   opium_debug_inline(pool->log, "\n");
}

static int
opium_pool_push_ex(opium_pool_t *pool, void *data, int preferred_index) {
   if (!opium_pool_isvalid(pool)) {
      return OPIUM_RET_ERR;
   }

   size_t capacity = pool->capacity;
   size_t block_size = pool->block_size;

   if (preferred_index >= 0 && (size_t)preferred_index >= capacity) {
      opium_err(pool->log, "Index %d exceeds capacity %zu\n", preferred_index, capacity);
      return OPIUM_RET_ERR;
   }

   int index = opium_bitmask_push(pool->bitmask, preferred_index, 1);
   if (index < 0) {
      return OPIUM_RET_ERR;
   }

   void *ptr = (char*)pool->data + index * block_size;
   if (data) {
      memcpy(ptr, data, block_size);
   } else {
      //memset(ptr, 0, block_size);
   }

   return index;
}
