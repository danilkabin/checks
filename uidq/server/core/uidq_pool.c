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

#define UIDQ_PL_DEFAULT_CAPACITY 64
#define UIDQ_PL_DEFAULT_SIZE     16

uidq_pool_conf_t *
uidq_pool_conf_get(uidq_pool_t *pool) {
   if (!uidq_pool_isvalid(pool)) return NULL;
   return &pool->conf;
}

   int
uidq_pool_isvalid(uidq_pool_t *pool) 
{
   return pool && pool->initialized == 1;
}

size_t
uidq_pool_get_size(uidq_pool_t *pool) {
   uidq_pool_conf_t *config = uidq_pool_conf_get(pool);
   return config->size * config->capacity;
}

   uidq_pool_t *
uidq_pool_create(uidq_pool_conf_t *conf, uidq_log_t *log)
{
   uidq_pool_t *pool = uidq_calloc(sizeof(uidq_pool_t), log);
   if (!pool) {
      return NULL;
   }
   pool->initialized = 1;

   if (uidq_pool_init(pool, conf, log) != UIDQ_RET_OK) {
      uidq_err(log, "Failed to init pool struct.\n"); 
      uidq_pool_abort(pool);
      return NULL;
   }

   return pool;
}

   void
uidq_pool_abort(uidq_pool_t *pool)
{
   if (!uidq_pool_isvalid(pool)) return;
   uidq_pool_exit(pool);
   pool->initialized = 0;
   uidq_free(pool, NULL);
}

int
uidq_pool_init(uidq_pool_t *pool, uidq_pool_conf_t *conf, uidq_log_t *log) {
   if (!uidq_pool_isvalid(pool)) return UIDQ_RET_ERR;
   if (!conf) return UIDQ_RET_ERR;

   uidq_pool_conf_t *config = uidq_pool_conf_get(pool);

   config->capacity = conf->capacity > 0 ? conf->capacity : UIDQ_PL_DEFAULT_CAPACITY;
   config->size = conf->size > 0 ? conf->size : UIDQ_PL_DEFAULT_SIZE;

   size_t data_size = uidq_pool_get_size(pool);

   pool->data = uidq_memalign(UIDQ_POOL_ALIGNMENT_SIZE, data_size, log);
   if (!pool->data) {
      goto fail;
   }

   uidq_bitmask_conf_t bm_conf = {.capacity = config->capacity};
   pool->bitmask = uidq_bitmask_create(&bm_conf, log);
   if (!pool->bitmask) {
      goto fail;
   }

   pool->log = log;

   return UIDQ_RET_OK;

fail:
   uidq_pool_exit(pool);
   return UIDQ_RET_ERR;
}

void
uidq_pool_exit(uidq_pool_t *pool) {
   if (!uidq_pool_isvalid(pool)) return;
   uidq_pool_conf_t *config = uidq_pool_conf_get(pool);

   config->capacity = 0;
   config->size = 0;

   pool->log = NULL;

   if (uidq_bitmask_isvalid(pool->bitmask)) {
      uidq_bitmask_abort(pool->bitmask);
      pool->bitmask = NULL;
   }

   if (pool->data) {
      uidq_free(pool->data, NULL);
      pool->data = NULL;
   }
}

int
uidq_pool_realloc(uidq_pool_t *pool, size_t new_capacity) {
   if (!uidq_pool_isvalid(pool)) return UIDQ_RET_ERR;
   uidq_pool_conf_t *config = uidq_pool_conf_get(pool);

   size_t old_size = uidq_pool_get_size(pool);
   size_t old_capacity = config->capacity; 

   uidq_bitmask_realloc(pool->bitmask, new_capacity);

   size_t new_size = new_capacity * config->size;
   void *new_data = uidq_memalign(UIDQ_POOL_ALIGNMENT_SIZE, new_size, pool->log);
   if (!new_data) return UIDQ_RET_ERR;

   if (pool->data) {
      for (size_t index = 0; index < old_capacity; index++) {
         if (uidq_pool_block_check(pool, index)) {
            memcpy((char*)new_data + index * config->size, (char*)pool->data + index * config->size, config->size);
         }
      }
      uidq_free(pool->data, pool->log);
   }

   pool->data = new_data;
   config->capacity = new_capacity;

   return UIDQ_RET_OK;
}

   int
uidq_pool_push_ex(uidq_pool_t *pool, void *data, int preferred_index) 
{
   uidq_pool_conf_t *conf = uidq_pool_conf_get(pool);
   if (!uidq_pool_isvalid(pool)) {
      return UIDQ_RET_ERR;
   }

   int index = uidq_bitmask_push(pool->bitmask, preferred_index, 1);
   if (index < 0) {
      return UIDQ_RET_ERR;
   }

   void *ptr = (char*)pool->data + index * conf->size;
   if (data) {
      memcpy(ptr, data, conf->size);
   } else {
      memset(ptr, 0, conf->size);
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
   uidq_pool_conf_t *conf = uidq_pool_conf_get(pool);
   if (!uidq_pool_isvalid(pool)) return UIDQ_RET_ERR;
   if ((size_t)index > conf->capacity) return UIDQ_RET_ERR; 

   if (uidq_bitmask_bit_test(pool->bitmask, index) == 0) {
      return UIDQ_RET_ERR;
   }

   uidq_bitmask_pop(pool->bitmask, index, 1);
   void *ptr = (char*)pool->data + index * conf->size;
   memset(ptr, 0, conf->size);

   return UIDQ_RET_OK;
}

   void *
uidq_pool_get(uidq_pool_t *pool, int index) 
{
   uidq_pool_conf_t *conf = uidq_pool_conf_get(pool); 
   if ((size_t)index > conf->capacity) return NULL;
   if (!uidq_pool_block_check(pool, index)) return NULL;
   return (char*)pool->data + index * conf->size; 
}

int
uidq_pool_block_check(uidq_pool_t *pool, int index) {
   return uidq_bitmask_bit_test(pool->bitmask, index);
}
