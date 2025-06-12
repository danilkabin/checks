#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

struct slab_block {
   size_t used;
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
};

struct slab *slab_init(size_t);
void slab_add(struct slab*, void *, size_t, int *, int *);
void slab_del(struct slab*, int, int);
#endif
