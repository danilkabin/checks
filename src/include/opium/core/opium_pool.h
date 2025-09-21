#ifndef OPIUM_POOL_INCLUDE_H
#define OPIUM_POOL_INCLUDE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/opium_core.h"
#include "core/opium_alloc.h"
#include "core/opium_bitmask.h"
#include "core/opium_log.h"

typedef struct {
   void *data;
} opium_pool_block_t;

typedef struct {
   opium_flag_t initialized;

   size_t block_size;
   size_t capacity;

   opium_bitmask_t *bitmask;
   void *data;

   opium_log_t *log;
} opium_pool_t;

/* Validity check */
int opium_pool_isvalid(opium_pool_t *pool);

/* Access */
void *opium_pool_get(opium_pool_t *pool, int index);
int opium_pool_index(opium_pool_t *pool, void *ptr);

/* Lifecycle */
opium_pool_t *opium_pool_create(size_t capacity, size_t block_size, opium_log_t *log);
void opium_pool_destroy(opium_pool_t *pool);
void opium_pool_reset(opium_pool_t *pool);

/* Allocate babababy */
int  opium_palloc(opium_pool_t *pool, void *data);
int  opium_palloc_at(opium_pool_t *pool, void *data, int index);
void opium_pool_free(opium_pool_t *pool, int index);
void opium_pool_free_ptr(opium_pool_t *pool, void *ptr);

/* Debug */
void opium_pool_debug(opium_pool_t *pool);

#endif /* OPIUM_POOL_INCLUDE_H */
