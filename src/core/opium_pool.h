#ifndef OPIUM_POOL_INCLUDE_H
#define OPIUM_POOL_INCLUDE_H

#include "core/opium_core.h"

typedef struct opium_pool_block_s opium_pool_block_t;

struct opium_pool_block_s {
   u_char *data;
   size_t index;
};

struct opium_pool_s {
   void **elts; 
   size_t nelts;
   size_t max_elts;

   opium_arena_t data;
   opium_bitmask_t *bitmask;

   opium_log_t *log;
};

int opium_pool_init(opium_pool_t *pool, size_t max_elts, opium_log_t *log);
void opium_pool_exit(opium_pool_t *pool);

void *opium_pool_push(opium_pool_t *pool, size_t size, int index);
void opium_pool_pop(opium_pool_t *pool, int index);

void *opium_pool_get(opium_pool_t *pool, int index);

#endif /* OPIUM_POOL_INCLUDE_H */

