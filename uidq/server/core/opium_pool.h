#ifndef OPIUM_POOL_INCLUDE_H
#define OPIUM_POOL_INCLUDE_H

#include "core/opium_bitmask.h"
#include "core/opium_list.h"
#include "opium_log.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
   void *data;
} opium_pool_block_t;

typedef struct {
   int initialized;

   size_t block_size;
   size_t capacity;

   opium_bitmask_t *bitmask;
   void *data;

   opium_log_t *log;
} opium_pool_t;

/* Validity check */
int opium_pool_isvalid(opium_pool_t *pool);

/* Pool creation terminator 3000 */
opium_pool_t *opium_pool_create(size_t capacity, size_t block_size, opium_log_t *log);
void opium_pool_abort(opium_pool_t *pool);
int opium_pool_init(opium_pool_t *pool, size_t capacity, size_t block_size, opium_log_t *log);
void opium_pool_exit(opium_pool_t *pool);
int opium_pool_realloc(opium_pool_t *pool, size_t new_capacity);

/* Allocate babababy */
void *opium_pool_get(opium_pool_t *pool, int index);
int opium_pool_block_check(opium_pool_t *pool, int index);
int opium_pool_push(opium_pool_t *pool, void *data);
int opium_pool_pushim(opium_pool_t *pool, void *data, int index);
int opium_pool_pop(opium_pool_t *pool, int index);

/* Debug */
void opium_pool_debug(opium_pool_t *pool);

#endif /* OPIUM_POOL_INCLUDE_H */
