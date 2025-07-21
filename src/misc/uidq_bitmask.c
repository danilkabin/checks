#include "uidq_bitmask.h"
#include "uidq_utils.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define WORD_BITS (sizeof(uint64_t) * 8)

#define UIDQ_BITMASK_DEBUG UIDQ_DEBUG_ENABLED
#if UIDQ_BITMASK_DEBUG == UIDQ_DEBUG_DISABLED
#define DEBUG_INFO(fmt, ...) do {} while(0)
#define DEBUG_ERR(fmt, ...)  do {} while(0)
#endif

/**
 * @brief Checks if bitmask is valid.
 * @return true if valid, false otherwise.
 */
static inline bool is_valid(const uidq_bitmask_t *bitmask) {
   return bitmask && bitmask->initialized && bitmask->mask;
}

/**
 * @brief Checks if index is within bitmask capacity.
 * @return true if valid, false otherwise.
 */
static inline bool is_valid_index(const uidq_bitmask_t *bitmask, size_t index) {
   return is_valid(bitmask) && index < bitmask->bit_capacity;
}

/**
 * @brief Checks if the bitmask is empty.
 * @return 0 if empty, -1 otherwise or on error.
 */
int uidq_bitmask_is_empty(const uidq_bitmask_t *bitmask) {
   if (!is_valid(bitmask)) return -1;
   return bitmask->bit_count == 0 ? 0 : -1;
}

/**
 * @brief Clears all bits in the bitmask structure.
 */
void uidq_bitmask_bits_clear(uidq_bitmask_t *bitmask) {
   if (!is_valid(bitmask)) return;
   memset(bitmask->mask, 0, bitmask->word_capacity * (bitmask->word_bits / 8));
   bitmask->bit_count = 0;
}

/**
 * @brief Prints the bitmask for debugging purposes.
 */
void uidq_bitmask_debug(const uidq_bitmask_t *bitmask) {
   if (!is_valid(bitmask)) return;
   size_t index;
   size_t bit;
   for (index = 0; index < bitmask->word_capacity; index = index + 1) {
      for (bit = 0; bit < bitmask->word_bits; bit = bit + 1) {
         if (index * bitmask->word_bits + bit >= bitmask->bit_capacity) break;
         printf("%d", (int)((bitmask->mask[index] >> bit) & 1));
         if ((index * bitmask->word_bits + bit + 1) % 64 == 0) {
            printf("\n");
         }
      }
   }
   printf("\n");
   DEBUG_INFO("bitmask busy: %zu\n", bitmask->bit_count);
}

/**
 * @brief Creates a new bitmask structure.
 * @return Pointer to new bitmask or NULL on failure.
 */
uidq_bitmask_t *uidq_bitmask_create(void) {
   uidq_bitmask_t *bitmask = calloc(1, sizeof(uidq_bitmask_t));
   if (!bitmask) {
      DEBUG_ERR("Failed to allocate bitmask\n");
      return NULL;
   }
   return bitmask;
}

/**
 * @brief Frees bitmask structure and its resources.
 */
void uidq_bitmask_free(uidq_bitmask_t *bitmask) {
   if (!bitmask) return;
   uidq_bitmask_exit(bitmask);
   free(bitmask);
}

/**
 * @brief Initializes bitmask structure.
 * @return 0 on success, -1 on failure.
 */
int uidq_bitmask_init(uidq_bitmask_t *bitmask, size_t bit_capacity, size_t word_size) {
   if (!bitmask || bit_capacity == 0 || word_size == 0) {
      DEBUG_ERR("Invalid: bit_capacity=%zu, word_size=%zu\n", bit_capacity, word_size);
      return -1;
   }
   uidq_free_pointer((void**)&bitmask->mask);
   bitmask->initialized = false;
   bitmask->word_bits = word_size * 8;
   bitmask->word_capacity = (bit_capacity + bitmask->word_bits - 1) / bitmask->word_bits;
   bitmask->bit_capacity = bit_capacity;
   bitmask->bit_count = 0;
   bitmask->mask = calloc(bitmask->word_capacity, word_size);
   if (!bitmask->mask) {
      DEBUG_ERR("Failed to allocate bitmask memory\n");
      return -1;
   }
   bitmask->initialized = true;
   return 0;
}

/**
 * @brief Frees resources associated with the bitmask.
 */
void uidq_bitmask_exit(uidq_bitmask_t *bitmask) {
   if (!is_valid(bitmask)) return;
   uidq_free_pointer((void**)&bitmask->mask);
   bitmask->mask = NULL;
   bitmask->bit_capacity = 0;
   bitmask->word_bits = 0;
   bitmask->word_capacity = 0;
   bitmask->bit_count = 0;
   bitmask->initialized = false;
}

/**
 * @brief Creates and initializes a bitmask structure.
 * @return Pointer to the new bitmask or NULL on failure.
 */
uidq_bitmask_t *uidq_bitmask_create_and_init(size_t bit_capacity, size_t word_size) {
   uidq_bitmask_t *bitmask = uidq_bitmask_create();
   if (!bitmask) {
      DEBUG_ERR("Failed to create bitmask\n");
      return NULL;
   }
   if (uidq_bitmask_init(bitmask, bit_capacity, word_size) < 0) {
      DEBUG_ERR("Failed to initialize bitmask\n");
      uidq_bitmask_free(bitmask);
      return NULL;
   }
   return bitmask;
}

/**
 * @brief Copies a bitmask structure.
 * @return 0 on success, -1 on failure.
 */
int uidq_bitmask_copy(uidq_bitmask_t *dest, uidq_bitmask_t *src) {
   if (!is_valid(src)) {
      DEBUG_ERR("Invalid source bitmask\n");
      return -1;
   }
   uidq_bitmask_exit(dest);
   int ret = uidq_bitmask_init(dest, src->bit_capacity, src->word_bits / 8);
   if (ret < 0) {
      DEBUG_ERR("Failed to initialize destination bitmask\n");
      return -1;
   }
   memcpy(dest->mask, src->mask, src->word_capacity * (src->word_bits / 8));
   dest->bit_count = src->bit_count;
   return 0;
}

/**
 * @brief Sets a bit at the specified offset in the bitmask.
 * @return true on success, false on failure.
 */
bool uidq_bitmask_set(uidq_bitmask_t *bitmask, size_t offset) {
   if (!is_valid_index(bitmask, offset)) {
      DEBUG_ERR("Offset out of bounds: %zu >= %zu\n", offset, bitmask->bit_capacity);
      return false;
   }
   size_t word = offset / bitmask->word_bits;
   size_t bit = offset % bitmask->word_bits;
   uint64_t mask = 1ULL << bit;
   if (!(bitmask->mask[word] & mask)) {
      bitmask->mask[word] = bitmask->mask[word] | mask;
      bitmask->bit_count = bitmask->bit_count + 1;
   }
   return true;
}

/**
 * @brief Clears a bit at the specified offset in the bitmask.
 * @return true on success, false on failure.
 */
bool uidq_bitmask_clear(uidq_bitmask_t *bitmask, size_t offset) {
   if (!is_valid_index(bitmask, offset)) {
      DEBUG_ERR("Offset out of bounds: %zu >= %zu\n", offset, bitmask->bit_capacity);
      return false;
   }
   size_t word = offset / bitmask->word_bits;
   size_t bit = offset % bitmask->word_bits;
   uint64_t mask = 1ULL << bit;
   if (bitmask->mask[word] & mask) {
      bitmask->mask[word] = bitmask->mask[word] & ~mask;
      bitmask->bit_count = bitmask->bit_count - 1;
   }
   return true;
}

/**
 * @brief Toggles a bit at the specified offset in the bitmask.
 * @return true on success, false on failure.
 */
bool uidq_bitmask_toggle(uidq_bitmask_t *bitmask, size_t offset, size_t grab) {
   if (!is_valid_index(bitmask, offset) 
         || (offset + grab) > bitmask->bit_capacity) {
      DEBUG_ERR("Offset out of bounds: %zu >= %zu\n", offset, bitmask->bit_capacity);
      return false;
   }
   size_t word = offset / bitmask->word_bits;
   size_t bit = offset % bitmask->word_bits;
   uint64_t mask = ((1ULL << grab) - 1) << bit;

   uint64_t before = bitmask->mask[word];
   int before_count = __builtin_popcountll(before & mask);

   bitmask->mask[word] ^= mask;

   uint64_t after = bitmask->mask[word];
   int after_count = __builtin_popcountll(after & mask);

   bitmask->bit_count += (after_count - before_count);
   return true;
}

/**
 * @brief Toggles a bit at the specified offset in the bitmask and add new.
 * @return true on success, false on failure.
 */
int uidq_bitmask_add_with_toggle(uidq_bitmask_t *bitmask, size_t offset, size_t last_grab, size_t grab) {
   if (!is_valid_index(bitmask, offset) ||
         (offset + last_grab) > bitmask->bit_capacity ||
         (offset + grab) > bitmask->bit_capacity) {
      DEBUG_ERR("Offset out of bounds: %zu >= %zu\n", offset, bitmask->bit_capacity);
      return -1;
   }
   bool toggle = uidq_bitmask_toggle(bitmask, offset, last_grab);
   if (!toggle) {
      DEBUG_ERR("Failed to toggle in bitmask.\n");
      return -1;
   }
   int start_pos = uidq_bitmask_add(bitmask, offset, grab);
   if (start_pos < 0) {
      uidq_bitmask_toggle(bitmask, offset, last_grab);
      DEBUG_ERR("Failed to add into bitmask.\n");
      return -1;
   }

   return start_pos;
}

/**
 * @brief Attempts to toggle bits near offset, then add bits to bitmask.
 *        Falls back to direct add if toggle fails.
 * @return start position on success, -1 on failure.
 */
int uidq_bitmask_add_with_fallback(uidq_bitmask_t *bitmask, size_t offset, size_t last_grab, size_t grab) {
   if (!is_valid_index(bitmask, offset) ||
         (offset + last_grab) > bitmask->bit_capacity ||
         (offset + grab) > bitmask->bit_capacity) {
      DEBUG_ERR("Offset out of bounds: %zu >= %zu\n", offset, bitmask->bit_capacity);
      return -1;
   }
   int start_pos = uidq_bitmask_add_with_toggle(bitmask, offset, last_grab, grab);
   if (start_pos < 0) {
      DEBUG_ERR("Failed to toggle, attempting fallback.\n");
      goto fallback; 
   }
   return start_pos;
fallback:
   start_pos = uidq_bitmask_add(bitmask, offset, grab);
   if (start_pos < 0) {
      DEBUG_ERR("Failed to add, bye bye.\n");
      return -1;
   }
   uidq_bitmask_toggle(bitmask, offset, last_grab);
   return start_pos;
}

/**
 * @brief Tests if a bit is set at the specified offset.
 * @return 1 if the bit is set, 0 if cleared, -1 on error.
 */
int uidq_bitmask_bit_test(const uidq_bitmask_t *bitmask, size_t offset) {
   if (!is_valid_index(bitmask, offset)) {
      DEBUG_ERR("Offset out of bounds: %zu >= %zu\n", offset, bitmask->bit_capacity);
      return -1;
   }
   size_t word = offset / bitmask->word_bits;
   size_t bit = offset % bitmask->word_bits;
   return (bitmask->mask[word] >> bit) & 1;
}

/**
 * @brief Tests if a bit sequence is set at the specified offset.
 * @return offset if the sequence matches the target, -1 on error or mismatch.
 */
int uidq_bitmask_test_sequence(const uidq_bitmask_t *bitmask, int target, size_t offset, size_t grab) {
   if (!is_valid(bitmask) || grab == 0 || offset + grab > bitmask->bit_capacity) {
      DEBUG_ERR("Grab range out of bounds: offset=%zu, grab=%zu, capacity=%zu\n",
            offset, grab, bitmask->bit_capacity);
      return -1;
   }
   size_t index;
   for (index = 0; index < grab; index = index + 1) {
      size_t real_pos = offset + index;
      int set = uidq_bitmask_bit_test(bitmask, real_pos);
      if (set != target) {
         return -1;
      }
   }
   return offset;
}

/**
 * @brief Finds the first set (type=1) or cleared (type=0) bit.
 * @return Index of the first found bit or -1 if none found.
 */
int uidq_bitmask_find_first_bit(const uidq_bitmask_t *bitmask, int type) {
   if (!is_valid(bitmask) || (type != 0 && type != 1)) {
      DEBUG_ERR("Invalid bitmask or type: %d\n", type);
      return -1;
   }
   size_t index;
   for (index = 0; index < bitmask->word_capacity; index = index + 1) {
      uint64_t word = type ? bitmask->mask[index] : ~bitmask->mask[index];
      if (word) {
         size_t pos = index * bitmask->word_bits + __builtin_ctzll(word);
         if (pos >= bitmask->bit_capacity) return -1;
         return pos;
      }
   }
   return -1;
}

/**
 * @brief Finds the first sequence of 'grab' consecutive set (type=1) or cleared (type=0) bits.
 * @return Index of the first bit in the sequence or -1 if none found.
 */
int uidq_bitmask_find_grab_bit(const uidq_bitmask_t *bitmask, size_t offset, size_t grab, int type) {
   if (!is_valid(bitmask) || (type != 0 && type != 1) || grab == 0 || offset + grab > bitmask->bit_capacity) {
      DEBUG_ERR("Invalid: offset=%zu, grab=%zu, type=%d\n", offset, grab, type);
      return -1;
   }
   size_t count = 0;
   int start = -1;
   size_t index;
   for (index = offset; index < bitmask->bit_capacity; index = index + 1) {
      if (uidq_bitmask_bit_test(bitmask, index) == type) {
         if (count == 0) start = index;
         count = count + 1;
         if (count == grab) return start;
      } else {
         count = 0;
         start = -1;
      }
   }
   return -1;
}

/**
 * @brief Sets 'count' consecutive bits starting from the first available cleared bit.
 * @return Index of the first set bit or -1 if none found.
 */
int uidq_bitmask_add(uidq_bitmask_t *bitmask, size_t offset, size_t grab) {
   if (!is_valid(bitmask) || grab == 0 || grab > bitmask->bit_capacity) {
      DEBUG_ERR("Invalid: grab=%zu\n", grab);
      return -1;
   }
   int start = offset == 0 ? uidq_bitmask_find_grab_bit(bitmask, 0, grab, 0) :
      uidq_bitmask_find_grab_bit(bitmask, offset, grab, 0);
   if (start < 0 || (size_t)start + grab > bitmask->bit_capacity) {
      DEBUG_ERR("No sufficient free bits\n");
      return -1;
   }
   size_t index;
   for (index = start; index < start + grab; index = index + 1) {
      uidq_bitmask_set(bitmask, index);
   }
   return start;
}

/**
 * @brief Clears 'count' consecutive bits starting from start_pos.
 * @return true on success, false on failure.
 */
bool uidq_bitmask_del(uidq_bitmask_t *bitmask, size_t start_pos, size_t count) {
   if (!is_valid_index(bitmask, start_pos) || count == 0 || start_pos + count > bitmask->bit_capacity) {
      DEBUG_ERR("Invalid: start_pos=%zu, count=%zu\n", start_pos, count);
      return false;
   }
   size_t index;
   for (index = start_pos; index < start_pos + count; index = index + 1) {
      uidq_bitmask_clear(bitmask, index);
   }
   return true;
}

void uidq_bitmask_replace(uidq_bitmask_t *bitmask, size_t start_pos, size_t count, size_t next_pos) {
}

/**
 * @brief Saves bitmask structure into file.
 * @return 0 on success, -1 on failure.
 */
int uidq_bitmask_save(uidq_bitmask_t *bitmask, int file) {
   if (!bitmask || file < 0) return -1;

   if (write(file, &bitmask->bit_capacity, sizeof(bitmask->bit_capacity)) != sizeof(bitmask->bit_capacity)) return -1;
   if (write(file, &bitmask->word_capacity, sizeof(bitmask->word_capacity)) != sizeof(bitmask->word_capacity)) return -1;
   if (write(file, &bitmask->bit_count, sizeof(bitmask->bit_count)) != sizeof(bitmask->bit_count)) return -1;
   if (write(file, &bitmask->word_bits, sizeof(bitmask->word_bits)) != sizeof(bitmask->word_bits)) return -1;

   ssize_t data_size = (bitmask->word_bits / 8) * bitmask->word_capacity;
   if (write(file, bitmask->mask, data_size) != data_size) return -1;

   return 0;
}

/**
 * @brief Loads bitmask structure into structure.
 * @return 1 on success, -1 on failure.
 */
int uidq_bitmask_load(uidq_bitmask_t *bitmask, int file) {
   if (!bitmask || file < 0) return -1;

   lseek(file, 0, SEEK_SET);

   if (read(file, &bitmask->bit_capacity, sizeof(bitmask->bit_capacity)) != sizeof(bitmask->bit_capacity)) return -1;
   if (read(file, &bitmask->word_capacity, sizeof(bitmask->word_capacity)) != sizeof(bitmask->word_capacity)) return -1;
   if (read(file, &bitmask->bit_count, sizeof(bitmask->bit_count)) != sizeof(bitmask->bit_count)) return -1;
   if (read(file, &bitmask->word_bits, sizeof(bitmask->word_bits)) != sizeof(bitmask->word_bits)) return -1;

   ssize_t data_size = (bitmask->word_bits / 8) * bitmask->word_capacity;
   if (read(file, bitmask->mask, data_size) != data_size) return -1;

   return 0;
}

/**
 * @brief Inverts all bits in the bitmask.
 */
void uidq_bitmask_invert(uidq_bitmask_t *bitmask) {
   if (!is_valid(bitmask)) return;
   size_t index;
   for (index = 0; index < bitmask->word_capacity; index = index + 1) {
      bitmask->mask[index] = ~bitmask->mask[index];
   }
   bitmask->bit_count = bitmask->bit_capacity - bitmask->bit_count;
}

/**
 * @brief Performs a bitwise operation (AND, OR, XOR, etc.) on two source bitmasks and stores the result in dest.
 * @return 0 on success, -1 on failure.
 */
int uidq_bitmask_op(uidq_bitmask_t *dest, uidq_bitmask_t *src1, uidq_bitmask_t *src2, uidq_bitmask_op_t opt) {
   if (!is_valid(dest) || !is_valid(src1) || !is_valid(src2) ||
         dest->bit_capacity != src1->bit_capacity || src1->bit_capacity != src2->bit_capacity) {
      DEBUG_ERR("Invalid bitmasks or mismatched capacities\n");
      return -1;
   }
   size_t count = 0;
   size_t index;
   for (index = 0; index < dest->word_capacity; index = index + 1) {
      switch (opt) {
         case UIDQ_BITMASK_OP_AND:
            dest->mask[index] = src1->mask[index] & src2->mask[index];
            break;
         case UIDQ_BITMASK_OP_OR:
            dest->mask[index] = src1->mask[index] | src2->mask[index];
            break;
         case UIDQ_BITMASK_OP_XOR:
            dest->mask[index] = src1->mask[index] ^ src2->mask[index];
            break;
         case UIDQ_BITMASK_OP_NAND:
            dest->mask[index] = ~(src1->mask[index] & src2->mask[index]);
            break;
         case UIDQ_BITMASK_OP_NOR:
            dest->mask[index] = ~(src1->mask[index] | src2->mask[index]);
            break;
         case UIDQ_BITMASK_OP_XNOR:
            dest->mask[index] = ~(src1->mask[index] ^ src2->mask[index]);
            break;
         default:
            DEBUG_ERR("Invalid operation\n");
            return -1;
      }
      count = count + __builtin_popcountll(dest->mask[index]);
   }
   dest->bit_count = count;
   return 0;
}
