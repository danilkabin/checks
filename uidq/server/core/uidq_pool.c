#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "uidq_pool.h"
#include "core/uidq_bitmask.h"
#include "uidq_alloc.h"
#include "uidq_list.h"
#include "uidq_log.h"
#include "uidq_types.h"

   int 
uidq_pool_isvalid(uidq_pool_t *pool) 
{
   return pool && pool->initialized;
}

   uidq_pool_t *
uidq_pool_create(size_t max, size_t size, uidq_log_t *log)
{
   if (max == 0 || size == 0) {
      return NULL;
   }

   uidq_pool_t *pool = uidq_calloc(sizeof(uidq_pool_t), log);
   if (!pool) {
      return NULL;
   }
   pool->initialized = 1;

   size_t data_size = size * max;

   pool->data = uidq_memalign(UIDQ_POOL_ALIGNMENT_SIZE, data_size, log);
   if (!pool->data) {
      goto fail;
   }

   pool->bitmask = uidq_bitmask_create(max, log);
   if (!pool->bitmask) {
      goto fail;
   }

   pool->max = max;
   pool->size = size;
   pool->log = log;

   return pool;

fail:
   uidq_pool_abort(pool);
   return NULL;
}

   void
uidq_pool_abort(uidq_pool_t *pool)
{
   if (!uidq_pool_isvalid(pool)) return;
   pool->initialized = 0;
   uidq_bitmask_free(pool->bitmask);
   uidq_free(pool->data, NULL);
   uidq_free(pool, NULL);
}

int uidq_pool_push_ex(uidq_pool_t *pool, void *data, int preferred_index) {
   if (!uidq_pool_isvalid(pool)) {
      return UIDQ_RET_ERR;
   }

   int index = uidq_bitmask_push(pool->bitmask, preferred_index, 1);
   if (index < 0) {
      return UIDQ_RET_ERR;
   }

   void *ptr = (char*)pool->data + index * pool->size;
   if (data) {
      memcpy(ptr, data, pool->size);
   } else {
      memset(ptr, 0, pool->size);
   }

   return index;
}

   int 
uidq_pool_push(uidq_pool_t *pool, void *data) 
{
   return uidq_pool_push_ex(pool, data, -1);
}

   int
uidq_pool_pushim(uidq_pool_t *pool, void *data, int index) 
{
   return uidq_pool_push_ex(pool, data, index);
}

   int 
uidq_pool_pop(uidq_pool_t *pool, int index) 
{
   if (!uidq_pool_isvalid(pool)) return UIDQ_RET_ERR;
   if ((size_t)index > pool->max) return UIDQ_RET_ERR; 

   if (uidq_bitmask_bit_test(pool->bitmask, index) == 0) {
      return UIDQ_RET_ERR;
   }

   uidq_bitmask_pop(pool->bitmask, index, 1);
   void *ptr = (char*)pool->data + index * pool->size;
   memset(ptr, 0, pool->size);

   return UIDQ_RET_OK;
}

   void *
uidq_pool_get(uidq_pool_t *pool, int index) 
{ 
   if ((size_t)index > pool->max) return NULL;
   // if (uidq_bitmask_bit_test(pool->bitmask, index) == 0) return NULL;
   return (char*)pool->data + index * pool->size; 
}

int
uidq_pool_block_check(uidq_pool_t *pool, int index) {
   return uidq_bitmask_bit_test(pool->bitmask, index);
}
