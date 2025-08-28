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
#include "core/uidq_core.h"

#define UIDQ_PL_DEFAULT_CAPACITY 64
#define UIDQ_PL_DEFAULT_SIZE     16

#define UIDQ_POOL_ALIGNMENT_SIZE 16
#define UIDQ_POOL_PAGE_SIZE 64

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
   if (!config) {
      uidq_err(pool->log, "Failed to get config in uidq_pool_get_size\n");
      return 0;
   }
   size_t size = config->size * config->capacity;
   uidq_debug(pool->log, "Pool size calculated: %zu\n", size);
   return size;
}

uidq_pool_t *
uidq_pool_create(uidq_pool_conf_t *conf, uidq_log_t *log) {
   uidq_pool_t *pool = uidq_calloc(sizeof(uidq_pool_t), log);
   if (!pool) {
      uidq_err(log, "Failed to allocate pool struct\n");
      return NULL;
   }
   pool->initialized = 1;
   uidq_debug(log, "Pool allocated and initialized\n");

   if (uidq_pool_init(pool, conf, log) != UIDQ_RET_OK) {
      uidq_err(log, "Failed to init pool struct\n"); 
      uidq_pool_abort(pool);
      return NULL;
   }

   return pool;
}

void
uidq_pool_abort(uidq_pool_t *pool) {
   if (!uidq_pool_isvalid(pool)) {
      uidq_err(pool->log, "Invalid pool in uidq_pool_abort\n");
      return;
   }
   uidq_pool_exit(pool);
   pool->initialized = 0;
   uidq_free(pool, NULL);
}

int
uidq_pool_init(uidq_pool_t *pool, uidq_pool_conf_t *conf, uidq_log_t *log) {
   if (!uidq_pool_isvalid(pool)) {
      uidq_err(log, "Invalid pool in uidq_pool_init\n");
      return UIDQ_RET_ERR;
   }
   if (!conf) {
      uidq_err(log, "Invalid config in uidq_pool_init\n");
      return UIDQ_RET_ERR;
   }

   uidq_pool_conf_t *config = uidq_pool_conf_get(pool);
   if (!config) {
      uidq_err(log, "Failed to get config in uidq_pool_init\n");
      return UIDQ_RET_ERR;
   }

   config->capacity = conf->capacity > 0 ? conf->capacity : UIDQ_PL_DEFAULT_CAPACITY;
   config->size = conf->size > 0 ? conf->size : UIDQ_PL_DEFAULT_SIZE;
   uidq_debug(log, "Pool config set: capacity=%zu, size=%zu\n", config->capacity, config->size);

   size_t data_size = uidq_pool_get_size(pool);
   uidq_debug(log, "Data size for allocation: %zu\n", data_size);

   pool->data = uidq_memalign(UIDQ_POOL_ALIGNMENT_SIZE, data_size, log);
   if (!pool->data) {
      uidq_err(log, "Failed to allocate pool data\n");
      goto fail;
   }
   uidq_debug(log, "Pool data allocated\n");

   uidq_bitmask_conf_t bm_conf = {.capacity = config->capacity};
   pool->bitmask = uidq_bitmask_create(&bm_conf, log);
   if (!pool->bitmask) {
      uidq_err(log, "Failed to create bitmask\n");
      goto fail;
   }
   uidq_debug(log, "Bitmask created\n");

   pool->log = log;
   return UIDQ_RET_OK;

fail:
   uidq_pool_exit(pool);
   return UIDQ_RET_ERR;
}

void
uidq_pool_exit(uidq_pool_t *pool) {
   if (!uidq_pool_isvalid(pool)) {
      uidq_err(pool->log, "Invalid pool in uidq_pool_exit\n");
      return;
   }
   uidq_pool_conf_t *config = uidq_pool_conf_get(pool);
   if (!config) {
      uidq_err(pool->log, "Failed to get config in uidq_pool_exit\n");
      return;
   }

   config->capacity = 0;
   config->size = 0;
   uidq_debug(pool->log, "Pool config reset\n");

   pool->log = NULL;

   if (uidq_bitmask_isvalid(pool->bitmask)) {
      uidq_bitmask_abort(pool->bitmask);
      uidq_debug(pool->log, "Bitmask aborted\n");
   }

   if (pool->data) {
      uidq_free(pool->data, NULL);
      pool->data = NULL;
      uidq_debug(pool->log, "Pool data freed\n");
   }
}

int
uidq_pool_realloc(uidq_pool_t *pool, size_t new_capacity) {
   if (!uidq_pool_isvalid(pool)) {
      uidq_err(pool->log, "Invalid pool in uidq_pool_realloc\n");
      return UIDQ_RET_ERR;
   }
   uidq_pool_conf_t *config = uidq_pool_conf_get(pool);
   if (!config) {
      uidq_err(pool->log, "Failed to get config in uidq_pool_realloc\n");
      return UIDQ_RET_ERR;
   }

   size_t old_size = uidq_pool_get_size(pool);
   size_t old_capacity = config->capacity; 
   uidq_debug(pool->log, "Old capacity=%zu, old size=%zu\n", old_capacity, old_size);

   uidq_bitmask_realloc(pool->bitmask, new_capacity);
   uidq_debug(pool->log, "Bitmask reallocated\n");

   size_t new_size = new_capacity * config->size;
   void *new_data = uidq_memalign(UIDQ_POOL_ALIGNMENT_SIZE, new_size, pool->log);
   if (!new_data) {
      uidq_err(pool->log, "Failed to allocate new data in uidq_pool_realloc\n");
      return UIDQ_RET_ERR;
   }
   uidq_debug(pool->log, "New data allocated, size=%zu\n", new_size);

   if (pool->data) {
      for (size_t index = 0; index < old_capacity; index++) {
         if (uidq_pool_block_check(pool, index)) {
            memcpy((char*)new_data + index * config->size, (char*)pool->data + index * config->size, config->size);
            uidq_debug(pool->log, "Copied data at index=%zu\n", index);
         }
      }
      uidq_free(pool->data, pool->log);
      uidq_debug(pool->log, "Old data freed\n");
   }

   pool->data = new_data;
   config->capacity = new_capacity;
   return UIDQ_RET_OK;
}

int
uidq_pool_push_ex(uidq_pool_t *pool, void *data, int preferred_index) {
   uidq_pool_conf_t *conf = uidq_pool_conf_get(pool);
   if (!uidq_pool_isvalid(pool)) {
      uidq_err(pool->log, "Invalid pool in uidq_pool_push_ex\n");
      return UIDQ_RET_ERR;
   }

   int index = uidq_bitmask_push(pool->bitmask, preferred_index, 1);
   if (index < 0) {
      uidq_err(pool->log, "Failed to push to bitmask\n");
      return UIDQ_RET_ERR;
   }
   uidq_debug(pool->log, "Bitmask push successful, index=%d\n", index);

   void *ptr = (char*)pool->data + index * conf->size;
   if (data) {
      memcpy(ptr, data, conf->size);
      uidq_debug(pool->log, "Data copied to pool at index=%d\n", index);
   } else {
      memset(ptr, 0, conf->size);
      uidq_debug(pool->log, "Data zeroed at index=%d\n", index);
   }

   return index;
}

int 
uidq_pool_push(uidq_pool_t *pool, void *data) {
   int result = uidq_pool_push_ex(pool, data, -1);
   return result;
}

int
uidq_pool_pushim(uidq_pool_t *pool, void *data, int index) {
   int result = uidq_pool_push_ex(pool, data, index);
   return result;
}

int 
uidq_pool_pop(uidq_pool_t *pool, int index) {
   uidq_pool_conf_t *conf = uidq_pool_conf_get(pool);
   if (!uidq_pool_isvalid(pool)) {
      uidq_err(pool->log, "Invalid pool in uidq_pool_pop\n");
      return UIDQ_RET_ERR;
   }
   if ((size_t)index > conf->capacity) {
      uidq_err(pool->log, "Index %d exceeds capacity %zu\n", index, conf->capacity);
      return UIDQ_RET_ERR;
   }

   if (uidq_bitmask_bit_test(pool->bitmask, index) == 0) {
      uidq_err(pool->log, "No data at index %d\n", index);
      return UIDQ_RET_ERR;
   }

   uidq_bitmask_pop(pool->bitmask, index, 1);
   void *ptr = (char*)pool->data + index * conf->size;
   memset(ptr, 0, conf->size);
   uidq_debug(pool->log, "Data popped and zeroed at index=%d\n", index);

   return UIDQ_RET_OK;
}

void *
uidq_pool_get(uidq_pool_t *pool, int index) {
   uidq_pool_conf_t *conf = uidq_pool_conf_get(pool); 
   if ((size_t)index > conf->capacity) {
      uidq_err(pool->log, "Index %d exceeds capacity %zu\n", index, conf->capacity);
      return NULL;
   }
   if (!uidq_pool_block_check(pool, index)) {
      uidq_err(pool->log, "No valid block at index %d\n", index);
      return NULL;
   }
   void *ptr = (char*)pool->data + index * conf->size;
   uidq_debug(pool->log, "Retrieved data at index=%d\n", index);
   return ptr;
}

int
uidq_pool_block_check(uidq_pool_t *pool, int index) {
   uidq_debug(pool->log, "Checking block at index=%d\n", index);
   int result = uidq_bitmask_bit_test(pool->bitmask, index);
   uidq_debug(pool->log, "Block check result: %d\n", result);
   return result;
}
