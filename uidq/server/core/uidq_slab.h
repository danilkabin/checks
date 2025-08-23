#ifndef UIDQ_SLAB_INCLUDE_H
#define UIDQ_SLAB_INCLUDE_H

#include "uidq_list.h"
#include "uidq_log.h"
#include <stddef.h>
#include <stdint.h>

struct uidq_slab_page_t {
    struct uidq_list_head *free;
    uintptr_t data;
    uintptr_t slab;
    size_t count;
    uint32_t index;
};

typedef struct {
   int initialized;

   size_t size;
   size_t count;
   size_t per;
   size_t free_c;
   size_t busy_c;

   struct uidq_slab_page_t *pages;

   void *data;

   uidq_log_t *log;
} uidq_slab_t;

#endif /* UIDQ_SLAB_INCLUDE_H */
