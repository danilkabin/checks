#ifndef OPIUM_ALLOC_INCLUDE_H
#define OPIUM_ALLOC_INCLUDE_H

#include "opium_log.h"

#include <stddef.h>
#include <stdint.h>

void opium_memzero(void *ptr, size_t size);

void *opium_malloc(size_t size, opium_log_t *log);
void *opium_calloc(size_t size, opium_log_t *log);

void opium_free(void *ptr, opium_log_t *log);

void *opium_memalign(size_t alignment, size_t size, opium_log_t *log);

#endif /* OPIUM_ALLOC_INCLUDE_H */
