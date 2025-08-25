#ifndef UIDQ_POOL_INCLUDE_H
#define UIDQ_POOL_INCLUDE_H

#include "core/uidq_bitmask.h"
#include "uidq_log.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UIDQ_POOL_ALIGNMENT_SIZE 16
#define UIDQ_POOL_PAGE_SIZE 64

typedef struct {
   size_t capacity;
   size_t size;
} uidq_pool_conf_t;

typedef struct uidq_pool_block_s {
    void *data;
} uidq_pool_block_t;

typedef struct {
   int initialized;

   uidq_pool_conf_t conf;
   uidq_bitmask_t *bitmask;
  
   void *data;
   
   uidq_log_t *log;
} uidq_pool_t;

uidq_pool_conf_t *uidq_pool_conf_get(uidq_pool_t *pool);
int uidq_pool_isvalid(uidq_pool_t *pool);

uidq_pool_t *uidq_pool_create(uidq_pool_conf_t *conf, uidq_log_t *log);
void         uidq_pool_abort(uidq_pool_t *pool);

int  uidq_pool_init(uidq_pool_t *pool, uidq_pool_conf_t *conf, uidq_log_t *log); 
void uidq_pool_exit(uidq_pool_t *pool);

int uidq_pool_realloc(uidq_pool_t *pool, size_t new_capacity);

int  uidq_pool_push(uidq_pool_t *pool, void *data);
int  uidq_pool_pushim(uidq_pool_t *pool, void *data, int index);
int  uidq_pool_pop(uidq_pool_t *pool, int index); 

void *uidq_pool_get(uidq_pool_t *pool, int index);
int  uidq_pool_block_check(uidq_pool_t *pool, int index);

#endif /* UIDQ_POOL_INCLUDE_H */
