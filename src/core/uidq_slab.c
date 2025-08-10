#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uidq_slab.h"
#include "core/uidq_alloc.h"
#include "core/uidq_log.h"
#include "core/uidq_types.h"
#include "uidq_bitmask.h"
#include "uidq_utils.h"

static void uidq_slab_block_change(uidq_slab_block_t *block, size_t count, void *data, int index);
static int uidq_slab_alloc(uidq_slab_t *slab, void *data, size_t size);
static int uidq_slab_alloc_at_pos(uidq_slab_t *slab, void *data, size_t size, int pos, size_t block_demand);
static uidq_slab_block_t *uidq_slab_pushka(uidq_slab_t **pslab, void *data, size_t size, bool init);
static void uidq_slab_dealloc(uidq_slab_t *slab, int pos);

/**
 * @brief Creates a new slab structure.
 * @return Pointer to the created slab, or NULL on allocation failure.
 */
uidq_slab_t *uidq_slab_create() {
   // Allocate memory for the slab structure
   uidq_slab_t *slab = malloc(sizeof(uidq_slab_t));
   if (!slab) {
      return NULL;
   }
   return slab;
}

/**
 * @brief Initializes a slab with the specified block count and block size.
 * @return UIDQ_OK on success, UIDQ_ERROR on failure.
 */
int uidq_slab_init(uidq_slab_t *slab, uidq_log_t *log, size_t count, size_t block_size) {
   // Validate input parameters
   if (!slab || !log || slab->initialized || count < 1 || block_size <= 0) {
      return UIDQ_ERROR;
   }

   // Calculate total size and round to power of 2
   size_t size = count * block_size;
   size_t round_size = (size_t)uidq_round_pow(size, 2);
   if (round_size <= 1) {
      uidq_err(log, "Invalid rounded size: %zu for requested size: %zu\n", round_size, size);
      return UIDQ_ERROR;
   }
   size = round_size;

   // Initialize slab fields
   size_t block_count = size / block_size;
   slab->size = size;
   slab->block_count = block_count;
   slab->block_size = block_size;
   slab->block_busy = 0;
   slab->block_free = block_count;
   slab->last_free_pos = 0;
   slab->log = log;
   slab->initialized = true;

   // Allocate and initialize bitmask
   slab->bitmask = uidq_bitmask_create_and_init(slab->block_count, sizeof(uint64_t));
   if (!slab->bitmask) {
      uidq_err(slab->log, "Failed to initialize bitmask\n");
      return UIDQ_ERROR;
   }

   // Allocate block metadata
   slab->block_data = calloc(slab->block_count, sizeof(uidq_slab_block_t));
   if (!slab->block_data) {
      uidq_err(slab->log, "Failed to allocate block metadata\n");
      goto fail;
   }

   // Initialize blocks
   int ret = uidq_slab_blocks_init(slab, slab->block_count);
   if (ret != UIDQ_OK) {
      uidq_err(slab->log, "Failed to initialize blocks\n");
      goto fail;
   }

   // Allocate data buffer
   slab->data = calloc(1, size);
   if (!slab->data) {
      uidq_err(slab->log, "Failed to allocate data buffer\n");
      goto fail;
   }

   return UIDQ_OK;
fail:
   uidq_slab_exit(slab);
   return UIDQ_ERROR;
}

/**
 * @brief Creates and initializes a slab with the specified block count and size.
 * @return Pointer to the initialized slab, or NULL on failure.
 */
uidq_slab_t *uidq_slab_create_and_init(uidq_log_t *log, size_t count, size_t block_size) {
   // Validate log
   if (!log) {
      return NULL;
   }

   // Create a new slab
   uidq_slab_t *slab = uidq_slab_create();
   if (!slab) {
      uidq_err(log, "Failed to create slab: count=%zu, block_size=%zu\n", count, block_size);
      return NULL;
   }

   // Initialize the slab
   int ret = uidq_slab_init(slab, log, count, block_size);
   if (ret != UIDQ_OK) {
      uidq_err(log, "Failed to initialize slab: count=%zu, block_size=%zu\n", count, block_size);
      uidq_slab_free(slab);
      return NULL;
   }

   return slab;
}

/**
 * @brief Reinitializes a slab with a new block count.
 * @return UIDQ_OK on success, UIDQ_ERROR on failure.
 */
int uidq_slab_reinit(uidq_slab_t **pslab, size_t count) {
   // Validate input parameters
   if (!pslab || !*pslab || !(*pslab)->initialized || count < 1) {
      return UIDQ_ERROR;
   }

   uidq_slab_t *slab = *pslab;
   // Create and initialize a new slab
   uidq_slab_t *new_slab = uidq_slab_create_and_init(slab->log, count, slab->block_size);
   if (!new_slab) {
      uidq_err(slab->log, "Failed to create new slab for reinitialization\n");
      return UIDQ_ERROR;
   }

   // Copy metadata and data to the new slab
   size_t copy_block = count > slab->block_count ? slab->block_count : count;
   size_t copy_size = copy_block * slab->block_size;
   for (size_t index = 0; index < copy_block; index++) {
      new_slab->block_data[index].count = slab->block_data[index].count;
   }
   if (uidq_bitmask_copy(new_slab->bitmask, slab->bitmask) != UIDQ_OK) {
      uidq_err(slab->log, "Failed to copy bitmask during reinitialization\n");
      uidq_slab_free(new_slab);
      return UIDQ_ERROR;
   }
   memcpy(new_slab->data, slab->data, copy_size);

   // Free old slab and update pointer
   uidq_slab_free(slab);
   *pslab = new_slab;
   return UIDQ_OK;
}

/**
 * @brief Updates the properties of a slab block.
 * @return None.
 */
void uidq_slab_block_change(uidq_slab_block_t *block, size_t count, void *data, int index) {
   // Set block properties
   block->count = count;
   block->data = data;
   block->index = index;
}

/**
 * @brief Initializes a slab block with specified parameters.
 * @return None.
 */
void uidq_slab_block_init(uidq_slab_block_t *block, size_t count, void *data, int index) {
   // Initialize block using change function
   uidq_slab_block_change(block, count, data, index);
}

/**
 * @brief Resets a slab block to an invalid state.
 * @return None.
 */
void uidq_slab_block_exit(uidq_slab_block_t *block) {
   // Reset block to invalid state
   uidq_slab_block_change(block, 0, NULL, -1);
}

/**
 * @brief Initializes all blocks in the slab to an invalid state.
 * @return UIDQ_OK on success, UIDQ_ERROR on invalid input.
 */
int uidq_slab_blocks_init(uidq_slab_t *slab, size_t count) {
   // Validate input parameters
   if (!slab || count == 0) {
      return UIDQ_ERROR;
   }

   // Initialize each block to an invalid state
   for (size_t i = 0; i < count; i++) {
      uidq_slab_block_t *block = uidq_slab_get_block(slab, (int)i);
      uidq_slab_block_exit(block);
   }
   return UIDQ_OK;
}

/**
 * @brief Allocates a block with the given data and size.
 * @return Position of the allocated block, or -1 on error.
 */
int uidq_slab_alloc(uidq_slab_t *slab, void *data, size_t size) {
   // Validate slab and size
   if (!uidq_slab_is_valid(slab, 0, size)) {
      return UIDQ_ERROR;
   }

   // Check for size overflow
   size_t block_size = slab->block_size;
   if (size > SIZE_MAX - block_size + 1) {
      uidq_err(slab->log, "Requested size too large: %zu\n", size);
      return UIDQ_ERROR;
   }

   // Calculate required blocks
   size_t block_demand = (size + block_size - 1) / block_size;
   if (block_demand > slab->block_free) {
      uidq_err(slab->log, "Insufficient free blocks: demand=%zu, free=%zu\n",
            block_demand, slab->block_free);
      return UIDQ_SLAB_BUSY;
   }

   // Allocate block at any available position
   return uidq_slab_alloc_at_pos(slab, data, size, -1, block_demand);
}

/**
 * @brief Allocates a block at a specified position with given data and block demand.
 * @return Allocated position, or -1 on error.
 */
int uidq_slab_alloc_at_pos(uidq_slab_t *slab, void *data, size_t size, int pos, size_t block_demand) {
   // Determine if allocation is at a specific position or free
   bool free = pos < 0 ? true : false;
   pos = pos < 0 ? 0 : pos;

   // Validate slab, position, and block demand
   if (!uidq_slab_is_valid(slab, pos, size) || block_demand > slab->block_free) {
      return UIDQ_ERROR;
   }
   if (block_demand == 0) {
      uidq_err(slab->log, "Invalid block demand: %zu\n", block_demand);
      return UIDQ_ERROR;
   }

   // Get memory position for allocation
   uint8_t *mem_pos = (uint8_t *)uidq_slab_get(slab, pos);
   size_t alloc_size = block_demand * slab->block_size;

   // Copy data or clear memory
   if (data) {
      memcpy(mem_pos, data, size);
      if (alloc_size > size) {
         memset(mem_pos + size, 0, alloc_size - size);
      }
   } else {
      memset(mem_pos, 0, alloc_size);
   }

   // Allocate blocks in bitmask
   int set_pos = -1;
   if (!free) {
      for (size_t index = pos; index < pos + block_demand; index++) {
         uidq_slab_dealloc(slab, index);
      }
      set_pos = uidq_bitmask_add_force(slab->bitmask, pos, block_demand);
   } else {
      set_pos = uidq_bitmask_add(slab->bitmask, pos, block_demand);
   }
   if (set_pos < 0 || (size_t)set_pos + block_demand > slab->block_count) {
      uidq_err(slab->log, "Failed to allocate free block\n");
      return UIDQ_ERROR;
   }

   // Initialize block metadata
   uidq_slab_block_t *block = uidq_slab_get_block(slab, set_pos);
   uidq_slab_block_init(block, block_demand, mem_pos, set_pos);

   // Update block counts
   uidq_slab_update_counts(slab, block_demand, -block_demand);

   return set_pos;
}

/**
 * @brief Allocates a block with optional reinitialization on busy condition.
 * @return Pointer to the allocated block, or NULL on error.
 */
uidq_slab_block_t *uidq_slab_pushka(uidq_slab_t **pslab, void *data, size_t size, bool init) {
   // Validate input slab pointer
   if (!pslab || !*pslab) {
      return NULL;
   }

   uidq_slab_t *slab = *pslab;

   // Attempt allocation
   int pos = uidq_slab_alloc(slab, data, size);
   if (pos == UIDQ_SLAB_BUSY && init) {
      // Reinitialize slab with doubled capacity
      int succ = uidq_slab_reinit(pslab, slab->block_count * 2 - 1);
      if (succ != UIDQ_OK) {
         uidq_err(slab->log, "Failed to reinitialize slab\n");
         return NULL;
      }
      slab = *pslab;
      pos = uidq_slab_alloc(slab, data, size);
      if (pos < 0) {
         uidq_err(slab->log, "Failed to allocate after reinitialization\n");
         return NULL;
      }
   } else if (pos < 0) {
      uidq_err(slab->log, "Allocation failed: pos=%d\n", pos);
      return NULL;
   }

   // Get allocated data
   void *node = uidq_slab_get(slab, pos);
   if (!node) {
      uidq_err(slab->log, "Failed to retrieve allocated data\n");
      uidq_slab_dealloc(slab, pos);
      return NULL;
   }

   // Log allocation position
   uidq_debug(slab->log, "Allocated block at position: %d\n", pos);

   return uidq_slab_get_block(slab, pos);
}

/**
 * @brief Allocates a block without reinitialization.
 * @return Pointer to the allocated block, or NULL on error.
 */
uidq_slab_block_t *uidq_alloc_push(uidq_slab_t **pslab, void *data, size_t size) {
   return uidq_slab_pushka(pslab, data, size, false);
}

/**
 * @brief Allocates a block with reinitialization on busy condition.
 * @return Pointer to the allocated block, or NULL on error.
 */
uidq_slab_block_t *uidq_alloc_pushin(uidq_slab_t **pslab, void *data, size_t size) {
   return uidq_slab_pushka(pslab, data, size, true);
}

/**
 * @brief Copies a block from a source slab to a destination slab.
 * @return Pointer to the copied block in the destination slab, or NULL on error.
 */
uidq_slab_block_t *uidq_slab_block_copy(uidq_slab_t *src, int src_pos, uidq_slab_t *dst, int dst_pos) {
   // Validate source slab and position
   if (!uidq_slab_is_valid(src, src_pos, 0)) {
      return NULL;
   }

   // Validate destination slab and position
   if (!uidq_slab_is_valid(dst, dst_pos, 0)) {
      uidq_err(src->log, "Invalid destination slab or position: %d\n", dst_pos);
      return NULL;
   }

   // Get source and destination blocks
   uidq_slab_block_t *src_block = uidq_slab_get_block(src, src_pos);
   uidq_slab_block_t *dst_block = uidq_slab_get_block(dst, dst_pos);

   // Validate source block
   if (!uidq_slab_block_is_valid(src_block)) {
      uidq_err(src->log, "Invalid source block for copy\n");
      return NULL;
   }

   // Validate destination block
   if (!dst_block) {
      uidq_err(src->log, "Invalid destination block for copy\n");
      return NULL;
   }

   // Deallocate destination block if already allocated
   if (dst_block->count > 0) {
      uidq_slab_pop(dst, dst_pos);
   }

   // Copy data from source to destination
   void *data = uidq_slab_get(src, src_pos);
   size_t size = src_block->count * src->block_size;
   uidq_slab_block_t *result = uidq_slab_push(&dst, data, size);
   if (!result) {
      uidq_err(src->log, "Failed to copy data to destination slab\n");
      return NULL;
   }
   return result;
}

/**
 * @brief Reallocates a block at the specified position with new data and size.
 * @return New position of the allocated block, or -1 on error.
 */
int uidq_block_realloc(uidq_slab_t *slab, int pos, void *new_data, size_t new_size) {
   // Validate slab and input parameters
   if (!uidq_slab_is_valid(slab, pos, 0)) {
      return UIDQ_ERROR;
   }
   if (!new_data || new_size < 1 || new_size > slab->size) {
      uidq_err(slab->log, "Invalid data or size: %zu\n", new_size);
      return UIDQ_ERROR;
   }

   // Get block and calculate demand
   uidq_slab_block_t *block = uidq_slab_get_block(slab, pos);
   size_t block_size = slab->block_size;
   size_t block_demand = (new_size + block_size - 1) / block_size;
   size_t old_count = block->count;

   // Validate block count
   if (old_count == 0) {
      uidq_err(slab->log, "Invalid block count: %zu\n", old_count);
      return UIDQ_ERROR;
   }

   // Determine if block needs to grow or shrink
   int grow = (block_demand > old_count);
   size_t diff = grow ? block_demand - old_count : old_count - block_demand;

   int new_pos = pos;
   if (grow) {
      // Allocate additional blocks for growing
      new_pos = uidq_bitmask_add_with_fallback(slab->bitmask, pos, old_count, block_demand);
      if (new_pos < 0) {
         uidq_err(slab->log, "Failed to find free blocks for reallocation\n");
         return UIDQ_ERROR;
      }
      block->count = 0;
      uidq_slab_block_t *new_block = uidq_slab_get_block(slab, new_pos);
      new_block->count = block_demand;
   } else {
      // Free excess blocks for shrinking
      uidq_bitmask_del(slab->bitmask, pos + block_demand, old_count - block_demand);
      block->count = block_demand;
   }

   // Update block counts
   size_t set_busy = (grow ? diff : -diff);
   size_t set_free = (grow ? -diff : diff);
   uidq_slab_update_counts(slab, set_busy, set_free);

   // Copy new data to the block
   uint8_t *mem_pos = (uint8_t*)uidq_slab_get(slab, grow ? new_pos : pos);
   memset(uidq_slab_get(slab, pos), 0, block_size * old_count);
   memcpy(mem_pos, new_data, new_size);

   return new_pos;
}

/**
 * @brief Validates a slab and position.
 * @return true if the slab and position are valid, false otherwise.
 */
bool uidq_slab_is_valid(const uidq_slab_t *slab, int pos, size_t size) {
   // Check slab structure and initialization
   if (!slab || !slab->initialized || !slab->data || !slab->bitmask) {
      return false;
   }

   // Validate position range
   if (pos < 0 || (size_t)pos >= slab->block_count) {
      uidq_err(slab->log, "Invalid position: %d >= %zu\n", pos, slab->block_count);
      return false;
   }

   // Ensure size does not exceed slab capacity
   if (size > slab->size) {
      uidq_err(slab->log, "Requested size too large: %zu > %zu\n", size, slab->size);
      return false;
   }
   return true;
}

/**
 * @brief Checks if a slab is empty.
 * @return true if the slab is empty (all blocks are free), false otherwise.
 */
bool uidq_slab_is_empty(const uidq_slab_t *slab) {
   // Validate slab before checking emptiness
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      return false;
   }
   return slab->block_free == slab->block_count;
}

/**
 * @brief Retrieves a pointer to the slab block at the specified position.
 * @return Pointer to the block, or NULL if the slab is invalid or position is out of range.
 */
uidq_slab_block_t *uidq_slab_get_block(const uidq_slab_t *slab, int pos) {
   // Validate slab and position
   if (!slab || pos < 0 || (size_t)pos >= slab->block_count) {
      return NULL;
   }
   return &slab->block_data[pos];
}

/**
 * @brief Retrieves a pointer to the data at the specified position in the slab.
 * @return Pointer to the data, or NULL if the slab or position is invalid.
 */
void *uidq_slab_get(uidq_slab_t *slab, int pos) {
   // Validate slab and position
   if (!uidq_slab_is_valid(slab, pos, 0)) {
      return NULL;
   }
   return (void *)((char *)slab->data + slab->block_size * pos);
}

/**
 * @brief Returns the number of free blocks in the slab.
 * @return Number of free blocks, or -1 if the slab is invalid.
 */
int uidq_slab_get_block_free(const uidq_slab_t *slab) {
   // Validate slab before accessing free block count
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      return -1;
   }
   return slab->block_free;
}

/**
 * @brief Returns the number of busy blocks in the slab.
 * @return Number of busy blocks, or -1 if the slab is invalid.
 */
int uidq_slab_get_block_busy(const uidq_slab_t *slab) {
   // Validate slab before accessing busy block count
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      return -1;
   }
   return slab->block_busy;
}

/**
 * @brief Checks if a block at the specified position is allocated.
 * @return Number of allocated blocks at the position, or -1 if invalid.
 */
int uidq_slab_is_block_allocated(const uidq_slab_t *slab, int pos) {
   // Validate slab and position
   if (!uidq_slab_is_valid(slab, pos, 0)) {
      return -1;
   }
   return slab->block_data[pos].count;
}

/**
 * @brief Updates the counts of busy and free blocks in a slab.
 * @return None.
 */
void uidq_slab_update_counts(uidq_slab_t *slab, int64_t busy_delta, int64_t free_delta) {
   // Validate slab before updating counts
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      return;
   }
   slab->block_busy += busy_delta;
   slab->block_free += free_delta;
}

/**
 * @brief Checks if a slab block is valid.
 * @return true if the block is valid (positive count and non-negative index), false otherwise.
 */
bool uidq_slab_block_is_valid(const uidq_slab_block_t *block) {
   return block && block->count > 0 && block->index >= 0;
}

/**
 * @brief Prints general information about the slab.
 * @return None.
 */
void uidq_slab_info(uidq_slab_t *slab) {
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      return;
   }
   // Output slab metadata for debugging
   uidq_debug(slab->log, "Slab info: address=%p, total_size=%zu, block_count=%zu, block_size=%zu, busy_blocks=%zu, free_blocks=%zu\n",
         slab, slab->size, slab->block_count, slab->block_size, slab->block_busy, slab->block_free);
}

/**
 * @brief Prints information about a single block in the slab.
 * @return None.
 */
void uidq_slab_info_block(uidq_slab_t *slab, int pos) {
   // Validate slab and position
   if (!uidq_slab_is_valid(slab, pos, 0)) {
      return;
   }

   // Get memory position
   uint8_t *mem_pos = (uint8_t*)uidq_slab_get(slab, pos);
   if (!mem_pos) {
      uidq_err(slab->log, "Failed to retrieve memory position\n");
      return;
   }

   // Get block metadata
   uidq_slab_block_t *block = uidq_slab_get_block(slab, pos);
   size_t count = block->count;
   if (count <= 0) {
      uidq_err(slab->log, "Invalid block data count: %zu\n", count);
      return;
   }

   // Calculate block size and print data
   size_t block_size = slab->block_size;
   size_t full_size = count * block_size;
   for (size_t index = 0; index < full_size; index++) {
      printf("%02X ", mem_pos[index]);
      if ((index + 1) % block_size == 0) break;
   }
   putchar('\n');

   // Print ASCII representation
   for (size_t index = 0; index < full_size; index++) {
      char sym = mem_pos[index];
      putchar((sym >= 32 && sym <= 126) ? sym : '?');
   }
   putchar('\n');
}

/**
 * @brief Resets the slab data and block counts.
 * @return None.
 */
void uidq_slab_reset(uidq_slab_t *slab) {
   // Validate slab before resetting
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      return;
   }

   // Reset bitmask and clear data
   uidq_bitmask_reset(slab->bitmask);
   memset(slab->block_data, 0, slab->block_count * sizeof(uidq_slab_block_t));
   memset(slab->data, 0, slab->size);

   // Update block counts
   slab->block_busy = 0;
   slab->block_free = slab->block_count;
}

/**
 * @brief Deallocates a block starting at the specified position.
 * @return None.
 */
void uidq_slab_dealloc(uidq_slab_t *slab, int pos) {
   // Validate slab and position
   if (!uidq_slab_is_valid(slab, pos, 0)) {
      return;
   }

   // Get block metadata
   uidq_slab_block_t *block = uidq_slab_get_block(slab, pos);
   size_t count = block->count;

   // Validate block count
   if (count <= 0) {
      uidq_err(slab->log, "Invalid block data count: %zu\n", count);
      return;
   }

   // Clear memory and update bitmask
   size_t block_size = slab->block_size;
   uint8_t *mem_pos = (uint8_t*)uidq_slab_get(slab, pos);
   size_t mem_size = block_size * count;
   memset(mem_pos, 0, mem_size);
   uidq_bitmask_del(slab->bitmask, pos, count);

   // Update block counts and reset block
   uidq_slab_update_counts(slab, -count, count);
   uidq_slab_block_exit(block);
}

/**
 * @brief Deallocates a block at the specified position (variant).
 * @return None.
 */
void uidq_slab_dealloc_v(uidq_slab_t *slab, int pos) {
   uidq_slab_dealloc(slab, pos);
}

/**
 * @brief Frees the slab and its resources.
 * @return None.
 */
void uidq_slab_exit(uidq_slab_t *slab) {
   // Validate slab before freeing
   if (!uidq_slab_is_valid(slab, 0, 0)) {
      return;
   }

   // Reset slab fields
   slab->size = 0;
   slab->block_count = 0;
   slab->block_size = 0;
   slab->block_busy = 0;
   slab->block_free = 0;

   // Free allocated resources
   if (slab->data) {
      uidq_free_pointer(&slab->data);
   }
   if (slab->block_data) {
      uidq_free_pointer((void**)&slab->block_data);
   }
   if (slab->bitmask) {
      uidq_bitmask_free(slab->bitmask);
      slab->bitmask = NULL;
   }
}

/**
 * @brief Frees the slab structure and its resources.
 * @return None.
 */
void uidq_slab_free(uidq_slab_t *slab) {
   if (!slab) return;
   uidq_slab_exit(slab);
   uidq_free(slab);
}
