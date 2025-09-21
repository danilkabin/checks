#ifndef OPIUM_ARENA_INCLUDE_H
#define OPIUM_ARENA_INCLUDE_H

#include "core/opium_core.h"
#include "core/opium_alloc.h"
#include "core/opium_list.h"
#include "core/opium_log.h"
#include "opium_slab.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

/* Define the "mask" for free/used slots depending on pointer size */
#if (OPIUM_PTR_SIZE == 4) 
/* All 32 bits set */

#define OPIUM_ARENA_MAX_SHIFT 16
#define OPIUM_ARENA_MIN_SHIFT 4

#else
/* All 64 bits set */

#define OPIUM_ARENA_MAX_SHIFT 16
#define OPIUM_ARENA_MIN_SHIFT 4

#endif

typedef struct opium_arena_page_s opium_arena_page_t;

struct opium_arena_page_s {
   opium_ubyte_t index;
   void *ptr;
};

typedef struct {
   size_t shift_count;

   size_t min_shift, max_shift;
   size_t min_size;

   opium_slab_t *slabs;

   opium_log_t *log;
} opium_arena_t;

/* API */

/* Lifecycle */
int opium_arena_init(opium_arena_t *arena, opium_log_t *log);
void opium_arena_exit(opium_arena_t *arena);

void *opium_arena_alloc(opium_arena_t *arena, size_t size);
void opium_arena_free(opium_arena_t *arena, void *ptr);

#endif /* OPIUM_ARENA_INCLUDE_H */
