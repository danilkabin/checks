#ifndef onion_slab_H
#define onion_slab_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

struct onion_slab_block {
   size_t used;
   size_t allocation_size;
   int is_start;
};

struct onion_slab {
    void *pool;
    size_t pool_size;

    struct onion_slab_block *blocks;
    size_t block_capacity;
    size_t block_size;
    size_t block_used;

    uint64_t *bitmask;
    size_t bitmask_size;

    size_t memory_allocated;
};

struct onion_slab *onion_slab_init(size_t);
void onion_slab_exit(struct onion_slab*);
void *onion_slab_malloc(struct onion_slab*, void *, size_t);
void *onion_slab_realloc(struct onion_slab*, void *, size_t);
void onion_slab_free(struct onion_slab*, void*);

void onion_slab_free_and_null(struct onion_slab*, void **);

int onion_slab_get_endpoint(struct onion_slab *, int start);

#endif
