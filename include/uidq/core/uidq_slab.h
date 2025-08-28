#ifndef UIDQ_SLAB_INCLUDE_H
#define UIDQ_SLAB_INCLUDE_H

#include "core/uidq_bitmask.h"
#include "uidq_list.h"
#include "uidq_log.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

typedef struct uidq_slab_page_s uidq_slab_page_t;
typedef struct uidq_slab_slot_s uidq_slab_slot_t;
typedef struct uidq_slab_stat_s uidq_slab_stat_t;

typedef struct {
   size_t count;
} uidq_slab_conf_t;

struct uidq_slab_stat_s {
   size_t total;
   size_t used;
   size_t reqs;
   size_t fails;
};

struct uidq_slab_page_s {
   struct uidq_list_head head;
   uidq_bitmask_t *bitmask;
};

struct uidq_slab_slot_s {
   struct uidq_list_head pages;
   uidq_slab_stat_t stats;
   size_t shift;
   size_t block_size;
};

typedef struct {
   int initialized;

   size_t size;

   uidq_slab_conf_t conf;

   uidq_slab_slot_t *slots;
   uidq_slab_page_t *pages;
   struct uidq_list_head free;

   void *data;

   uidq_log_t *log;
} uidq_slab_t;

uidq_slab_t *uidq_slab_create(uidq_slab_conf_t *conf, uidq_log_t *log);
void uidq_slab_abort(uidq_slab_t *slab);

int uidq_slab_init(uidq_slab_t *slab, uidq_slab_conf_t *conf, uidq_log_t *log);
void uidq_slab_exit(uidq_slab_t *slab);

void *uidq_slab_push(uidq_slab_t *slab, size_t size);
void uidq_slab_pop(uidq_slab_t *slab, void *ptr);

void uidq_slab_chains_debug(uidq_slab_t *slab);

#endif /* UIDQ_SLAB_INCLUDE_H */
