#ifndef OPIUM_ARENA_INCLUDE_H
#define OPIUM_ARENA_INCLUDE_H

#include "core/opium_core.h"
#include "core/opium_alloc.h"
#include "core/opium_list.h"
#include "core/opium_log.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#if (OPIUM_PTR_SIZE == 4) 
/* All 32 bits set -> all slots used */

#else
/* All 64 bits set -> all slots used */

#endif

typedef struct opium_arena_stat_s opium_arena_stat_t;

struct opium_arena_stat_s {
   size_t total;
   size_t used;

   size_t reqs;
   size_t fails;
};

typedef struct opium_arena_page_s opium_arena_page_t;

struct opium_arena_page_s {
};


typedef struct {

   opium_log_t *log;
} opium_arena_t;

typedef void (*opium_slab_trav_ctx)(void *data);

/* API */

/* Lifecycle */
void opium_slab_init(opium_arena_t *arena, size_t item_size, opium_log_t *log);
void opium_slab_exit(opium_arena_t *arena);

#endif /* OPIUM_ARENA_INCLUDE_H */
