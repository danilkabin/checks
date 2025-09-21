#ifndef OPIUM_ALLOC_INCLUDE_H
#define OPIUM_ALLOC_INCLUDE_H

#include "opium_log.h"

#include <stddef.h>
#include <stdint.h>

void *opium_malloc(size_t size, opium_log_t *log);
void *opium_calloc(size_t size, opium_log_t *log);
void opium_free(void *ptr, opium_log_t *log);

void *opium_mmap(size_t size, opium_log_t *log);
void opium_munmap(void *data, size_t size, opium_log_t *log);

void *opium_memalign(size_t alignment, size_t size, opium_log_t *log);

void *opium_memcpy(void *dst, void *src, size_t len);

#define opium_memzero(buf, len) (void) (memset)(buf, 0, len)
#define opium_memset(buf, c, len) (void) memset(buf, c, len)
#define opium_memmove(dst, src, n) (void) memmove(dst, src, n)
#define opium_memcmp(s1, s2, len) memcmp(s1, s2, len)

#endif /* OPIUM_ALLOC_INCLUDE_H */
