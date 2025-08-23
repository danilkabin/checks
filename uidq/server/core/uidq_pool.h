#ifndef UIDQ_POOL_INCLUDE_H
#define UIDQ_POOL_INCLUDE_H

#include "core/uidq_bitmask.h"
#include "uidq_log.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UIDQ_POOL_ALIGNMENT_SIZE 16
#define UIDQ_POOL_PAGE_SIZE 64

typedef struct uidq_pool_block_s {
    void *data;
} uidq_pool_block_t;

typedef struct {
   int initialized;

   size_t max;
   size_t size;

   void *data;
   uidq_bitmask_t *bitmask;
   
   uidq_log_t *log;
} uidq_pool_t;

uidq_pool_t *uidq_pool_create(size_t max, size_t size, uidq_log_t *log);
void         uidq_pool_abort(uidq_pool_t *pool);

int          uidq_pool_push(uidq_pool_t *pool, void *data);
int          uidq_pool_pushim(uidq_pool_t *pool, void *data, int index);
int          uidq_pool_pop(uidq_pool_t *pool, int index); 

void        *uidq_pool_get(uidq_pool_t *pool, int index);
int          uidq_pool_block_check(uidq_pool_t *pool, int index);

#endif /* UIDQ_POOL_INCLUDE_H */
