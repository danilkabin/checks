#ifndef UIDQ_ALLOC_H
#define UIDQ_ALLOC_H

#include <stddef.h>

#define uidq_memzero(a,c) memset(a,0,c);

void *uidq_malloc(size_t size);
void *uidq_calloc(size_t size);
void uidq_free(void *ptr);

#endif
