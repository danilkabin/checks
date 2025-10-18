#ifndef OPIUM_ARRAY_INCLUDE_H
#define OPIUM_ARRAY_INCLUDE_H

#include "core/opium_core.h"

struct opium_array_s {
    void         *elts;
    size_t        nelts, nalloc;
    size_t        size;
    
    opium_pool_t *pool;

    opium_log_t  *log;
};

int opium_array_init(opium_array_t *array, opium_pool_t *pool, size_t n, size_t size, opium_log_t *log);
void opium_array_destroy(opium_array_t *array);

void *opium_array_push(opium_array_t *array);
void *opium_array_push_n(opium_array_t *array, size_t n);

#endif /* OPIUM_ARRAY_INCLUDE_H */
