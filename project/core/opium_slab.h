#ifndef OPIUM_SLAB_INCLUDE_H
#define OPIUM_SLAB_INCLUDE_H

#include "core/opium_core.h"

/* Define the "mask" for free/used slots depending on pointer size */
#if (OPIUM_PTR_SIZE == 4) 
/* All 32 bits set */
typedef opium_uint_t opium_slab_mask_t;
#define OPIUM_SLAB_PAGE_FREE 0
#define OPIUM_SLAB_PAGE_BUSY ((opium_u32_t)0xFFFFFFFF)

#else
/* All 64 bits set */
typedef opium_u64_t opium_slab_mask_t; 
#define OPIUM_SLAB_PAGE_FREE 0
#define OPIUM_SLAB_PAGE_BUSY ((opium_u64_t)0xFFFFFFFFFFFFFFFF)

#endif

/* 
 * opium_slab_header_t — per-slot header for slab allocator.
 * 
 * Stores metadata about the slot, currently just the slab index.
 * Placed immediately before user data (ptr - OPIUM_SLAB_SLOT_HEADER).
 */
typedef struct opium_slab_header_s opium_slab_header_t;

struct opium_slab_header_s {
   opium_u8_t index;
};

/* opium_slab_stat_t - Just will store informaion
 * It will be convenient for both debbugind and testing.
 */
typedef struct opium_slab_stat_s opium_slab_stat_t;

struct opium_slab_stat_s {
   size_t total;
   size_t used;

   size_t reqs;
   size_t fails;
};

/* opium_slab_page_t - The page where memory and block are stored
 * Contains:
 *  - mask - a bitmask of occupied slots.
 *  - refcount - the counter of occupied slots at the 'boss' level.
 * If refcount != 0, the boss page manages the memory block's lifecycle.
 * If refcount == 0, the slave page is a "child" page, its lifecycle is controlled by the boss.
 *  
 *  - boss - a pointer to the boss page (for slaves).
 *  - data - an array of objects (items) on the page.
 *
 * Meaning: a page is a memory block with objects, 
 * which can be either "independent" (boss) or subordinate (slave). 
 * Using refcount and the list of pages, we can easily understand 
 * which pages are fully occupied, which are partially occupied,
 * and which are empty.
 *
 *  [Boss Page 0 | Page 1 | Page 2 | Page 3 ... Page N]
 *
 */ 
typedef struct opium_slab_page_s opium_slab_page_t;

struct opium_slab_page_s {
   opium_list_head_t head;
   struct opium_slab_page_s *boss;

   opium_slab_mask_t mask;
   size_t refcount;

   opium_s8_t data[] __attribute__((aligned(sizeof(void *))));
};

/* opium_slab_t - is the main allocator structure for objects of the same size.
 *  - page_size - the size of one page (including header and data).
 *  - item_size - the size of each object in bytes.
 *  - item_count - the number of objects that fit on one page.
 * 
 * Page lists:
 *  - empty - completely empty pages.
 *  - partial - pages with at least one occupied object.
 *  - full - pages where all slots are occupied.
 *
 *  - alignment_mask - a mask for quickly finding the start of a page using an arbitrary pointer.
 *  - stats - usage statistics (number of objects in use, requests, failures, etc.)
 *
 */

//typedef struct opium_slab_s opium_slab_t;

struct opium_slab_s {
   size_t page_size, pages_per_alloc;
   size_t item_size, item_count;

   opium_u64_t alignment_mask; 

   opium_list_head_t empty, partial, full; 

   opium_slab_stat_t stats;

   opium_log_t *log;
};

#define OPIUM_SLAB_SLOT_HEADER sizeof(opium_slab_header_t)

typedef void (*opium_slab_trav_ctx)(void *data);

/* API */

/* Lifecycle */
int  opium_slab_init(opium_slab_t *slab, size_t item_size, opium_log_t *log);
void opium_slab_exit(opium_slab_t *slab);

/* Allocation */
void *opium_slab_alloc(opium_slab_t *slab);
void *opium_slab_calloc(opium_slab_t *slab);
void opium_slab_free(opium_slab_t *slab, void *ptr);

/* Additional */
void opium_slab_traverse(opium_slab_t *slab, opium_slab_trav_ctx func);
void opium_slab_stats(opium_slab_t *slab);

/* Statics */
static inline void opium_slab_zero_stats(opium_slab_stat_t *stats) {
   stats->total = stats->used = stats->reqs = stats->fails = 0; 
}

static inline opium_slab_header_t *opium_slab_slot_header(void *ptr) {
    return (opium_slab_header_t *)((opium_u8_t *)ptr - OPIUM_SLAB_SLOT_HEADER);
}

static inline void *opium_slab_slot(opium_slab_t *slab, opium_slab_page_t *page, size_t index)  {
   return (void*)((u_char*) page->data + index * slab->item_size + OPIUM_SLAB_SLOT_HEADER);
}

static inline opium_slab_mask_t opium_slab_page_init(size_t count) {
   return OPIUM_SLAB_PAGE_BUSY ^ ((1ULL << count) - 1);
}

static inline size_t opium_slab_page_one_used(opium_slab_mask_t mask, size_t count) {
   opium_slab_mask_t relevant = mask & ((1ULL << count) - 1);
   return (relevant != 0 && (relevant & (relevant - 1)) == 0) ? OPIUM_RET_OK : OPIUM_RET_ERR; 
}

#endif /* OPIUM_SLAB_INCLUDE_H */

