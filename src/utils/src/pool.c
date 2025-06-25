#include "pool.h"
#include "listhead.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

struct list_head onion_memoryPool_list = LIST_HEAD_INIT(onion_memoryPool_list);

size_t onion_bs_malloc_current_size = 0;

int onion_memoryPool_init(struct onion_memoryPool **poolPtr, size_t max_size, size_t block_size) {
    int ret = -1;

    *poolPtr = malloc(sizeof(struct onion_memoryPool));
    if (!*poolPtr) {
        DEBUG_FUNC("The pool is null!\n");
        goto free_this_trash;
    }
    struct onion_memoryPool *pool = *poolPtr;

    size_t maxSize = round_size_pow2(max_size);
    size_t blockSize = block_size;
    pool->block_max = maxSize / blockSize;
    size_t bitmap_size = (pool->block_max + 7) / 8;

    pool->current_size = 0;
    pool->block_count = 0;
    pool->block_free = pool->block_max;
    pool->block_size = blockSize;

    pool->bitmap = malloc(bitmap_size);
    if (!pool->bitmap) {
        DEBUG_FUNC("The bitmap is null!\n");
        goto free_pool;
    }
    memset(pool->bitmap, 0, bitmap_size);

    pool->data = malloc(maxSize);
    if (!pool->data) {
        DEBUG_FUNC("The data is null!\n");
        goto free_bitmap;
    }

    ret = 0;
    return ret;

    free_bitmap:
    free(pool->bitmap);
    free_pool:
    free(pool);
    free_this_trash:
    return ret;
}

void onion_memoryPool_free(struct onion_memoryPool *pool) {
    list_del(&pool->list);

    if (pool->data) {
        free(pool->data);
        pool->data = NULL;
    }
    if (pool->bitmap) {
        free(pool->bitmap);
        pool->bitmap = NULL;
    }
    free(pool);
}

void *onion_memoryPool_allocBlock(struct onion_memoryPool *pool) {
    if (!pool) return NULL;
    if (onion_memoryPool_isFull(pool)) {
        DEBUG_FUNC("Memory pool is full!\n");
        return NULL;
    }

    int index = ffb(pool->bitmap, pool->block_max);
    if (index < 0) {
        DEBUG_FUNC("find_first_free_bit was failed!\n");
        return NULL;
    }

    set_bit(pool->bitmap, index);
    pool->block_free--;
    pool->block_count++;
    pool->current_size += pool->block_size;

    onion_bs_malloc_current_size = onion_bs_malloc_current_size + pool->block_size;

    return (void*)((uint8_t*)pool->data + index * pool->block_size);
}

void onion_memoryPool_freeBlock(struct onion_memoryPool *pool, void *ptr) {
    int offset = (uint8_t*)ptr - (uint8_t*)pool->data;
    size_t index = offset / pool->block_size;

    if (index >= pool->block_max) {
        DEBUG_FUNC("index > blockmax\n");
        return;
    }

    clear_bit(pool->bitmap, index);
    pool->block_free++;
    pool->block_count--;
    pool->current_size -= pool->block_size;

    onion_bs_malloc_current_size = onion_bs_malloc_current_size - pool->block_size;
}

int onion_memoryPool_isFull(struct onion_memoryPool *pool) {
    return pool->block_free == 0;
}

int onion_memoryPool_isFree(struct onion_memoryPool *pool) {
    return pool->block_free > 0;
}

size_t onion_memoryPool_getBusySize(struct onion_memoryPool *pool) {
    return pool->block_count * pool->block_size;
}

size_t onion_memoryPool_getFreeSize(struct onion_memoryPool *pool) {
    return pool->block_free * pool->block_size;
}

int onion_memoryPools_init() {
    INIT_LIST_HEAD(&onion_memoryPool_list);
    return 0;
}

void onion_memoryPools_release() {
    struct onion_memoryPool *node, *tmp;
    list_for_each_entry_safe(node, tmp, &onion_memoryPool_list, list) {
        DEBUG_FUNC("pool deleting! \n");
        onion_memoryPool_free(node);
    }
}
