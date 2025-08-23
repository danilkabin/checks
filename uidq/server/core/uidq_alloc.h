#ifndef UIDQ_ALLOC_INCLUDE_H 
#define UIDQ_ALLOC_INCLUDE_H

#include "uidq_log.h"

#include <stddef.h>
#include <stdint.h>

void uidq_memzero(void *ptr, size_t size);

void *uidq_malloc(size_t size, uidq_log_t *log);
void *uidq_calloc(size_t size, uidq_log_t *log);

void uidq_free(void *ptr, uidq_log_t *log);

void *uidq_memalign(size_t alignment, size_t size, uidq_log_t *log);

#endif /* UIDQ_ALLOC_INCLUDE_H */
