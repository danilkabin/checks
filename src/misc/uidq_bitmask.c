#include "uidq_bitmask.h"
#include "uidq_utils.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define UIDQ_BITMASK_DEBUG UIDQ_DEBUG_DISABLED
#if UIDQ_BITMASK_DEBUG == UIDQ_DEBUG_DISABLED
#define DEBUG_INFO(fmt, ...) do {} while(0)
#define DEBUG_ERR(fmt, ...)  do {} while(0)
#endif

/**
 * @brief Macro to check if type is valid (0 or 1).
 */
#define UIDQ_BITMASK_CHECK_TYPE(t) \
   do { \
      if ((t) != 0 && (t) != 1) { \
         DEBUG_ERR("Type must be 0 (find zeros) or 1 (find ones)\n"); \
         return -1; \
      } \
   } while (0)

/**
 * @brief Sets a bit at the specified offset in the bitmask.
 */
void uidq_bitmask_set(uidq_bitmask_t *bitmask, size_t offset) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return);
   if (offset >= bitmask->bit_capacity) {
      DEBUG_ERR("Offset out of bounds: %zu >= %zu\n", offset, bitmask->bit_capacity);
      return;
   }
   if (!uidq_bitmask_bit_test(bitmask, offset)) {
      bitmask->mask[offset / bitmask->word_bits] |= (1ULL << (offset % bitmask->word_bits));
      bitmask->bit_count++;
   }
}

/**
 * @brief Clears a bit at the specified offset in the bitmask.
 */
void uidq_bitmask_clear(uidq_bitmask_t *bitmask, size_t offset) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return);
   if (offset >= bitmask->bit_capacity) {
      DEBUG_ERR("Offset out of bounds: %zu >= %zu\n", offset, bitmask->bit_capacity);
      return;
   }
   if (uidq_bitmask_bit_test(bitmask, offset)) {
      bitmask->mask[offset / bitmask->word_bits] &= ~(1ULL << (offset % bitmask->word_bits));
      bitmask->bit_count--;
   }
}

/**
 * @brief Tests if a bit is set at the specified offset.
 * @return 1 if the bit is set, 0 if cleared, -1 on error.
 */
int uidq_bitmask_bit_test(uidq_bitmask_t *bitmask, size_t offset) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return -1);
   if (offset >= bitmask->bit_capacity) {
      DEBUG_ERR("Offset out of bounds: %zu >= %zu\n", offset, bitmask->bit_capacity);
      return -1;
   }
   return (bitmask->mask[offset / bitmask->word_bits] >> (offset % bitmask->word_bits)) & 1;
}

/**
 * @brief Tests if a bit sequence is set at the specified offset.
 * @return 1 if the bit is set, 0 if cleared, -1 on error.
 */
int uidq_bitmask_test_sequence(uidq_bitmask_t *bitmask, int target, size_t offset, size_t grab) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return -1);
   if (grab == 0 || offset + grab > bitmask->bit_capacity) {
      DEBUG_ERR("Grab range out of bounds: offset=%zu, grab=%zu, capacity=%zu\n",
            offset, grab, bitmask->bit_capacity);
      return -1;
   }
   for (size_t index = 0; index < grab; index++) {
      size_t real_pos = offset + index;
      int set = (bitmask->mask[real_pos / bitmask->word_bits] >> (real_pos % bitmask->word_bits)) & 1;
      if (set != target) {
         return -1;
      }
   }
   return 0;
}

/**
 * @brief Finds the first set (type=1) or cleared (type=0) bit starting from offset.
 * @return Index of the first found bit or -1 if none found.
 */
int uidq_bitmask_ffb(uidq_bitmask_t *bitmask, int offset, int type) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return -1);
   UIDQ_BITMASK_CHECK_TYPE(type);

   if (offset < 0 || (size_t)offset >= bitmask->bit_capacity) {
      DEBUG_ERR("Invalid offset\n");
      return -1;
   }

   size_t word_bits = bitmask->word_bits;
   size_t frame_count = bitmask->word_capacity;
   size_t frame_offset = offset / word_bits;
   size_t bit_offset = offset % word_bits;

   for (size_t index = frame_offset; index < frame_count; index++) {
      uint64_t mask = bitmask->mask[index];
      if ((type == 0 && mask == UINT64_MAX) || (type == 1 && mask == 0)) {
         continue;
      }
      mask = type == 0 ? ~mask : mask;

      if (index == frame_offset) {
         mask &= ~((1ULL << bit_offset) - 1);
      }

      if (mask) {
         int bit_pos = __builtin_ctzll(mask);
         size_t new_pos = index * word_bits + bit_pos;
         if (new_pos >= bitmask->bit_capacity) {
            DEBUG_ERR("Pos exceeds bit capacity: %zu >= %zu\n", new_pos, bitmask->bit_capacity);
            return -1;
         }
         return (int)new_pos;
      }
   }

   return -1;
}

/**
 * @brief Finds the first sequence of 'grab' consecutive set (type=1) or cleared (type=0) bits.
 * @return Index of the first bit in the sequence or -1 if none found.
 */
int uidq_bitmask_ffgb(uidq_bitmask_t *bitmask, int grab, int type) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return -1);
   UIDQ_BITMASK_CHECK_TYPE(type);

   if (grab < 0 || (size_t)grab > bitmask->bit_capacity) {
      DEBUG_ERR("Grab is invalid\n");
      return -1;
   }

   size_t offset = 0;
   int combo = 0;
   int last_pos = -2;
   int start_pos = -1;

   while ((start_pos = uidq_bitmask_ffb(bitmask, offset, type)) >= 0) {
      offset = start_pos + 1;
      if (start_pos - last_pos > 1) {
         combo = 0;
      }
      combo = combo + 1;
      last_pos = start_pos;
      if (combo >= grab) {
         return start_pos - grab + 1;
      }
   }
   return -1;
}

/**
 * @brief Finds the first set (type=1) or cleared (type=0) bit.
 * @return Index of the first found bit or -1 if none found.
 */
int uidq_bitmask_find_first_bit(uidq_bitmask_t *bitmask, int type) {
   return uidq_bitmask_ffb(bitmask, 0, type);
}

/**
 * @brief Finds the first sequence of 'grab' consecutive set (type=1) or cleared (type=0) bits.
 * @return Index of the first bit in the sequence or -1 if none found.
 */
int uidq_bitmask_find_grab_bit(uidq_bitmask_t *bitmask, int grab, int type) {
   return uidq_bitmask_ffgb(bitmask, grab, type);
}

/**
 * @brief Sets 'count' consecutive bits starting from the first available cleared bit.
 * @return Index of the first set bit or -1 if none found.
 */
int uidq_bitmask_add(uidq_bitmask_t *bitmask, size_t offset, size_t grab) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return -1);
   if (grab == 0 || grab > bitmask->bit_capacity) {
      DEBUG_ERR("Invalid count\n");
      return -1;
   }

   int start_pos = offset <= 0 ? uidq_bitmask_find_grab_bit(bitmask, grab, 0) : uidq_bitmask_test_sequence(bitmask, 0, offset, grab);
   if (start_pos < 0 || (size_t)start_pos + grab > bitmask->bit_capacity) {
      DEBUG_ERR("No sufficient free bits\n");
      return -1;
   }

   for (size_t index = 0; index < grab; index++) {
      uidq_bitmask_set(bitmask, start_pos + index);
   }

   return start_pos;
}

/**
 * @brief Clears 'count' consecutive bits starting from start_pos.
 */
void uidq_bitmask_del(uidq_bitmask_t *bitmask, int start_pos, size_t count) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return);
   if (start_pos < 0 || (size_t)start_pos + count > bitmask->bit_capacity) {
      DEBUG_ERR("Invalid start_pos or count\n");
      return;
   }

   for (size_t index = 0; index < count; index++) {
      uidq_bitmask_clear(bitmask, start_pos + index);
   }
}

/**
 * @brief Prints the bitmask for debugging purposes.
 */
void uidq_bitmask_debug(uidq_bitmask_t *bitmask) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return);
   for (size_t index = 0; index < bitmask->word_capacity; index++) {
      for (size_t bit = 0; bit < bitmask->word_bits; bit++) {
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
 * @brief Saves bitmask structure into file.
 * @return 0 on success, -1 on failure.
 */
int uidq_bitmask_save(uidq_bitmask_t *bitmask, int file) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return -1);

   if (file < 0) {
      DEBUG_ERR("File is invalid.\n");
      return -1;
   }

   char header[128];
   ssize_t count = snprintf(header, sizeof(header), 
         "Bitmask. Word capacity: %zu, Bit capacity: %zu. Bits: ", 
         bitmask->word_capacity, bitmask->bit_capacity);

   ssize_t total_written = 0;
   while (total_written < count) {
      ssize_t written = write(file, header + total_written, count - total_written);
      if (written < 0) {
         close(file);
         return -1;
      }
      total_written += written;
   }

   ssize_t mask_bytes = bitmask->word_capacity * sizeof(uint64_t);
   total_written = 0;
   while (total_written < mask_bytes) {
      ssize_t written = write(file, (char *)bitmask->mask + total_written, mask_bytes - total_written);
      if (written < 0) {
         close(file);
         return -1;
      }
      total_written += written;
   }

   close(file);
   return 0;
}

/**
 * @brief Checks the bitmask on empty.
 */
int uidq_bitmask_is_empty(uidq_bitmask_t *bitmask) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return -1);
   return bitmask->bit_count == 0 ? 0 : -1; 
}

/**
 * @brief Inverts all bits in the bitmask.
 */
void uidq_bitmask_invert(uidq_bitmask_t *bitmask) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return);
   for (size_t index = 0; index < bitmask->word_capacity; index++) {
      bitmask->mask[index] = ~bitmask->mask[index];
   }
   size_t free_bits = bitmask->bit_capacity - bitmask->bit_count;
   bitmask->bit_count = free_bits; 
}

/**
 * @brief Performs a bitwise operation (AND, OR, XOR, etc.) on two source bitmasks and stores the result in dest.
 * @return 0 on success, -1 on failure.
 */
int uidq_bitmask_op(uidq_bitmask_t *dest, uidq_bitmask_t *src1, uidq_bitmask_t *src2, uidq_bitmask_op_t opt) {
   UIDQ_STRUCT_CHECK_INIT(dest, return -1);
   UIDQ_STRUCT_CHECK_INIT(src1, return -1);
   UIDQ_STRUCT_CHECK_INIT(src2, return -1);

   if (dest->bit_capacity != src1->bit_capacity || src1->bit_capacity != src2->bit_capacity) {
      DEBUG_ERR("Bit capacity is not correct.\n");
      return -1;
   }

   size_t count = 0;
   size_t word_capacity = dest->word_capacity;

   for (size_t index = 0; index < word_capacity; index++) {
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
            DEBUG_ERR("Invalid operation.\n");
            return -1;
      }

      uint64_t mask = dest->mask[index];
      int index_count = __builtin_popcountll(mask);
      count += index_count;
   }

   dest->bit_count = count;
   return 0;
}

/**
 * @brief frees all bits in bitmask structure.
 */
void uidq_bitmask_bits_clear(uidq_bitmask_t *bitmask) {
   UIDQ_STRUCT_CHECK_INIT(bitmask, return);
   memset(bitmask->mask, 0, bitmask->word_capacity * (bitmask->word_bits / 8));
   bitmask->bit_count = 0;
}

/**
 * @brief Initializes a bitmask structure.
 * @return 0 on success, -1 on failure.
 */
int uidq_bitmask_init(uidq_bitmask_t *bitmask, size_t bit_capacity, size_t word_size) {
   if (!bitmask || bit_capacity == 0 || word_size != sizeof(uint64_t)) {
      DEBUG_ERR("Invalid parameters: bit_capacity=%zu, word_size=%zu\n", bit_capacity, word_size);
      return -1;
   }

   if (bitmask->mask) {
      free(bitmask->mask);
      bitmask->mask = NULL;
   }

   bitmask->initialized   = false;
   bitmask->word_bits     = word_size * 8; 
   bitmask->word_capacity = (bit_capacity + bitmask->word_bits - 1) / bitmask->word_bits;
   bitmask->bit_capacity  = bit_capacity;
   bitmask->bit_count     = 0;

   bitmask->mask = calloc(bitmask->word_capacity, word_size);
   if (!bitmask->mask) {
      DEBUG_ERR("Failed to allocate memory for bitmask\n");
      return -1;
   }

   bitmask->initialized = true;
   return 0;
}

/**
 * @brief Frees resources associated with the bitmask.
 */
void uidq_bitmask_exit(uidq_bitmask_t *bitmask) {
   if (!bitmask || !bitmask->initialized) {
      return;
   }

   free(bitmask->mask);
   bitmask->mask          = NULL;
   bitmask->bit_capacity  = 0;
   bitmask->word_bits     = 0;
   bitmask->word_capacity = 0;
   bitmask->bit_count     = 0;
   bitmask->initialized   = false;
}

/**
 * @brief Copies a bitmask structure.
 * @return 0 on success, -1 on failure.
 */
int uidq_bitmask_copy(uidq_bitmask_t *dest, uidq_bitmask_t *src) {
   UIDQ_STRUCT_CHECK_INIT(src, return -1);
   size_t word_size = src->word_bits / 8;
   int ret = uidq_bitmask_init(dest, src->bit_capacity, word_size);
   if (ret < 0) {
      DEBUG_ERR("Failed to initialize destination bitmask\n");
      return -1;
   }
   memcpy(dest->mask, src->mask, src->word_capacity * word_size);
   dest->bit_count = src->bit_count;
   return 0;
}

/**
 * @brief Creates a new bitmask structure.
 * @return Pointer to the new bitmask or NULL on failure.
 */
uidq_bitmask_t *uidq_bitmask_create(void) {
   uidq_bitmask_t *bitmask = calloc(1, sizeof(uidq_bitmask_t));
   if (!bitmask) {
      DEBUG_ERR("Failed to allocate uidq_bitmask structure\n");
      return NULL;
   }
   bitmask->instance = true;
   return bitmask;
}

/**
 * @brief Frees a bitmask structure and its resources.
 */
void uidq_bitmask_free(uidq_bitmask_t *bitmask) {
   if (!bitmask) {
      return;
   }
   uidq_bitmask_exit(bitmask);

   UIDQ_STRUCT_CHECK_INSTANCE(bitmask, return);   
   free(bitmask);
}

/**
 * @brief Creates and Initializes bitmask structure.
 */
uidq_bitmask_t *uidq_bitmask_create_and_init(size_t bit_capacity, size_t word_size) {
   uidq_bitmask_t *bitmask = uidq_bitmask_create();
   if (!bitmask) {
      DEBUG_ERR("Failed to create bitmask structure.\n");
      return NULL;
   }

   int ret = uidq_bitmask_init(bitmask, bit_capacity, word_size);
   if (ret < 0) {
      DEBUG_ERR("Failed to initialize bitmask structure.\n");
      uidq_bitmask_free(bitmask);   
      return NULL;
   }
   return bitmask;
}
