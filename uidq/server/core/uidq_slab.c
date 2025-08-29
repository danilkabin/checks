#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "uidq_slab.h"
#include "core/uidq_alloc.h"
#include "core/uidq_bitmask.h"
#include "core/uidq_conf.h"
#include "core/uidq_core.h"
#include "core/uidq_list.h"
#include "core/uidq_log.h"
#include "uidq/core/uidq_list.h"

#define UIDQ_SLAB_PAGE_SIZE     4096
#define UIDQ_SLAB_DEFAULT_COUNT 64
#define UIDQ_SLAB_SHIFT         32
#define UIDQ_SLAB_SHIFT_INVALID 0
#define UIDQ_SLAB_SHIFT_MIN     8
#define UIDQ_SLAB_SHIFT_MAX     2048

#define UIDQ_SLAB_SHIFT_NUM \
   ((int)(log2(UIDQ_SLAB_SHIFT_MAX) - log2(UIDQ_SLAB_SHIFT_MIN) + 1))

#define UIDQ_SLAB_PAGE_DEFAULT 0
#define UIDQ_SLAB_PAGE_BIG     1

static uidq_slab_page_t *uidq_page_alloc(uidq_slab_t *slab, uidq_slab_slot_t *slot);
static void uidq_page_dealloc(uidq_slab_t *slab, uidq_slab_page_t *page);

static void *uidq_push_default(uidq_slab_t *slab, uidq_slab_slot_t *slot);
static void *uidq_push_big(uidq_slab_t *slab, uidq_slab_slot_t *slot, size_t block_size);

static void uidq_slab_pop_default(uidq_slab_t *slab, uidq_slab_page_t *page);
static void uidq_slab_pop_big(uidq_slab_t *slab, uidq_slab_page_t *page);

   int
uidq_slab_isvalid(uidq_slab_t *slab) 
{
   return slab && slab->initialized == 1;
}

   uidq_slab_conf_t *
uidq_slab_conf_get(uidq_slab_t *slab) 
{
   if (!uidq_slab_isvalid(slab))
      return NULL;
   return (uidq_slab_conf_t *)&slab->conf;
}

   void
uidq_slab_zero_stats(uidq_slab_stat_t *stats) 
{
   stats->total = 0; 
   stats->used = 0; 
   stats->reqs = 0; 
   stats->fails = 0; 
}

   uidq_slab_t *
uidq_slab_create(uidq_slab_conf_t *conf, uidq_log_t *log) 
{
   if (!conf) {
      uidq_err(log, "Invalid configuration.\n");
      return NULL;
   }

   uidq_slab_t *slab = uidq_calloc(sizeof(uidq_slab_t), log);
   if (!slab) {
      uidq_err(log, "Failed to allocate hash table.\n");
      return NULL;
   }

   slab->initialized = 1;

   if (uidq_slab_init(slab, conf, log) != UIDQ_RET_OK) {
      uidq_err(log, "Failed to initialize hash table.\n");
      uidq_slab_abort(slab);
      return NULL;
   }

   return slab;
}

   void 
uidq_slab_abort(uidq_slab_t *slab) 
{
   if (!slab) return;
   uidq_slab_exit(slab);
   slab->initialized = 0;
   uidq_free(slab, NULL);
}

   int
uidq_slab_init(uidq_slab_t *slab, uidq_slab_conf_t *conf, uidq_log_t *log) 
{
   if (!uidq_slab_isvalid(slab)) {
      return UIDQ_RET_ERR;
   }

   if (!conf) {
      uidq_err(log, "Invalid configuration.\n");
      return UIDQ_RET_ERR;
   }

   uidq_slab_conf_t *config = uidq_slab_conf_get(slab);
   if (!config) {
      uidq_err(log, "Failed to get configuration.\n");
      return UIDQ_RET_ERR;
   }

   // Slab setting
   config->count = conf->count > 0 ? conf->count : UIDQ_SLAB_DEFAULT_COUNT;
   slab->size = UIDQ_SLAB_PAGE_SIZE * config->count;
   slab->free_count = config->count;
   slab->log = log;

   UIDQ_INIT_LIST_HEAD(&slab->free);

   // Init
   size_t slots_size = sizeof(uidq_slab_slot_t) * UIDQ_SLAB_SHIFT_NUM;
   size_t pages_size = sizeof(uidq_slab_page_t) * config->count;
   size_t data_size = slab->size; 

   slab->slots = uidq_calloc(slots_size, slab->log);
   if (!slab->slots) {
      uidq_err(slab->log, "Failed to allocated slab slots.\n");
      goto fail;
   }

   for (int index = 0; index < UIDQ_SLAB_SHIFT_NUM; index++) {
      uidq_slab_slot_t *slot = &slab->slots[index];
      UIDQ_INIT_LIST_HEAD(&slot->pages);

      uidq_slab_stat_t *stats = &slot->stats;
      uidq_slab_zero_stats(stats);

      slot->shift = index;
      slot->block_size = UIDQ_SLAB_SHIFT_MIN << index; 
   }

   slab->pages = uidq_calloc(pages_size, slab->log);
   if (!slab->pages) {
      uidq_err(slab->log, "Failed to allocate slab pages.\n");
      goto fail;
   }

   for (size_t index = 0; index < config->count; index++) {
      uidq_slab_page_t *page = &slab->pages[index];
      UIDQ_INIT_LIST_HEAD(&page->head);
      uidq_list_add(&page->head, &slab->free);

      page->tag = UIDQ_SLAB_PAGE_DEFAULT;

      uidq_bitmask_conf_t bitmask_conf = {.capacity = UIDQ_SLAB_PAGE_SIZE / UIDQ_SLAB_SHIFT_MIN};
      page->bitmask = uidq_bitmask_create(&bitmask_conf, slab->log);
      if (!page->bitmask) {
         uidq_err(slab->log, "Failed to allocate page bitmask.\n");
         goto fail;
      }
   }

   slab->data = uidq_calloc(data_size, slab->log);
   if (!slab->data) {
      uidq_err(slab->log, "Failed to allocate slab data.\n");
      goto fail;
   }

   return UIDQ_RET_OK;

fail:
   uidq_slab_exit(slab);
   return UIDQ_RET_ERR;
}

void uidq_slab_exit(uidq_slab_t *slab) {
   if (!uidq_slab_isvalid(slab)) {
      return;
   }

   if (slab->data) {
      uidq_free(slab->data, NULL);
      slab->data = NULL;
   }

   uidq_slab_conf_t *config = uidq_slab_conf_get(slab);

   if (slab->pages) {
      for (size_t index = 0; index < config->count; index++) {
         uidq_slab_page_t *page = &slab->pages[index];
         uidq_bitmask_exit(page->bitmask);
         if (uidq_list_is_linked(&page->head)) {
            uidq_list_del(&page->head);
         }
      }

      uidq_free(slab->pages, NULL);
      slab->pages = NULL;
   }

   if (slab->slots) {
      for (int index = 0; index < UIDQ_SLAB_SHIFT_NUM; index++) {
         uidq_slab_slot_t *slot = &slab->slots[index];

         uidq_slab_stat_t *stats = &slot->stats;
         uidq_slab_zero_stats(stats);

         uidq_slab_page_t *page, *tmp;
         uidq_list_for_each_entry_safe(page, tmp, &slot->pages, head) {
            uidq_list_del(&page->head);
         }

         slot->shift = 0;
         slot->block_size = 0;
      }

      uidq_free(slab->slots, NULL);
      slab->slots = NULL;
   }

   config->count = 0;
   slab->size = 0;
   slab->free_count = 0;
   slab->log = NULL;

}

void *uidq_slab_push(uidq_slab_t *slab, size_t size) {
   if (!uidq_slab_isvalid(slab)) {
      return NULL;
   }

   if (size == 0) {
      uidq_debug(slab->log, "Zero size requested.\n");
      return NULL;
   }

   size_t shift = 0;
   size_t block_size = UIDQ_SLAB_SHIFT_MIN;

   while (block_size < size) {
      block_size <<= 1;
      shift++;
   }

   uidq_slab_slot_t *slot = &slab->slots[shift];
   uint8_t *data = slab->data;
   uint8_t *ptr = NULL;

   if (block_size <= UIDQ_SLAB_SHIFT_MAX) {
      if (shift >= UIDQ_SLAB_SHIFT_NUM) {
         uidq_err(slab->log, "Calculated shift exceeds slots array.\n");
         return NULL;
      }
      ptr = uidq_push_default(slab, slot);
   } else {
      ptr = uidq_push_big(slab, slot, block_size);
   }

   if (!ptr || ptr < data || ptr >= data + slab->size) {
      uidq_err(slab->log, "Returned pointer is out of slab data range.\n");
      return NULL;
   }

   return ptr;
}

   static void *
uidq_push_default(uidq_slab_t *slab, uidq_slab_slot_t *slot)
{
   if (!uidq_slab_isvalid(slab)) {
      return NULL;
   }

   if (!slot) {
      uidq_err(slab->log, "Invalid slot parameter.\n");
      return NULL;
   }

   uidq_slab_stat_t *stats = &slot->stats; 
   uidq_slab_page_t *page = NULL;

   if (uidq_list_empty(&slot->pages)) {
      page = uidq_page_alloc(slab, slot);
   } else {
      page = uidq_list_first_entry(&slot->pages, uidq_slab_page_t, head);
   }

   stats->total = stats->total + 1;

   if (!page) {
      uidq_err(slab->log, "Failed to allocate page!\n");
      stats->fails++;
      return NULL;
   }

   int index = uidq_bitmask_push(page->bitmask, -1, 1);
   if (index < 0) {
      stats->fails = stats->fails + 1;
      if (uidq_bitmask_is_empty(page->bitmask)) {
         uidq_page_dealloc(slab, page);
      }
      return NULL;
   }

   if (uidq_bitmask_is_full(page->bitmask) == UIDQ_RET_OK) {
      uidq_list_del(&page->head);
   }

   page->tag = UIDQ_SLAB_PAGE_DEFAULT;

   size_t page_index = page - slab->pages;
   size_t offset = page_index * UIDQ_SLAB_PAGE_SIZE + index * slot->block_size;
   return slab->data + offset;
}

   static void *
uidq_push_big(uidq_slab_t *slab, uidq_slab_slot_t *slot, size_t block_size) 
{
   if (!uidq_slab_isvalid(slab)) {
      return NULL;
   }

   if (!slot || block_size == 0) {
      uidq_err(slab->log, "Invalid parameters.\n");
      return NULL;
   }

   uidq_slab_stat_t *stats = &slot->stats;
   uidq_slab_page_t *current, *tmp = NULL;
   uidq_slab_page_t *baby = NULL;

   size_t demand = block_size / UIDQ_SLAB_PAGE_SIZE;
   size_t found = 0;

   if (slab->free_count < demand) {
      uidq_debug(slab->log, "Too few free pages for big allocation.\n");
      return NULL;
   }

   uidq_list_for_each_entry_safe(current, tmp, &slab->free, head) {
      current->tag = UIDQ_SLAB_PAGE_BIG;
      uidq_list_del(&current->head);

      if (!baby) {
         baby = current;
         UIDQ_INIT_LIST_HEAD(&baby->head);
      } else {
         uidq_list_add(&current->head, &baby->head);
      }

      found++;
      if (found == demand) {
         break;
      }
   }

   if (!baby) {
      return NULL;
   }

   uidq_bitmask_reset(baby->bitmask);
   uidq_bitmask_lim_capacity(baby->bitmask, found);
   baby->tag = UIDQ_SLAB_PAGE_BIG;

   slab->free_count -= found;

   size_t page_index = baby - slab->pages;
   size_t offset = page_index * UIDQ_SLAB_PAGE_SIZE;
   return slab->data + offset;
}

void uidq_slab_pop(uidq_slab_t *slab, void *ptr) {
   if (!uidq_slab_isvalid(slab)) {
      return;
   }

   if (!ptr) {
      uidq_err(slab->log, "Invalid pointer parameter.\n");
      return;
   }

   size_t offset = (uint8_t*)ptr - (uint8_t*)slab->data;
   size_t page_index = offset / UIDQ_SLAB_PAGE_SIZE;
   size_t page_offset = offset % UIDQ_SLAB_PAGE_SIZE;

   uidq_slab_page_t *page = &slab->pages[page_index];

   if (page->tag == UIDQ_SLAB_PAGE_BIG) {
      uidq_slab_pop_big(slab, page);
   } else {
      uidq_slab_pop_default(slab, page);
   }

   if (uidq_bitmask_is_empty(page->bitmask) == UIDQ_RET_OK) {
      uidq_page_dealloc(slab, page);
   }
}

   static void
uidq_slab_pop_default(uidq_slab_t *slab, uidq_slab_page_t *page)
{
   if (!uidq_slab_isvalid(slab)) {
      return;
   }

   if (!page) {
      uidq_err(slab->log, "Invalid page parameter.\n");
      return;
   }

   size_t size = UIDQ_SLAB_PAGE_SIZE / uidq_bitmask_get_capacity(page->bitmask);
   if (size == 0) {
      return;
   }

   size_t shift = 0;
   size_t block_size = UIDQ_SLAB_SHIFT_MIN;
   while (block_size < size) {
      block_size <<= 1;
      shift++;
   }

   uidq_slab_slot_t *slot = &slab->slots[shift];

   if (uidq_bitmask_is_full(page->bitmask) == UIDQ_RET_OK) {
      uidq_list_add_tail(&page->head, &slot->pages); 
   }

   uidq_bitmask_pop(page->bitmask, -1, 1);
}

   static void
uidq_slab_pop_big(uidq_slab_t *slab, uidq_slab_page_t *page)
{
   if (!uidq_slab_isvalid(slab)) {
      return;
   }

   if (!page) {
      uidq_err(slab->log, "Invalid page parameter.\n");
      return;
   }

   size_t page_index = page - slab->pages;

   uidq_slab_page_t *current, *tmp = NULL;
   uidq_list_for_each_entry_safe(current, tmp, &page->head, head) {
      uidq_list_del(&current->head);
      uidq_bitmask_reset(current->bitmask);
      slab->free_count++;
   }
}

   static uidq_slab_page_t *
uidq_page_alloc(uidq_slab_t *slab, uidq_slab_slot_t *slot)
{
   if (!uidq_slab_isvalid(slab)) {
      return NULL;
   }

   if (!slot) {
      uidq_err(slab->log, "Invalid slot parameter.\n");
      return NULL;
   }

   if (uidq_list_empty(&slab->free)) {
      uidq_err(slab->log, "Free list is empty.\n");
      return NULL;
   }

   uidq_slab_page_t *page = uidq_list_first_entry(&slab->free, uidq_slab_page_t, head);
   if (!page) {
      uidq_err(slab->log, "Failed to allocate page. Page in free list empty.\n");
      return NULL;
   }

   size_t new_capacity = UIDQ_SLAB_PAGE_SIZE / slot->block_size;
   uidq_bitmask_reset(page->bitmask);
   uidq_bitmask_lim_capacity(page->bitmask, new_capacity);

   uidq_list_del(&page->head);
   uidq_list_add_tail(&page->head, &slot->pages);

   slab->free_count--;

   return page;
}

   void
uidq_page_dealloc(uidq_slab_t *slab, uidq_slab_page_t *page) 
{
   if (!uidq_slab_isvalid(slab)) {
      return;
   }

   if (!page) {
      uidq_err(slab->log, "Invalid page pointer.\n");
      return;
   }

   if (uidq_list_is_linked(&page->head)) {
      uidq_debug(slab->log, "Deallocated page %p\n", page);
      uidq_list_del(&page->head);
   }

   uidq_list_add_tail(&page->head, &slab->free);
   uidq_bitmask_reset(page->bitmask);

   slab->free_count++;
}

   void
uidq_slab_chains_debug(uidq_slab_t *slab) 
{
   if (!uidq_slab_isvalid(slab)) {
      return;
   }

   uidq_slab_page_t *page, *tmp = NULL;

   for (int index = 0; index < UIDQ_SLAB_SHIFT_NUM; index++) {
      uidq_slab_slot_t *slot = &slab->slots[index];

      uidq_debug_inline(slab->log, "head. Index: %d ", index);

      uidq_list_for_each_entry_safe(page, tmp, &slot->pages, head) {
         uidq_debug_inline(slab->log, "%p ", page);
      }

      uidq_debug_inline(slab->log, "\n");
   }
}
