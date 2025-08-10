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

#define UIDQ_BITMASK_DEBUG UIDQ_DEBUG_ENABLED
#if UIDQ_BITMASK_DEBUG == UIDQ_DEBUG_DISABLED
#define DEBUG_INFO(fmt, ...) do {} while(0)
#define DEBUG_ERR(fmt, ...)  do {} while(0)
#endif

struct uidq_bitmask_header_to_write {
   size_t bit_capacity;
   size_t word_capacity;
   size_t bit_count;
   size_t word_bits;
};

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
   for (size_t index = 0; index < bitmask->bit_capacity; index++) {
      printf("%d", uidq_bitmask_bit_test(bitmask, index));
      if ((index + 1) % 64 == 0) printf("\n");
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
 * @brief Resets the entire bitmask, clearing all bits and resetting the bit count to zero.
 * @param bitmask Pointer to the bitmask structure.
 */
void uidq_bitmask_reset(uidq_bitmask_t *bitmask) {
   if (!bitmask) return;
   size_t words = (bitmask->bit_capacity + bitmask->word_bits - 1) / bitmask->word_bits;
   for (size_t index = 0; index < words; index++) {
      bitmask->mask[index] = 0;
   }
   bitmask->bit_count = 0;
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
        return -1;
    }

    size_t word_bits = bitmask->word_bits;
    size_t start_word = offset / word_bits;
    size_t start_bit = offset % word_bits;

    size_t count = 0;
    size_t start = (size_t)-1;

    for (size_t w = start_word; w < bitmask->word_capacity; w++) {
        uint64_t word = bitmask->mask[w];
        if (type == 0) word = ~word;
        if (w == start_word && start_bit > 0) {
            word &= (~0ULL << start_bit);
        }
        while (word) {
            int bit_pos = __builtin_ctzll(word);
            if (count == 0) start = w * word_bits + bit_pos;
            int run = 0;
            while (bit_pos + run < (int)word_bits && (word & (1ULL << (bit_pos + run)))) {
                run++;
            }
            count += run;
            if (count >= grab) return start;
            word &= ~(((1ULL << run) - 1) << bit_pos);
            count = 0;
        }
        if (word != ~0ULL) count = 0;
    }
    return -1;
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

   bitmask->bit_count = bitmask->bit_count + (after_count - before_count);
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
 * @brief Sets force 'count' consecutive bits starting bit.
 * @return Index of the first set bit or -1 if none found.
 */
int uidq_bitmask_add_force(uidq_bitmask_t *bitmask, int pos, size_t offset) {
   pos = pos <= 0 ? 0 : pos;
   if (!is_valid(bitmask) || (size_t)pos >= bitmask->bit_capacity || offset == 0 || pos + offset >= bitmask->bit_capacity) {
      DEBUG_ERR("Invalid: grab=%zu\n", offset);
      return -1;
   }
   size_t index;
   for (index = pos; index < pos + offset; index = index + 1) {
      uidq_bitmask_set(bitmask, index);
   }
   return pos;
}

/**
 * @brief Sets 'count' consecutive bits starting from the first available cleared bit.
 * @return Index of the first set bit or -1 if none found.
 */
int uidq_bitmask_add(uidq_bitmask_t *bitmask, int pos, size_t offset) {
   pos = pos <= 0 ? 0 : pos;
   if (!is_valid(bitmask) || (size_t)pos >= bitmask->bit_capacity || offset == 0 || pos + offset >= bitmask->bit_capacity) {
      DEBUG_ERR("Invalid: grab=%zu\n", offset);
      return -1;
   }

   int start = uidq_bitmask_find_grab_bit(bitmask, pos, offset, 0);
   if (start < 0 || (size_t)start + offset > bitmask->bit_capacity) {
      DEBUG_ERR("No sufficient free bits\n");
      return -1;
   }
   size_t index;
   for (index = start; index < start + offset; index = index + 1) {
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

/**
 * @brief Replaces 'count' consecutive bits starting from start_pos to next_pos.
 * @return hello my baby.
 */
void uidq_bitmask_replace(uidq_bitmask_t *bitmask, size_t start_pos, size_t offset, size_t next_pos) {
   if (!is_valid(bitmask) || start_pos + offset > bitmask->bit_capacity || next_pos + offset > bitmask->bit_capacity) {
      DEBUG_ERR("Invalid parameters: start_pos=%zu, offset=%zu, next_pos=%zu, bit_capacity=%zu\n",
            start_pos, offset, next_pos, bitmask->bit_capacity);
      return;
   }

   size_t size = bitmask->word_capacity;
   size_t word_bits = bitmask->word_bits;
   DEBUG_FUNC("word_bits: %zu\n", word_bits);

   uint64_t tmp[size];
   memcpy(tmp, bitmask->mask, size * sizeof(uint64_t));

   size_t old_bit_count = 0;
   size_t new_bit_count = 0;

   size_t bits_left = offset;
   size_t current_pos = start_pos;
   while (bits_left > 0) {
      size_t word_index = current_pos / word_bits;
      size_t bit_index = current_pos % word_bits;
      size_t bits_in_word = word_bits - bit_index;
      size_t bits_to_clear = UIDQ_MIN(bits_left, bits_in_word);

      uint64_t mask = (bits_to_clear == word_bits) ? ~0ULL : ((1ULL << bits_to_clear) - 1) << bit_index;
      old_bit_count = old_bit_count + __builtin_popcountll(tmp[word_index] & mask); 
      tmp[word_index] = tmp[word_index] & ~mask;

      bits_left = bits_left - bits_to_clear;
      current_pos = current_pos + bits_to_clear;
   }

   bits_left = offset;
   current_pos = start_pos;
   size_t current_n_pos = next_pos;

   while (bits_left > 0) {
      size_t src_word_index = current_pos / word_bits;
      size_t src_bit_index = current_pos % word_bits;
      size_t dst_word_index = current_n_pos / word_bits;
      size_t dst_bit_index = current_n_pos % word_bits;

      size_t bits_in_src_word = word_bits - src_bit_index;
      size_t bits_in_dst_word = word_bits - dst_bit_index;
      size_t bits_to_copy = UIDQ_MIN(bits_left, UIDQ_MIN(bits_in_src_word, bits_in_dst_word));

      DEBUG_FUNC("index: %zu, next_index: %zu, bits_to_copy: %zu\n",
            src_word_index, dst_word_index, bits_to_copy);
      DEBUG_FUNC("bits_left: %zu, current_pos: %zu, current_next_pos: %zu\n",
            bits_left, current_pos, current_n_pos);

      uint64_t mask = (bits_to_copy == word_bits) ? ~0ULL : ((1ULL << bits_to_copy) - 1);
      uint64_t src_mask = mask << src_bit_index;
      uint64_t dst_mask = mask << dst_bit_index;

      uint64_t extracted = (bitmask->mask[src_word_index] & src_mask) >> src_bit_index;

      new_bit_count = new_bit_count + __builtin_popcountll(tmp[dst_word_index] & dst_mask);

      tmp[dst_word_index] = tmp[dst_word_index] & ~dst_mask;

      tmp[dst_word_index] = tmp[dst_word_index] | (extracted << dst_bit_index);

      // new_bit_count = new_bit_count - __builtin_popcountll(tmp[dst_word_index] & dst_mask);
      new_bit_count = new_bit_count + __builtin_popcountll(extracted << dst_bit_index);

      bits_left = bits_left - bits_to_copy;
      current_pos = current_pos + bits_to_copy;
      current_n_pos = current_n_pos + bits_to_copy;
   }

   bitmask->bit_count = bitmask->bit_count - old_bit_count + new_bit_count;

   memcpy(bitmask->mask, tmp, size * sizeof(uint64_t));
}

/**
 * @brief Saves bitmask structure into file.
 * @return 0 on success, -1 on failure.
 */
int uidq_bitmask_save(uidq_bitmask_t *bitmask, int file) {
   if (!bitmask || file < 0) return -1;

   struct uidq_bitmask_header_to_write header = {
      bitmask->bit_capacity,
      bitmask->word_capacity,
      bitmask->bit_count,
      bitmask->word_bits
   };

   if (write(file, &header, sizeof(header)) != sizeof(header)) {
      DEBUG_ERR("Failed to write header\n");
      return -1;
   }

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

   struct uidq_bitmask_header_to_write header = {
      bitmask->bit_capacity,
      bitmask->word_capacity,
      bitmask->bit_count,
      bitmask->word_bits
   };

   if (read(file, &header, sizeof(header)) != sizeof(header)) {
      DEBUG_ERR("Failed to read header\n");
      return -1;
   }

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
