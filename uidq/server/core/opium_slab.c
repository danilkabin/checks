#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "opium_slab.h"
#include "core/opium_alloc.h"
#include "core/opium_bitmask.h"
#include "core/opium_conf.h"
#include "core/opium_core.h"
#include "core/opium_list.h"
#include "core/opium_log.h"
#include "core/opium_list.h"

#define OPIUM_SLAB_PAGE_SIZE     4096
#define OPIUM_SLAB_DEFAULT_COUNT 64
#define OPIUM_SLAB_SHIFT         32
#define OPIUM_SLAB_SHIFT_INVALID 0
#define OPIUM_SLAB_SHIFT_MIN     8
#define OPIUM_SLAB_SHIFT_MAX     2048

#define OPIUM_SLAB_SHIFT_NUM \
   ((int)(log2(OPIUM_SLAB_SHIFT_MAX) - log2(OPIUM_SLAB_SHIFT_MIN) + 1))

#define OPIUM_SLAB_PAGE_DEFAULT 0
#define OPIUM_SLAB_PAGE_BIG     1

static opium_slab_page_t *opium_page_alloc(opium_slab_t *slab, opium_slab_slot_t *slot);
static void opium_page_dealloc(opium_slab_t *slab, opium_slab_page_t *page);

static void *opium_push_default(opium_slab_t *slab, opium_slab_slot_t *slot);
static void *opium_push_big(opium_slab_t *slab, opium_slab_slot_t *slot, size_t block_size);

static void opium_slab_pop_default(opium_slab_t *slab, opium_slab_page_t *page);
static void opium_slab_pop_big(opium_slab_t *slab, opium_slab_page_t *page);

   int
opium_slab_isvalid(opium_slab_t *slab)
{
   return slab && slab->initialized == 1;
}

   opium_slab_conf_t *
opium_slab_conf_get(opium_slab_t *slab)
{
   if (!opium_slab_isvalid(slab))
      return NULL;
   return (opium_slab_conf_t *)&slab->conf;
}

   void
opium_slab_zero_stats(opium_slab_stat_t *stats)
{
   stats->total = 0; 
   stats->used = 0; 
   stats->reqs = 0; 
   stats->fails = 0; 
}

   opium_slab_t *
opium_slab_create(opium_slab_conf_t *conf, opium_log_t *log)
{
   if (!conf) {
      opium_err(log, "Invalid configuration.\n");
      return NULL;
   }

   opium_slab_t *slab = opium_calloc(sizeof(opium_slab_t), log);
   if (!slab) {
      opium_err(log, "Failed to allocate hash table.\n");
      return NULL;
   }

   slab->initialized = 1;

   if (opium_slab_init(slab, conf, log) != OPIUM_RET_OK) {
      opium_err(log, "Failed to initialize hash table.\n");
      opium_slab_abort(slab);
      return NULL;
   }

   return slab;
}

   void 
opium_slab_abort(opium_slab_t *slab)
{
   if (!slab) return;
   opium_slab_exit(slab);
   slab->initialized = 0;
   opium_free(slab, NULL);
}

   int
opium_slab_init(opium_slab_t *slab, opium_slab_conf_t *conf, opium_log_t *log)
{
   if (!opium_slab_isvalid(slab)) {
      return OPIUM_RET_ERR;
   }

   if (!conf) {
      opium_err(log, "Invalid configuration.\n");
      return OPIUM_RET_ERR;
   }

   opium_slab_conf_t *config = opium_slab_conf_get(slab);
   if (!config) {
      opium_err(log, "Failed to get configuration.\n");
      return OPIUM_RET_ERR;
   }

   // Slab setting
   config->count = conf->count > 0 ? conf->count : OPIUM_SLAB_DEFAULT_COUNT;
   slab->size = OPIUM_SLAB_PAGE_SIZE * config->count;
   slab->free_count = config->count;
   slab->log = log;

   OPIUM_INIT_LIST_HEAD(&slab->free);

   // Init
   size_t slots_size = sizeof(opium_slab_slot_t) * OPIUM_SLAB_SHIFT_NUM;
   size_t pages_size = sizeof(opium_slab_page_t) * config->count;
   size_t data_size = slab->size; 

   slab->slots = opium_calloc(slots_size, slab->log);
   if (!slab->slots) {
      opium_err(slab->log, "Failed to allocated slab slots.\n");
      goto fail;
   }

   for (int index = 0; index < OPIUM_SLAB_SHIFT_NUM; index++) {
      opium_slab_slot_t *slot = &slab->slots[index];
      OPIUM_INIT_LIST_HEAD(&slot->pages);

      opium_slab_stat_t *stats = &slot->stats;
      opium_slab_zero_stats(stats);

      slot->shift = index;
      slot->block_size = OPIUM_SLAB_SHIFT_MIN << index;
   }

   slab->pages = opium_calloc(pages_size, slab->log);
   if (!slab->pages) {
      opium_err(slab->log, "Failed to allocate slab pages.\n");
      goto fail;
   }

   for (size_t index = 0; index < config->count; index++) {
      opium_slab_page_t *page = &slab->pages[index];
      OPIUM_INIT_LIST_HEAD(&page->head);
      opium_list_add(&page->head, &slab->free);

      page->tag = OPIUM_SLAB_PAGE_DEFAULT;

      size_t capacity = OPIUM_SLAB_PAGE_SIZE / OPIUM_SLAB_SHIFT_MIN;
      page->bitmask = opium_bitmask_create(capacity, slab->log);
      if (!page->bitmask) {
         opium_err(slab->log, "Failed to allocate page bitmask.\n");
         goto fail;
      }
   }

   slab->data = opium_calloc(data_size, slab->log);
   if (!slab->data) {
      opium_err(slab->log, "Failed to allocate slab data.\n");
      goto fail;
   }

   return OPIUM_RET_OK;

fail:
   opium_slab_exit(slab);
   return OPIUM_RET_ERR;
}

void opium_slab_exit(opium_slab_t *slab) {
   if (!opium_slab_isvalid(slab)) {
      return;
   }

   if (slab->data) {
      opium_free(slab->data, NULL);
      slab->data = NULL;
   }

   opium_slab_conf_t *config = opium_slab_conf_get(slab);

   if (slab->pages) {
      for (size_t index = 0; index < config->count; index++) {
         opium_slab_page_t *page = &slab->pages[index];
         opium_bitmask_exit(page->bitmask);
         if (opium_list_is_linked(&page->head)) {
            opium_list_del(&page->head);
         }
      }

      opium_free(slab->pages, NULL);
      slab->pages = NULL;
   }

   if (slab->slots) {
      for (int index = 0; index < OPIUM_SLAB_SHIFT_NUM; index++) {
         opium_slab_slot_t *slot = &slab->slots[index];

         opium_slab_stat_t *stats = &slot->stats;
         opium_slab_zero_stats(stats);

         opium_slab_page_t *page, *tmp;
         opium_list_for_each_entry_safe(page, tmp, &slot->pages, head) {
            opium_list_del(&page->head);
         }

         slot->shift = 0;
         slot->block_size = 0;
      }

      opium_free(slab->slots, NULL);
      slab->slots = NULL;
   }

   config->count = 0;
   slab->size = 0;
   slab->free_count = 0;
   slab->log = NULL;

}

void *opium_slab_push(opium_slab_t *slab, size_t size) {
   if (!opium_slab_isvalid(slab)) {
      return NULL;
   }

   if (size == 0) {
      opium_debug(slab->log, "Zero size requested.\n");
      return NULL;
   }

   size_t shift = 0;
   size_t block_size = OPIUM_SLAB_SHIFT_MIN;

   while (block_size < size) {
      block_size <<= 1;
      shift++;
   }

   opium_slab_slot_t *slot = &slab->slots[shift];
   uint8_t *data = slab->data;
   uint8_t *ptr = NULL;

   if (block_size <= OPIUM_SLAB_SHIFT_MAX) {
      if (shift >= OPIUM_SLAB_SHIFT_NUM) {
         opium_err(slab->log, "Calculated shift exceeds slots array.\n");
         return NULL;
      }
      ptr = opium_push_default(slab, slot);
   } else {
      ptr = opium_push_big(slab, slot, block_size);
   }

   if (!ptr || ptr < data || ptr >= data + slab->size) {
      opium_err(slab->log, "Returned pointer is out of slab data range.\n");
      return NULL;
   }

   return ptr;
}

   static void *
opium_push_default(opium_slab_t *slab, opium_slab_slot_t *slot)
{
   if (!opium_slab_isvalid(slab)) {
      return NULL;
   }

   if (!slot) {
      opium_err(slab->log, "Invalid slot parameter.\n");
      return NULL;
   }

   opium_slab_stat_t *stats = &slot->stats;
   opium_slab_page_t *page = NULL;

   if (opium_list_empty(&slot->pages)) {
      page = opium_page_alloc(slab, slot);
   } else {
      page = opium_list_first_entry(&slot->pages, opium_slab_page_t, head);
   }

   stats->total = stats->total + 1;

   if (!page) {
      opium_err(slab->log, "Failed to allocate page!\n");
      stats->fails++;
      return NULL;
   }

   int index = opium_bitmask_push(page->bitmask, -1, 1);
   if (index < 0) {
      stats->fails = stats->fails + 1;
      if (opium_bitmask_is_empty(page->bitmask)) {
         opium_page_dealloc(slab, page);
      }
      return NULL;
   }

   if (opium_bitmask_is_full(page->bitmask) == OPIUM_RET_OK) {
      opium_list_del(&page->head);
   }

   page->tag = OPIUM_SLAB_PAGE_DEFAULT;

   size_t page_index = page - slab->pages;
   size_t offset = page_index * OPIUM_SLAB_PAGE_SIZE + index * slot->block_size;
   return slab->data + offset;
}

   static void *
opium_push_big(opium_slab_t *slab, opium_slab_slot_t *slot, size_t block_size)
{
   if (!opium_slab_isvalid(slab)) {
      return NULL;
   }

   if (!slot || block_size == 0) {
      opium_err(slab->log, "Invalid parameters.\n");
      return NULL;
   }

   opium_slab_stat_t *stats = &slot->stats;
   opium_slab_page_t *current, *tmp = NULL;
   opium_slab_page_t *baby = NULL;

   size_t demand = block_size / OPIUM_SLAB_PAGE_SIZE;
   size_t found = 0;

   if (slab->free_count < demand) {
      opium_debug(slab->log, "Too few free pages for big allocation.\n");
      return NULL;
   }

   opium_list_for_each_entry_safe(current, tmp, &slab->free, head) {
      current->tag = OPIUM_SLAB_PAGE_BIG;
      opium_list_del(&current->head);

      if (!baby) {
         baby = current;
         OPIUM_INIT_LIST_HEAD(&baby->head);
      } else {
         opium_list_add(&current->head, &baby->head);
      }

      found++;
      if (found == demand) {
         break;
      }
   }

   if (!baby) {
      return NULL;
   }

   opium_bitmask_reset(baby->bitmask);
   opium_bitmask_lim_capacity(baby->bitmask, found);
   baby->tag = OPIUM_SLAB_PAGE_BIG;

   slab->free_count -= found;

   size_t page_index = baby - slab->pages;
   size_t offset = page_index * OPIUM_SLAB_PAGE_SIZE;
   return slab->data + offset;
}

void opium_slab_pop(opium_slab_t *slab, void *ptr) {
   if (!opium_slab_isvalid(slab)) {
      return;
   }

   if (!ptr) {
      opium_err(slab->log, "Invalid pointer parameter.\n");
      return;
   }

   size_t offset = (uint8_t*)ptr - (uint8_t*)slab->data;
   size_t page_index = offset / OPIUM_SLAB_PAGE_SIZE;
   size_t page_offset = offset % OPIUM_SLAB_PAGE_SIZE;

   opium_slab_page_t *page = &slab->pages[page_index];

   if (page->tag == OPIUM_SLAB_PAGE_BIG) {
      opium_slab_pop_big(slab, page);
   } else {
      opium_slab_pop_default(slab, page);
   }

   if (opium_bitmask_is_empty(page->bitmask) == OPIUM_RET_OK) {
      opium_page_dealloc(slab, page);
   }
}

   static void
opium_slab_pop_default(opium_slab_t *slab, opium_slab_page_t *page)
{
   if (!opium_slab_isvalid(slab)) {
      return;
   }

   if (!page) {
      opium_err(slab->log, "Invalid page parameter.\n");
      return;
   }

   size_t size = OPIUM_SLAB_PAGE_SIZE / opium_bitmask_get_capacity(page->bitmask);
   if (size == 0) {
      return;
   }

   size_t shift = 0;
   size_t block_size = OPIUM_SLAB_SHIFT_MIN;
   while (block_size < size) {
      block_size <<= 1;
      shift++;
   }

   opium_slab_slot_t *slot = &slab->slots[shift];

   if (opium_bitmask_is_full(page->bitmask) == OPIUM_RET_OK) {
      opium_list_add_tail(&page->head, &slot->pages);
   }

   opium_bitmask_pop(page->bitmask, -1, 1);
}

   static void
opium_slab_pop_big(opium_slab_t *slab, opium_slab_page_t *page)
{
   if (!opium_slab_isvalid(slab)) {
      return;
   }

   if (!page) {
      opium_err(slab->log, "Invalid page parameter.\n");
      return;
   }

   size_t page_index = page - slab->pages;

   opium_slab_page_t *current, *tmp = NULL;
   opium_list_for_each_entry_safe(current, tmp, &page->head, head) {
      opium_list_del(&current->head);
      opium_bitmask_reset(current->bitmask);
      slab->free_count++;
   }
}

   static opium_slab_page_t *
opium_page_alloc(opium_slab_t *slab, opium_slab_slot_t *slot)
{
   if (!opium_slab_isvalid(slab)) {
      return NULL;
   }

   if (!slot) {
      opium_err(slab->log, "Invalid slot parameter.\n");
      return NULL;
   }

   if (opium_list_empty(&slab->free)) {
      opium_err(slab->log, "Free list is empty.\n");
      return NULL;
   }

   opium_slab_page_t *page = opium_list_first_entry(&slab->free, opium_slab_page_t, head);
   if (!page) {
      opium_err(slab->log, "Failed to allocate page. Page in free list empty.\n");
      return NULL;
   }

   size_t new_capacity = OPIUM_SLAB_PAGE_SIZE / slot->block_size;
   opium_bitmask_reset(page->bitmask);
   opium_bitmask_lim_capacity(page->bitmask, new_capacity);

   opium_list_del(&page->head);
   opium_list_add_tail(&page->head, &slot->pages);

   slab->free_count--;

   return page;
}

   void
opium_page_dealloc(opium_slab_t *slab, opium_slab_page_t *page)
{
   if (!opium_slab_isvalid(slab)) {
      return;
   }

   if (!page) {
      opium_err(slab->log, "Invalid page pointer.\n");
      return;
   }

   if (opium_list_is_linked(&page->head)) {
      opium_debug(slab->log, "Deallocated page %p\n", page);
      opium_list_del(&page->head);
   }

   opium_list_add_tail(&page->head, &slab->free);
   opium_bitmask_reset(page->bitmask);

   slab->free_count++;
}

   void
opium_slab_chains_debug(opium_slab_t *slab)
{
   if (!opium_slab_isvalid(slab)) {
      return;
   }

   opium_slab_page_t *page, *tmp = NULL;

   for (int index = 0; index < OPIUM_SLAB_SHIFT_NUM; index++) {
      opium_slab_slot_t *slot = &slab->slots[index];

      opium_debug_inline(slab->log, "head. Index: %d ", index);

      opium_list_for_each_entry_safe(page, tmp, &slot->pages, head) {
         opium_debug_inline(slab->log, "%p ", page);
      }

      opium_debug_inline(slab->log, "\n");
   }
}
