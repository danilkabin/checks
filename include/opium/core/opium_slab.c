/* opium_slab.c 
 * 
 * Slab Allocator Overview:
 *
 * This module implements a slab-style memory allocator designed for
 * fast allocation of fixed-size objects. It is primary goals are to
 * minimize fragmentation, ensure alignment and maintain fast allocation
 * and deallocation oprations.
 *
 * Memory alignment is important to:
 *  - Speed up memory access (a hardware requirement of processors)
 *  - Simplify the allocator, which allocates memory blocks (slabs) with addresses
 *  are multiples of the page size.
 *  - Minimize memory fragmentation.
 *
 */

#include "core/opium_core.h"

/* Number of bits in a pointer (32-bit or 64-bit) */
#define OPIUM_SLAB_BITS (OPIUM_PTR_SIZE * 8)

/* Page size in bytes (standard slab page) */
#define OPIUM_SLAB_PAGE_SIZE 4096  

/* Maximum and minimum number of items per page */
#define OPIUM_SLAB_PAGE_MAX OPIUM_SLAB_BITS   /* Max items in a slab page */
#define OPIUM_SLAB_PAGE_MIN 8   /* Min items for allocation */

/* Find the first free slot in a mask (count trailing zeros in inverted mask) */
#define OPIUM_SLOTS_FIND_FREE(s) ((size_t) __builtin_ctzll(s))

/* Iterates over memory blocks between pos and end, each of size page_size 
 * and casts the current element to type opium_slab_page_t*
 */
#define opium_slab_slots_for_each(current, pos, end, page_size) \
   for ((current) = (opium_slab_page_t*) (pos);            \
         (u_char*) (current) < (u_char*) (end);            \
         (current) = (opium_slab_page_t*) ((current) + (page_size)))

   static void *
opium_slab_new_slot(opium_slab_t *slab, opium_slab_page_t *page)
{
   assert(slab != NULL);
   assert(page != NULL);

   /* Locate a free slot (0 in bitmask) and mark it as occupied */
   size_t slot = OPIUM_SLOTS_FIND_FREE(~page->mask);
   page->mask = page->mask | (1ULL << slot);

   /* Allocating a slot in the page:
    * 
    * 'page->mask' is a bitmask where a each bit represents a slot:
    *   0 -> free slot
    *   1 -> busy slot
    *
    *  Leading bits beyond 'item_count' are always set to 1 (unavailable)
    *
    *  - Example (item_count = 5, 8-bit mask for clarity):
    *   After 1st alloc:   11100001  (slot 0 used)
    *   After 2nd alloc:   11100011  (slot 1 used)
    *   After 3rd alloc:   11100111  (slot 2 used)
    *   After 4th alloc:   11101111  (slot 3 used)
    *   After 5th alloc:   11111111  (all slots used, fully occupied)
    *
    *  Once the mask becomes all 1s ('OPIUM_SLAB_PAGE_BUSY'), the page
    *  is moved from the 'partial' list to 'full' list.
    *
    */

   if (opium_unlikely(page->mask == OPIUM_SLAB_PAGE_BUSY)) {
      opium_list_del(&page->head);
      opium_list_add(&page->head, &slab->full);
   }

   slab->stats.used = slab->stats.used + 1;

   /* Return pointer to allocated object within page */
   //*((opium_ubyte_t*)(data)) = slot;
   void *ptr = opium_slab_slot(slab, page, slot);
   opium_slab_header_t *header = opium_slab_slot_header(ptr);
   header->index = slot;
   return ptr;
}

   int 
opium_slab_init(opium_slab_t *slab, size_t item_size, opium_log_t *log)
{
   assert(slab != NULL);
   assert(item_size >= 1 && item_size <= SIZE_MAX);

   size_t item_count = OPIUM_SLAB_PAGE_MAX;

   slab->item_size = item_size + OPIUM_SLAB_SLOT_HEADER;

   /* 
    * 1. offsetof(opium_slab_page_t, data) - This is the offset of the data field
    * within in structure, i.e how many bytes the header occupies before the data array.
    * 2. data_offset + item_count * slab->item_size - This is the full size of the slab
    * in bytes, including the header and all objects.
    * 
    * 3. 1ULL << (size_t)(ceil(log2(data_size))) - We take the logarithm to the base 2 of this size.
    * What power of two is greater than or equal to this size?
    * ceil(...) we round up to the nearest whole number because the logarithm may be fractional
    * 4. item_count - guess 
    */

   size_t data_offset = offsetof(opium_slab_page_t, data);
   size_t page_size = data_offset + item_count * slab->item_size; 

   slab->page_size = 1ULL << (size_t)(ceil(log2(page_size)));
   slab->item_count = item_count;

   /*
    * Minimizing internal fragmentation:
    *
    * When rounding page size to a power of two, we might allocate more memory than needed.
    *
    * Example:
    *   Required data size: 530 bytes
    *   Rounded up: ceil(log2(530)) = 10 -> 1 << 10 = 1024 bytes
    *   Waste: 1024 - 530 = 494 bytes -> internal fragmentation
    *
    * Optimization:
    *   Sometimes it’s better to choose a smaller power of two if it still fits all items
    *   and the header.
    *
    *   Example continued:
    *     512 bytes (2^9) is closer to 530 than 1024
    *     Header size: 40 bytes
    *     Minimum items: (OPIUM_SLAB_PAGE_MIN)
    *     Available for items: 512 - 40 = 472 bytes
    *     Each item: 10 bytes -> fits 47 items
    *
    *   Result:
    *     - Page size smaller
    *     - Less wasted memory
    *     - Objects packed tightly
    */

   if (slab->page_size - page_size != 0) {
      size_t shrinked_page_size = slab->page_size >> 1;
      size_t shrinked_data_size = shrinked_page_size - data_offset;

      if (data_offset < shrinked_page_size && shrinked_data_size > slab->item_size * OPIUM_SLAB_PAGE_MIN) {
         slab->page_size = shrinked_page_size;
         slab->item_count = shrinked_data_size / slab->item_size; 
      }
   }

   OPIUM_INIT_LIST_HEAD(&slab->empty);
   OPIUM_INIT_LIST_HEAD(&slab->partial);
   OPIUM_INIT_LIST_HEAD(&slab->full);

   opium_slab_zero_stats(&slab->stats);

   slab->pages_per_alloc = slab->page_size > OPIUM_SLAB_PAGE_SIZE ? slab->page_size : OPIUM_SLAB_PAGE_SIZE;

   /* 
    * Why the mask is ~(page_size - 1)
    * ---------------------------------
    * page_size must be a power of two (e.g., 32, 64, 128, 512 ...)
    *
    * Let's take an example with:
    *   page_size = 32
    *
    * Then log2(32) = 5, which means we need to zero out the 5 least significant bits
    * of any address to align it to the start of a 32-byte page.
    *
    * Step 1: Compute the mask
    *   page_size       = 0b00100000 = 32
    *   page_size - 1   = 0b00011111 = 31
    *   alignment_mask  = ~(page_size - 1) = 0b11100000
    *
    *  The lower 5 bits become 0 -> will be zeroed when addr & mask
    *  The upper 3 bits remain 1 -> preserve the upper part of the address
    *
    * Step 2: Apply the mask to an address
    *   addr = 0b10110110 = 182
    *   mask = 0b11100000 = 224
    *
    *   Applying addr & alignment_mask:
    *
    *     0b10110110  (addr = 182)
    *   & 0b11100000  (mask = 224)
    *   -------------
    *     0b10100000  (result = 160)
    *
    * Result:
    *   Address 182 is rounded down to 160, which is the **start of the 32-byte block**.
    *
    */

   slab->alignment_mask = ~(slab->page_size - 1);

   slab->log = log;

   opium_log_debug(log, 
         "Slab initialization: item_size: %zu, item_count: %zu data_offset: %zu, page_size: %zu\n", 
         slab->item_size, slab->item_count, data_offset, slab->page_size);

   return OPIUM_RET_OK;

}

   void
opium_slab_exit(opium_slab_t *slab) 
{
   assert(slab != NULL);

   /* We want to go through all the pages */
   opium_list_head_t *heads[] = { &slab->empty, &slab->partial, &slab->full };

   for (size_t index = 0; index < 3; index++) {
      opium_list_head_t *current_head = heads[index];

      opium_slab_page_t *current, *tmp = NULL;

      opium_list_for_each_entry_safe(current, tmp, current_head, head) {
         /*
          * If refcount != 0, them this page is the boss, 
          * its own memory block, including all slave pages.
          */

         if (current->refcount != 0) {
            opium_list_del(&current->head);

            if (slab->page_size <= OPIUM_SLAB_PAGE_SIZE) {
               opium_munmap(current, slab->pages_per_alloc, slab->log);
            } else {
               opium_free(current, slab->log);
            }

            current = NULL;
         } else {
            if (opium_list_is_linked(&current->head)) {
               opium_list_del(&current->head);
            }

         }
      }

   }

   slab->page_size = slab->pages_per_alloc = 0;
   slab->item_size = slab->item_count = 0;

   slab->alignment_mask = 0;

   opium_slab_zero_stats(&slab->stats);

   slab->log = NULL;

}

   void *
opium_slab_alloc(opium_slab_t *slab)
{
   assert(slab != NULL);

   opium_slab_page_t *page;

   slab->stats.reqs = slab->stats.reqs + 1;

   if (opium_likely(!opium_list_empty(&slab->partial))) {
      /*
       * We already have a page with free slots (a partially full page).
       * Simply take the first page from the partial list and allocate
       * a new slot in it using opium_slab_new_slot().
       */

      page = opium_list_first_entry(&slab->partial, opium_slab_page_t, head);

      return opium_slab_new_slot(slab, page);

   } else if (opium_likely(!opium_list_empty(&slab->empty))) {
      /*
       * There are no partially filled pages, but we have completely empty pages.
       * Take the first empty page from the 'empty' list, remove it from there
       * and add it to the 'partial' list, since it will now contain at least one item.
       * Initialize its bitmask and mark it as used.
       * Then allocate a slot on this page using opium_slab_new_slot().
       */

      page = opium_list_first_entry(&slab->empty, opium_slab_page_t, head);

      opium_list_del(&page->head);
      opium_list_add(&page->head, &slab->partial);

      page->mask = opium_slab_page_init(slab->item_count);

      /*
       * Slave pages always have a refcount of 0 - they don`t keep a counter.
       * Only the boss page manages the lifecycle of the entire chunk.
       * Boss.refcount = how many pages (itself + all its slaves) are occupied
       * Slave.refcount = always 0, used only to indicate that it`s a slave,
       * not an independent boss.
       */

      if (opium_unlikely(page->refcount) != 0) {
         /* Boss: refcount >= 1 -> increment at the page itself. */
         page->refcount = page->refcount + 1;
      } else { 
         /* Slave: refcount == 0 and is the boss -> increment at the boss. */
         page->boss->refcount = page->boss->refcount + 1;
      }

      return opium_slab_new_slot(slab, page);

   } else {
      /* Why not malloc? 
       * Malloc has several drawbacks for the slab allocator:
       *  - It does not guarantee alignment to an arbitrary power of two for large objects.
       *  - It doesn`t always allocate entire pages, we need to manage pages precisely.
       *  - malloc adds overhead memory and metadata, which increases internal fragmentation.
       *
       *  It is important for this slab that each page start with an address that
       *  is a multiple of its size, which can be 512, 1024, 2048 ...
       *  
       *  Without alignment, it is impossible to:
       *   - Quickly find the page base by pointer(base = ptr & alignment_mask)
       *
       *  It is better to use mmap for small slabs and posix_memalign for large ones,
       *  because they give full control over the memory.
       *
       *  mmap - for small slabs
       *   - Allocates pages directly from the OS
       *   - Guarantees that blocks are aligned on a page boundary
       *   - You can control directly how many pages are allocated
       *  
       *  posix_memalign - for large slabs 
       *   - Allocates memory with the specified alignment
       *   - You can specify any power of two for alignment (16K, 32K, 64K ...)
       *   - Allows you to obtain a contiguous block of the desired size.
       *
       */

      if (slab->page_size <= OPIUM_SLAB_PAGE_SIZE) {
         page = opium_mmap(slab->pages_per_alloc, slab->log);
         if (opium_unlikely(!page)) {
            slab->stats.fails = slab->stats.fails + 1;
            opium_log_err(slab->log, "Failed to allocate opium_mmap!\n");
            return NULL;
         }

      } else {
         page = opium_memalign(slab->page_size, slab->pages_per_alloc, slab->log);
         if (opium_unlikely(!page)) {
            slab->stats.fails = slab->stats.fails + 1;
            opium_log_err(slab->log, "Failed to allocate opium_memalign!\n");
            return NULL;
         }

      }

      /* The general scheme:
       *
       *  One large chunk of memory is allocated (mmap or posix_memalign) of size
       *  pages_per_alloc. Ths is exactly one block, within which several
       *  opium_slab_page_t will live. This block consists of N pages
       *  (usually N = pages_per_alloc / page_size, for example 
       *  OPIUM_SLAB_PAGE_SIZE = 4096 and page_size = 512. 4096/512 = 8 pages)
       * 
       *  The first page is the boss. The remaining N-1 pages are slaves.
       *   1) For the boss:
       *   - Boss->refcount = 1 (it is immediately considered 'partial' 
       *     because we will begin to prefer the slot in it)
       *   - Boss->mask = opium_slab_page_init(item_count) - bitmask of slots
       *   (one slot is occupied)
       *   The boss is added to the slab->partial list
       *
       *   2) For slaves:
       *   - tmp->boss = boss (point to the boss block)
       *   - tmp->refcount = 0 (because empty)
       *   The are in the slab->empty list
       *
       *  [Boss Page 0 | Page 1 | Page 2 | Page 3 | Page 4 | Page 5 | Page 6 | Page 7 ]
       *   ^ refcount=1  ^ refcount=0 ...  ^ refcount=0
       *   ^ in partial  ^ in empty        ^ in empty
       *  
       *  The first allocated slot is returned (opium_slab_new_slot(slab, boss)) 
       *
       * The first slab page will be used immediately, the rest will go into 'empty' list.
       *   - pos: points to the memory after the first slab page.
       *   - end: points to the end of the allocated block.
       *
       *  Initialize the first page:
       *   - set its bitmask to initial bitmask (all slots are free)
       *   - initialize its list head and add it to the 'partial' list
       *
       * The remaining memory is split into additional slab pages:
       *  - each gets a pointer to its "boss" page,
       *  - its bitmask is fully empty (all slots free),
       *  - initialize list head and add it to the 'empty' list.
       *
       */

      u_char *pos = (u_char*)page + slab->page_size;
      u_char *end = (u_char*)page + slab->pages_per_alloc;

      /* Initialize the mask for a new slab page:
       *
       * - Lower 'item_count' bits are 0 -> represents free slots
       * - Higher bits are 1 -> out-of-range / unavailable (won`t be used)
       *
       * Why the high bits are 1:
       *  The slab mask is stored in a fixed-size integer
       *  (32 or 64 bits depending on the system).
       *  If 'item_count' < size of the integer, the remaining higher
       *  bits are not part of the usable slots. We mark them as 1 so
       *  that when all real slots are allocated, the mask becomes fully
       *  1s, making it easy to detect a fully occupied page. 
       * 
       * Example:
       *   11111111111000000000000000000000
       *        |                |
       *        | available bits (free slots) – 21 items
       *        unavailable bits – not used
       */

      page->mask = opium_slab_page_init(slab->item_count);
      page->refcount = 1;

      OPIUM_INIT_LIST_HEAD(&page->head);
      opium_list_add(&page->head, &slab->partial);

      opium_slab_page_t *current = NULL;

      opium_slab_slots_for_each(current, pos, end, slab->page_size) {
         current = (opium_slab_page_t*)pos;
         current->boss = page;
         current->mask = opium_slab_page_init(slab->item_count);
         current->refcount = 0;

         OPIUM_INIT_LIST_HEAD(&current->head);
         opium_list_add(&current->head, &slab->empty);
      }

      return opium_slab_new_slot(slab, page);

   }

   return NULL;
}

   void *
opium_slab_calloc(opium_slab_t *slab)
{
   assert(slab != NULL);

   /* Default alloc with memzero */
   void *ptr = opium_slab_alloc(slab);
   if (!ptr) {
      return NULL;
   }

   opium_memzero(ptr, slab->item_size);

   return ptr;
}

   void
opium_slab_free(opium_slab_t *slab, void *ptr)
{
   assert(slab != NULL);
   assert(ptr != NULL);

   slab->stats.reqs = slab->stats.reqs + 1;

   /*
    * 'slot_ptr' is the pointer to the full slot, including the header.
    * We subtract OPIUM_SLAB_SLOT_HEADER to get the original slot start,
    * which is required for computing the page base and slot index.
    */

   u_char *slot_ptr = (u_char*)ptr - OPIUM_SLAB_SLOT_HEADER;

   /*
    * It takes a pointer.
    * It converts it to an integer (uintptr_t) so bitwise ops are legal.
    * It uses a precomputed mask 'alignment_mask' to clear the low bit of the address.
    * Clearing those low bits produces the start address of the page that contains slot_ptr.
    * So: slot_ptr & alignment_mask = pointer rounded down to the nearest multiple of page_size.
    */

   opium_slab_page_t *page = (void*)((uintptr_t)slot_ptr & slab->alignment_mask);

   /*
    * Calculate the slot index of a pointer within a slab page.
    *
    * 'slot_ptr'   - pointer to the object
    * 'page->data' - start of the slab data block
    *
    * All objects are laid out consecutively:
    * page->data -> [item0][item1][item2]...[itemN]
    * Each object occupies slab->item_size bytes.
    *
    * distance = (u_char*)slot_ptr - (u_char*)page->data
    *   -> distance in bytes from the start of the page
    *
    * slot = distance / slab->item_size
    *   -> index of the object within the page
    *
    * Example:
    *   page->data    = 0x1000
    *   slot_ptr      = 0x1014
    *   slab->item_size = 8
    *   distance      = 0x1014 - 0x1000 = 0x14 = 20 bytes
    *   slot          = 20 / 8 = 2 (third element, zero-based)
    */

   size_t distance = (u_char*) slot_ptr - (u_char*) page->data; 
   size_t slot = distance / slab->item_size;

   /*
    * Each slab page can be either a boss or a slave
    * refcount != 0 -> This is the boss page
    * refcount == 0 -> This is the slave page, and its Boss value is stored in page->boss
    */

   opium_slab_page_t *boss = opium_unlikely(page->refcount != 0) ? page : page->boss;

   if (opium_unlikely(page->mask == OPIUM_SLAB_PAGE_BUSY)) {
      /*
       * OPIUM_SLAB_PAGE_BUSY is a mask for when all page slots are occupied
       * We free one slot. Since the page is no longer fully occupied, 
       * we move it from the full list to the partial list. 
       */

      page->mask = page->mask & ~(1ULL << slot);

      opium_list_del(&page->head);
      opium_list_add(&page->head, &slab->partial);

   } else if (opium_unlikely(opium_slab_page_one_used(page->mask, slab->item_count) == OPIUM_RET_OK)) {
      /* 
       * Checks whether there is only one occupied slot left on the page.
       * If yes, this may mean that after deallocation, the page will be
       * completely empty and can be returned to the OS 
       */

      if (opium_unlikely(boss->refcount == 1)) {
         opium_slab_page_t *current = NULL;
         u_char *pos = (u_char*) page + slab->page_size;
         u_char *end = (u_char*) page + slab->pages_per_alloc;

         if (opium_list_is_linked(&boss->head)) {
            opium_list_del(&boss->head);
         }

         opium_slab_slots_for_each(current, pos, end, slab->page_size) {
            if (opium_list_is_linked(&current->head)) {
               opium_list_del(&current->head);
            }
         }

         /*
          * Small pages (<= OPIUM_SLAB_PAGE_SIZE) were allocated via mmap -> free with munmap.
          * Large pages (> OPIUM_SLAB_PAGE_SIZE) were allocated via posix_memalign -> free with free.
          */
         if (slab->page_size <= OPIUM_SLAB_PAGE_SIZE) {
            opium_munmap(boss, slab->pages_per_alloc, slab->log);
         } else {
            opium_free(boss, slab->log);
         }

      } else {
         /* Guess */
         if (!opium_list_is_linked(&page->head)) {
            opium_list_add(&page->head, &slab->empty);
         }

         boss->refcount = boss->refcount - 1;
      }

   } else {
      /* If the page was not full and was no one left slot, simply clear the slot bit */
      page->mask = page->mask & ~(1ULL << slot);
   }

   slab->stats.used = slab->stats.used - 1;
}

   void
opium_slab_traverse(opium_slab_t *slab, opium_slab_trav_ctx func)
{
   assert(slab != NULL);
   assert(func != NULL);

   opium_slab_page_t *page, *tmp;

   opium_list_for_each_entry_safe(page, tmp, &slab->partial, head) {
      opium_slab_mask_t mask = page->mask;

      while (mask) {
         /* 
          * __builtin_ctzll(mask) returns the index of the first set bit (occupied slot). 
          * We clear the bit after processing to iterate over all set slots efficiently.
          */
         size_t slot = __builtin_ctzll(mask);
         func(opium_slab_slot(slab, page, slot));
         mask = mask & ~(1ULL << slot);
      }
   }

   opium_list_for_each_entry_safe(page, tmp, &slab->full, head) {
      for (size_t index = 0; index < slab->item_count; index++) {
         func(opium_slab_slot(slab, page, index));
      }
   }

}

void
opium_slab_stats(opium_slab_t *slab) {
   assert(slab != NULL);

   /*
    * - Displays global allocator statistics (total, used, reqs, fails)
    * - Lists all pages by category (Empty / Partial / Full)
    *
    *   For each boss page (refcount != 0), displays:
    *    - Page address
    *    - Page type (Empty / Partial / Full)
    *    - Refcount - number of occupied slots
    */

   opium_list_head_t *heads[] = {&slab->empty, &slab->partial, &slab->full};
   char *labels[] = {"Empty", "Partial", "Full"};

   opium_log_debug_inline(slab->log, "%45s\n", "Slab Stats"); 

   opium_log_debug_inline(slab->log,
         "%5s %15s %15s %10s %10s\n", "", 
         "Total", "Used", "Reqs", "Fails");

   opium_log_debug_inline(slab->log,
         "%5s %13zu %16zu %10zu %10zu\n", "", 
         slab->stats.total, slab->stats.used, slab->stats.reqs, slab->stats.fails);

   opium_log_debug_inline(slab->log, "%45s\n", "Slab Chunks");

   opium_log_debug_inline(slab->log,
         "%5s %15s %15s %10s %10s %10s\n", "", 
         "Schunk", "Type", "Refcount", "Used", "Free");

   for (size_t index = 0; index < 3; index++) {
      opium_list_head_t *current_head = heads[index];

      opium_slab_page_t *current, *tmp = NULL;

      opium_list_for_each_entry_safe(current, tmp, current_head, head) {
         if (current && current->refcount != 0) {
            size_t used_mask = current->mask & ((1ULL << slab->item_count) - 1);
            size_t used = __builtin_popcountll(used_mask);
            size_t free = slab->item_count - used;

            opium_log_debug_inline(slab->log, "%10s %p %12s %6zu %12zu %10zu\n",
                  "Address:", current, labels[index], current->refcount, used, free);
         }
      }

   }

}

