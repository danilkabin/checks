#include "opium_core.h"

int
opium_array_init(opium_array_t *array, opium_pool_t *pool, size_t n, size_t size, opium_log_t *log)
{
    assert(array != NULL);
    assert(pool != NULL);
    assert(n > 0);
    assert(size > 0);

    array->elts = opium_pool_push(pool, n * size, -1);
    if (!array->elts) {
        opium_log_err(log, "Failed to allocate array elements: %zu bytes\n", n * size);
        return OPIUM_RET_ERR;
    }

    array->nelts = 0;
    array->nalloc = n;
    array->size = size;
    array->pool = pool;
    array->log = log;

    opium_log_debug(log, "Array initialized: nalloc=%zu, size=%zu\n", n, size);
    return OPIUM_RET_OK;
}

void
opium_array_destroy(opium_array_t *array)
{
    assert(array != NULL);

    if (array->elts) {
        for (size_t index = 0; index < array->nelts; index++) {
            int i = opium_bitmask_ffb(array->pool->bitmask, 0);
            if (i >= 0 && array->pool->elts[i] == (u_char *)array->elts) {
                opium_pool_pop(array->pool, i);
                break;
            }
        }
        array->elts = NULL;
    }

    array->nelts = 0;
    array->nalloc = 0;
    array->size = 0;
    array->pool = NULL;
    array->log = NULL;

    opium_log_debug(array->log, "Array destroyed\n");
}

void *
opium_array_push(opium_array_t *array)
{
    assert(array != NULL);

    if (array->nelts >= array->nalloc) {
        size_t new_nalloc = array->nalloc * 2;
        void *new_elts = opium_pool_push(array->pool, new_nalloc * array->size, -1);
        if (!new_elts) {
            opium_log_err(array->log, "Failed to resize array to %zu elements\n", new_nalloc);
            return NULL;
        }

        memcpy(new_elts, array->elts, array->nelts * array->size);

        for (size_t index = 0; index < array->nelts; index++) {
            int i = opium_bitmask_ffb(array->pool->bitmask, 0);
            if (i >= 0 && array->pool->elts[i] == array->elts) {
                opium_pool_pop(array->pool, i);
                break;
            }
        }

        array->elts = new_elts;
        array->nalloc = new_nalloc;
        opium_log_debug(array->log, "Array resized: nalloc=%zu\n", new_nalloc);
    }

    void *elt = (u_char *)array->elts + array->nelts * array->size;
    array->nelts++;
    return elt;
}

void *
opium_array_push_n(opium_array_t *array, size_t n)
{
    assert(array != NULL);
    assert(n > 0);

    if (array->nelts + n > array->nalloc) {
        size_t new_nalloc = array->nalloc * 2 > array->nelts + n ? array->nalloc * 2 : array->nelts + n;
        void *new_elts = opium_pool_push(array->pool, new_nalloc * array->size, -1);
        if (!new_elts) {
            opium_log_err(array->log, "Failed to resize array to %zu elements\n", new_nalloc);
            return NULL;
        }

        memcpy(new_elts, array->elts, array->nelts * array->size);

        for (size_t index = 0; index < array->nelts; index++) {
            int i = opium_bitmask_ffb(array->pool->bitmask, 0);
            if (i >= 0 && array->pool->elts[i] == array->elts) {
                opium_pool_pop(array->pool, i);
                break;
            }
        }

        array->elts = new_elts;
        array->nalloc = new_nalloc;
        opium_log_debug(array->log, "Array resized: nalloc=%zu\n", new_nalloc);
    }

    void *elt = (u_char *)array->elts + array->nelts * array->size;
    array->nelts = array->nelts + n;
    return elt;
}
