#ifndef OPIUM_SLAB_INCLUDE_H
#define OPIUM_SLAB_INCLUDE_H

#include "core/opium_bitmask.h"
#include "opium_list.h"
#include "opium_log.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

typedef struct opium_slab_page_s opium_slab_page_t;
typedef struct opium_slab_slot_s opium_slab_slot_t;
typedef struct opium_slab_stat_s opium_slab_stat_t;

typedef struct {
   size_t count;
} opium_slab_conf_t;

struct opium_slab_stat_s {
   size_t total;
   size_t used;

   size_t reqs;
   size_t fails;
};

struct opium_slab_page_s {
   struct opium_list_head head;
   opium_bitmask_t *bitmask;
   unsigned tag:1;
};

struct opium_slab_slot_s {
   struct opium_list_head pages;
   opium_slab_stat_t stats;
   size_t shift;
   size_t block_size;
};

typedef struct {
   int initialized;

   size_t size;

   opium_slab_conf_t conf;

   opium_slab_slot_t *slots;
   opium_slab_page_t *pages;

   struct opium_list_head free;
   size_t free_count;

   void *data;

   opium_log_t *log;
} opium_slab_t;

opium_slab_t *opium_slab_create(opium_slab_conf_t *conf, opium_log_t *log);
void opium_slab_abort(opium_slab_t *slab);

int opium_slab_init(opium_slab_t *slab, opium_slab_conf_t *conf, opium_log_t *log);
void opium_slab_exit(opium_slab_t *slab);

void *opium_slab_push(opium_slab_t *slab, size_t size);
void opium_slab_pop(opium_slab_t *slab, void *ptr);

void opium_slab_chains_debug(opium_slab_t *slab);

#endif /* OPIUM_SLAB_INCLUDE_H */
