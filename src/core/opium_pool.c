#include "core/opium_core.h"
#include <sys/types.h>

#define OPIUM_POOL_CHECK(pool, idx) assert((idx) < (int)(pool)->max_elts)

   static void *
opium_pool_allocate_slot(opium_pool_t *pool, size_t index, void *ptr)
{
   if (ptr) {
      opium_bitmask_set(pool->bitmask, index);
      pool->nelts = pool->nelts + 1;
   } else {
      opium_bitmask_clear(pool->bitmask, index);
      pool->nelts = pool->nelts - 1;
   }
   return pool->elts[index] = ptr;
}

   int
opium_pool_init(opium_pool_t *pool, size_t max_elts, opium_log_t *log)
{
   assert(pool != NULL);

   pool->elts = opium_calloc(max_elts * sizeof(void *), log);
   if (!pool->elts) {
      opium_log_err(log, "Failed to allocate pool elements.\n");
      return OPIUM_RET_ERR;
   }

   int ret = opium_arena_init(&pool->data, log);
   if (ret != OPIUM_RET_OK) {
      goto free_elts;
   }

   pool->bitmask = opium_bitmask_create(&pool->data, max_elts);
   if (!pool->bitmask) {
      opium_log_err(log, "Failed to allocate bitmask.\n");
      goto free_data;
   }

   pool->max_elts = max_elts;
   pool->nelts = 0;
   pool->log = log;

   return OPIUM_RET_OK;

free_data:
   opium_arena_exit(&pool->data);
free_elts:
   opium_free(pool->elts, log);
   return OPIUM_RET_ERR;
}

   void
opium_pool_exit(opium_pool_t *pool)
{
   assert(pool != NULL);

   opium_free(pool->elts, pool->log);
   opium_bitmask_destroy(pool->bitmask);
   opium_arena_exit(&pool->data);

   pool->elts = NULL;
   pool->nelts = 0;
   pool->max_elts = 0;
   pool->log = NULL;
}

   void *
opium_pool_push(opium_pool_t *pool, size_t size, int index)
{
   assert(pool != NULL);
   OPIUM_POOL_CHECK(pool, index);

   if (pool->nelts >= pool->max_elts) {
      return NULL;
   }

   if (index < 0) {
      if ((index = opium_bitmask_ffb(pool->bitmask, 0)) < 0) {
         return NULL;
      }
   } else if (opium_bitmask_bit_is_set(pool->bitmask, index)) {
      return NULL;
   }

   void *ptr = opium_arena_alloc(&pool->data, size);
   return ptr ? opium_pool_allocate_slot(pool, index, ptr) : NULL;
}

   void
opium_pool_pop(opium_pool_t *pool, int index)
{
   assert(pool != NULL);
   OPIUM_POOL_CHECK(pool, index);

   if (pool->nelts == 0) {
      return;
   }

   if (opium_bitmask_bit_is_set(pool->bitmask, index)) {
      u_char *ptr = pool->elts[index];
      printf("delete ptr: %s\n", ptr);
      opium_arena_free(&pool->data, pool->elts[index]);
      opium_pool_allocate_slot(pool, index, NULL);
   }
}

   void *
opium_pool_get(opium_pool_t *pool, int index)
{
   assert(pool != NULL);
   OPIUM_POOL_CHECK(pool, index);

   return pool->elts[index]; 
}
