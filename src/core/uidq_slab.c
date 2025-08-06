#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uidq_slab.h"
#include "uidq_bitmask.h"
#include "uidq_utils.h"

#define UIDQ_SLAB_DEBUG UIDQ_DEBUG_ENABLED
#if UIDQ_SLAB_DEBUG == UIDQ_DEBUG_DISABLED
#define DEBUG_INFO(fmt, ...) do {} while(0)
#define DEBUG_ERR(fmt, ...)  do {} while(0)
#endif

/**
 * @brief Updates busy and free block counts.
 */
void uidq_slab_update_counts(uidq_slab_t *slab, int64_t busy_delta, int64_t free_delta) {
   if (!uidq_slab_is_valid(slab, 0, 0)) return; 
   slab->block_busy += busy_delta;
   slab->block_free += free_delta;
}

/**
 * @brief Validates slab and position.
 * @return true if valid, false otherwise.
 */
bool uidq_slab_is_valid(const uidq_slab_t *slab, int pos, size_t size) {
   if (!slab || !slab->initialized || !slab->data || !slab->bitmask) {
      DEBUG_ERR("Invalid slab structure\n");
      return false;
   }
   if (pos < 0 || (size_t)pos >= slab->block_count) {
      DEBUG_ERR("Invalid position: %d >= %zu\n", pos, slab->block_count);
      return false;
   }
   if (size > slab->size) {
      DEBUG_ERR("Invalid size: %zu > %zu\n", size, slab->size);
      return false;
   }
   return true;
}

/**
 * @brief Checks if slab is empty.
 */
bool uidq_slab_is_empty(uidq_slab_t *slab) {
   if (!uidq_slab_is_valid(slab, 0, 0)) return false;
   return slab->block_free == slab->block_count;
}

/**
 * @brief Returns blocks free count in slab.
 */
int uidq_slab_get_block_free(const uidq_slab_t *slab) {
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      DEBUG_ERR("Invalid slab.\n");
      return -1;
   }
   return slab->block_free;
}

/**
 * @brief Returns blocks busy count in slab.
 */
int uidq_slab_get_block_busy(const uidq_slab_t *slab) {
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      DEBUG_ERR("Invalid slab.\n");
      return -1;
   }
   return slab->block_busy;
}

/**
 * @brief Checks if a block at start_pos is allocated.
 * @return true if allocated, false otherwise.
 */
int uidq_slab_is_block_allocated(const uidq_slab_t *slab, int pos) {
   if (!uidq_slab_is_valid(slab, pos, 0)) {
      DEBUG_ERR("Invalid slab or start_pos: %d\n", pos);
      return -1;
   }
   return slab->block_data[pos].count;
}

/**
 * @brief Returns pointer to block data at given position.
 */
uidq_slab_block_t *uidq_slab_get_block(const uidq_slab_t *slab, int pos) {
   if (!uidq_slab_is_valid(slab, pos, 0)) {
      DEBUG_ERR("Invalid slab or index: %d\n", pos);
      return NULL;
   }
   return &slab->block_data[pos];
}

/**
 * @brief Copies a block from src slab at src_pos to dst slab at dst_pos.
 * @return position allocated in dst, or -1 on error.
 */
int uidq_slab_block_copy(uidq_slab_t *src, int src_pos, uidq_slab_t *dst, int dst_pos) {
   if (!uidq_slab_is_valid(src, src_pos, 0)) {
      DEBUG_ERR("Invalid slab or src_pos: %d\n", src_pos);
      return -1;
   }

   if (!uidq_slab_is_valid(dst, dst_pos, 0)) {
      DEBUG_ERR("Invalid slab or dst_pos: %d\n", dst_pos);
      return -1;
   }

   uidq_slab_block_t *src_block = uidq_slab_get_block(src, src_pos);
   uidq_slab_block_t *dst_block = uidq_slab_get_block(dst, dst_pos);

   if (!src_block || !dst_block) {
      DEBUG_ERR("Invalid blocks for copy.\n");
      return -1;
   }

   int src_count = src_block->count; 
   int dst_count = dst_block->count; 

   if (src_count == 0) {
      DEBUG_ERR("Invalid block counts for copy: src_count=%zu, dst_count=%zu\n", src_block->count, dst_block->count);
      return -1;
   }

   if (dst_count > 0) {
      uidq_slab_dealloc(dst, dst_pos);
   }

   void *data = uidq_slab_get(src, src_pos);
   size_t size = src_block->count * src->block_size;
   int alloc = uidq_slab_alloc(dst, data, size);
   if (alloc < 0) {
      DEBUG_ERR("Failed to allocate from src into dst. data: %s, size: %zu\n", (char*)data, size);
      return alloc;
   }

   return alloc;
}

/**
 * @brief Returns pointer to data at given start position in slab.
 */
void *uidq_slab_get(uidq_slab_t *slab, int pos) {
   if (!uidq_slab_is_valid(slab, pos, 0)) {
      DEBUG_ERR("Invalid slab or start_pos: %d\n", pos);
      return NULL;
   }
   return (void *)((char *)slab->data + slab->block_size * pos);
}

/**
 * @brief Prints slab general info.
 */
void uidq_slab_info(uidq_slab_t *slab) {
   DEBUG_INFO("Slab info: addr: %p; total size: %zu; Blocks: count: %zu, size: %zu, busy: %zu, free: %zu\n",
         slab, slab->size, slab->block_count, slab->block_size, slab->block_busy, slab->block_free);
}

/**
 * @brief Prints info about a single block.
 */
void uidq_slab_info_block(uidq_slab_t *slab, int pos) {
   if (!uidq_slab_is_valid(slab, pos, 0)) {
      DEBUG_ERR("Start position > block count.\n");
      return;
   }

   uint8_t *mem_pos = (uint8_t*)uidq_slab_get(slab, pos);
   uidq_slab_block_t *block = uidq_slab_get_block(slab, pos);
   size_t count = block->count;
   if (count <= 0) {
      DEBUG_ERR("Invalid block data count.\n");
      return;
   }

   size_t block_size = slab->block_size;
   size_t full_size = count * block_size;

   for (size_t index = 0; index < full_size; index++) {
      printf("%02X ", mem_pos[index]);
      if ((index + 1) % block_size == 0) break; 
   }
   putchar('\n');

   for (size_t index = 0; index < full_size; index++) {
      char sym = mem_pos[index];
      if (sym >= 32 && sym <= 126) {
         putchar(sym);
      } else {
         putchar('?');
      }
   }
   putchar('\n');
}

/**
 * @brief Resets slab data and counts.
 */
void uidq_slab_reset(uidq_slab_t *slab) {
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      DEBUG_ERR("Slab is invalid\n");
      return;
   }
   uidq_bitmask_reset(slab->bitmask);
   memset(slab->block_data, 0, slab->block_count * sizeof(uidq_slab_block_t));
   memset(slab->data, 0, slab->size);
   slab->block_busy = 0;
   slab->block_free = slab->block_count;
}

/**
 * @brief Reallocates block at pos with new data and size.
 */
int uidq_block_realloc(uidq_slab_t *slab, int pos, void *new_data, size_t new_size) {
   if (!uidq_slab_is_valid(slab, pos, 0)) {
      DEBUG_ERR("Slab is invalid\n");
      goto fail;
   }

   if (!new_data || new_size < 1 || new_size > slab->size) {
      DEBUG_ERR("Invalid output: data or size.\n");
      goto fail;
   }

   uidq_slab_block_t *block = uidq_slab_get_block(slab, pos);
   size_t block_size    = slab->block_size;
   size_t block_demand  = (new_size + block_size - 1) / block_size;
   size_t old_count     = block->count;

   if (old_count == 0) {
      goto fail;
   }

   int grow = (block_demand > old_count);
   size_t diff = grow ? block_demand - old_count : old_count - block_demand;

   int new_pos = pos;
   if (grow) {
      new_pos = uidq_bitmask_add_with_fallback(slab->bitmask, pos, old_count, block_demand);
      if (new_pos < 0) {
         DEBUG_ERR("Failed to find free bit.\n");
         goto fail; 
      }
      block->count = 0;
      uidq_slab_block_t *new_block = uidq_slab_get_block(slab, new_pos);
      new_block->count = block_demand;
   } else {
      uidq_bitmask_del(slab->bitmask, pos + block_demand, old_count - block_demand);
      block->count = block_demand;
   }

   size_t set_busy = (grow ? diff : -diff);
   size_t set_free = (grow ? -diff : diff);
   uidq_slab_update_counts(slab, set_busy, set_free);

   uint8_t *mem_pos = (uint8_t*)uidq_slab_get(slab, grow ? new_pos : pos);
   memset(uidq_slab_get(slab, pos), 0, block_size * old_count);
   memcpy(mem_pos, new_data, new_size);

   return new_pos;

fail:
   DEBUG_ERR("uidq_block_realloc failed.\n");
   return -1;
}

/**
 * @brief Allocates a block at specified position with given data and block demand.
 * @return allocated position, or -1 on error.
 */
int uidq_slab_alloc_at_pos(uidq_slab_t *slab, void *data, size_t size, int pos, size_t block_demand) {
   bool free = pos < 0 ? true : false;
   pos = pos < 0 ? 0 : pos;
   if (!uidq_slab_is_valid(slab, pos, size) || block_demand > slab->block_free) return -1;

   if (block_demand == 0) {
      DEBUG_ERR("Invalid block_demand: %zu\n", block_demand);
      return -1;
   }

   uint8_t *mem_pos = (uint8_t *)uidq_slab_get(slab, pos);
   size_t alloc_size = block_demand * slab->block_size;

   if (data) {
      memcpy(mem_pos, data, size);
      if (alloc_size > size) {
         memset(mem_pos + size, 0, alloc_size - size);
      }
   } else {
      memset(mem_pos, 0, alloc_size);
   }

   DEBUG_FUNC("Block demand: %zu\n", block_demand);

   int set_pos = -1;
   if (!free) {
      for (size_t index = pos; index < block_demand; index++) {
         uidq_slab_dealloc(slab, index);
      }
      set_pos = uidq_bitmask_add_force(slab->bitmask, pos, block_demand);
   } else {
      set_pos = uidq_bitmask_add(slab->bitmask, pos, block_demand);
   }

   if (set_pos < 0 || (size_t)set_pos + block_demand > slab->block_count) {
      DEBUG_ERR("Failed to find free block\n");
      return -1;
   }

   uidq_slab_block_t *block = uidq_slab_get_block(slab, set_pos);
   block->count = block_demand;

   uidq_slab_update_counts(slab, block_demand, -block_demand);

   return pos;
}

/**
 * @brief Allocates block with given data and size.
 * @return position allocated or -1 on error.
 */
int uidq_slab_alloc(uidq_slab_t *slab, void *data, size_t size) {
   if (!uidq_slab_is_valid(slab, 0, size)) return -1;

   size_t block_size   = slab->block_size;
   size_t block_demand = (size + block_size - 1) / block_size;

   DEBUG_FUNC("threshold: %zu, size: %zu, block_demand: %zu\n", slab->block_size, size, block_demand);

   if (block_demand > slab->block_free) {
      DEBUG_ERR("Insufficient blocks: demand=%zu, free=%zu\n", block_demand, slab->block_free);
      return -1;
   }

   return uidq_slab_alloc_at_pos(slab, data, size, -1, block_demand);
}

/**
 * @brief Deallocates block starting at pos.
 */
void uidq_slab_dealloc(uidq_slab_t *slab, int pos) {
   if (!uidq_slab_is_valid(slab, pos, 0)) goto fail;

   uidq_slab_block_t *block = uidq_slab_get_block(slab, pos);
   size_t count = block->count;
   DEBUG_FUNC("BLOCK COUNT: %zu\n", count);
   if (count <= 0) {
      DEBUG_ERR("Invalid block data count.\n");
      goto fail;
   }

   size_t block_size = slab->block_size;
   uint8_t *mem_pos = (uint8_t*)uidq_slab_get(slab, pos);
   size_t mem_size = block_size * count;

   memset(mem_pos, 0, mem_size);
   uidq_bitmask_del(slab->bitmask, pos, count);
   uidq_slab_update_counts(slab, -count, count);
   block->count = 0;

fail:
   return;
}

int uidq_slab_init(uidq_slab_t *slab, size_t count, size_t block_size) {
   if (slab->initialized) {
      DEBUG_ERR("Slab structure is initialized.\n");
      goto fail;
   }
   if (count < 1 || block_size <= 0) {
      goto fail;
   }
   size_t size = count * block_size;
   size_t round_size = (size_t)uidq_round_pow(size, 2);
   if (round_size <= 1) {
      printf("Failed to allocate, rounded size = %zu for requested size = %zu\n", round_size, size);
      goto fail;
   }
   size = round_size;

   size_t block_count = size / block_size;
   size_t block_busy = 0;
   size_t block_free = block_count;

   slab->size        = size;
   slab->block_count = block_count;
   slab->block_size  = block_size;
   slab->block_busy  = block_busy;
   slab->block_free  = block_free;
   slab->initialized = true;

   slab->bitmask = uidq_bitmask_create_and_init(slab->block_count, sizeof(uint64_t));
   if (!slab->bitmask) {
      DEBUG_ERR("Failed to initialize bitmask structure.\n");
      goto fail;
   }
   slab->block_data = calloc(slab->block_count, sizeof(uidq_slab_block_t)); 
   if (!slab->block_data) {
      DEBUG_ERR("Failed to initialize bitmask block data.\n");
      goto fail;
   }
   slab->data = calloc(1, size);
   if (!slab->data) {
      DEBUG_ERR("Failed to initialize data in slab structure.\n");
      goto struct_exit;
   }
   return 0;
struct_exit:
   uidq_slab_exit(slab); 
fail:
   return -1;
}

void uidq_slab_exit(uidq_slab_t *slab) {
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      return;
   }
   slab->size        = 0;
   slab->block_count = 0;
   slab->block_size  = 0;
   slab->block_busy  = 0;
   slab->block_free  = 0;
   uidq_free_pointer(&slab->data);
   uidq_free_pointer((void**)&slab->block_data);
   if (slab->bitmask) {
      uidq_bitmask_free(slab->bitmask);
      slab->bitmask = NULL;
   }
}

uidq_slab_t *uidq_slab_create() {
   uidq_slab_t *slab = malloc(sizeof(uidq_slab_t)); 
   if (!slab) {
      DEBUG_ERR("Failed to allocate slab structure.\n");
      return NULL;
   }
   return slab;
}

void uidq_slab_free(uidq_slab_t *slab) {
   if (!slab) return;
   uidq_slab_exit(slab);
   free(slab);
}

uidq_slab_t *uidq_slab_create_and_init(size_t count, size_t block_size) {
   uidq_slab_t *slab = uidq_slab_create();
   if (!slab) {
      DEBUG_ERR("Failed to create slab: count=%zu, block_size=%zu\n", count, block_size);
      return NULL;
   }
   int ret = uidq_slab_init(slab, count, block_size);
   if (ret < 0) {
      DEBUG_ERR("Failed to initialize slab: count=%zu, block_size=%zu, ret=%d\n", count, block_size, ret);
      uidq_slab_free(slab);
      return NULL;
   }

   return slab;
}
