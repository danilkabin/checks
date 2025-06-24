#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

struct slab_block {
   size_t used;
   size_t allocation_size;
   int is_start;
};

struct slab {
    void *pool;
    size_t pool_size;

    struct slab_block *blocks;
    size_t block_capacity;
    size_t block_size;
    size_t block_used;

    u_int8_t *bitmask;
    size_t bitmask_size;

    size_t memory_allocated;
};

struct slab *slab_init(size_t);
void slab_exit(struct slab*);
void *slab_malloc(struct slab*, void *, size_t);
void *slab_realloc(struct slab*, void *, size_t);
void slab_free(struct slab*, void*);

void slab_free_and_null(struct slab*, void **);

#endif
