/* opium_bitmask.c
 * Bitmask management module for the Opium core library. 
 *  
 * This module provides a dynamic bitmask abstraction with support for:
 *  - allocation, reallocation, and cleanup of bitmask objects
 *  - toggling, testing, and resetting bits
 *  - range operations (push, pop, toggle)
 *  - searching for free/used bit ranges
 *  - validation helpers and error logging
 *
 * Key features:
 *  - Backed by an array of 64-bit words (uint64_t)
 *
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/opium_bitmask.h"
#include "core/opium_core.h"
#include "core/opium_alloc.h"
#include "core/opium_log.h"
#include "opium_log.h"

#define OPIUM_BITMASK_WORD_SIZE (sizeof(uint64_t) * 8)

static int opium_bitmask_check_index(opium_bitmask_t *bitmask, size_t index, const char *func_name);
static int opium_bitmask_toggle_bit(opium_bitmask_t *bitmask, size_t index, bool set);
static int opium_bitmask_find_bits(opium_bitmask_t *bitmask, size_t start_index, size_t length, int type);
   
   int
opium_bitmask_isvalid(opium_bitmask_t *bitmask)
{
   return bitmask && bitmask->mask;
}

   int
opium_bitmask_isvalid_index(opium_bitmask_t *bitmask, size_t index)
{
   if (!opium_bitmask_isvalid(bitmask) || index >= bitmask->capacity || bitmask->word_capacity == 0) {
      return OPIUM_RET_ERR;
   }

   return OPIUM_RET_OK;
}

   int 
opium_bitmask_is_empty(opium_bitmask_t *bitmask) 
{ 
   if (!opium_bitmask_isvalid(bitmask)) {
      return OPIUM_RET_ERR;
   }

   return (bitmask->count == 0) ? OPIUM_RET_OK : OPIUM_RET_ERR;
}

int
opium_bitmask_is_full(opium_bitmask_t *bitmask)  
{
   if (!opium_bitmask_isvalid(bitmask)) {
      return OPIUM_RET_ERR;
   }

   return (bitmask->count >= bitmask->capacity) ? OPIUM_RET_OK : OPIUM_RET_ERR;
}

int
opium_bitmask_count(opium_bitmask_t *bitmask)
{
   if (!opium_bitmask_isvalid(bitmask)) {
      return OPIUM_RET_ERR;
   }

   return bitmask->count; 
}

   opium_bitmask_t *
opium_bitmask_create(size_t capacity, opium_log_t *log)
{
   opium_bitmask_t *bitmask;
   size_t word_capacity = (capacity + OPIUM_BITMASK_WORD_SIZE - 1) / OPIUM_BITMASK_WORD_SIZE; 
 
   /* Initialize and create a new bitmask.
    *
    * Allocates and Initializes an opium_bitmask_t structure with given capacity
    * All bits are initially cleared(0), and the count of set bits is zero
    *
    * Bitmask fields:
    *  count         - number of bits currently set
    *  capacity      - total number of bits in the bitmask
    *  word_capacity - number of 64-bit words used to store the bits
    *  mask          - pointer to the allocated memory storing the bits
    *
    */

   if (!log || capacity == 0) {
      return NULL;
   }

   bitmask = opium_calloc(sizeof(opium_bitmask_t), log);
   if (!bitmask) {
      return NULL;
   }

   bitmask->count = 0;
   bitmask->capacity = capacity;
   bitmask->word_capacity = word_capacity;
   bitmask->log = log;

   bitmask->mask = opium_calloc(word_capacity * sizeof(uint64_t), log);
   if (!bitmask->mask) {
      opium_free(bitmask, NULL);
      return NULL;
   }

   return bitmask;
}

   void
opium_bitmask_destroy(opium_bitmask_t *bitmask)
{
   /* Frees and clean up a bitmask.
    * Releases all memory associated with the given bitmask, including the internal
    * mask array and the bitmask structure itself. After calling this function, 
    * the pointer should not be used.
    *
    */

   if (!opium_bitmask_isvalid(bitmask)) {
      return;
   }

   opium_free(bitmask->mask, NULL);
   bitmask->mask = NULL;
   opium_free(bitmask, NULL);
}

   int
opium_bitmask_realloc(opium_bitmask_t *bitmask, size_t new_capacity)
{
   size_t old_wd = bitmask->word_capacity;
   size_t new_wd = (new_capacity + OPIUM_BITMASK_WORD_SIZE - 1) / OPIUM_BITMASK_WORD_SIZE;

   /* Reallocate a bitmask to a new capacity.
    *
    * Resizes the internal storage of the bitmask to hold `new_capacity` bits.
    * Existing bits are preserved up to the minimum of old and new capacities.
    * Bits beyond the new capacity are cleared, and new bits (if expanding) are initialized to 0.
    *
    * Parameters:
    *   bitmask      - pointer to the bitmask to resize
    *   new_capacity - new total number of bits for the bitmask
    *
    */

   if (!opium_bitmask_isvalid(bitmask) || new_capacity == 0) {
      return OPIUM_RET_ERR;
   }

   /* Shrink bitmask: clear bits beyond bitmask new capacity and update count */
   if (new_capacity < bitmask->capacity) {
      size_t old_capacity = bitmask->capacity;
      for (size_t index = new_capacity; index < old_capacity; index++) {
         if (opium_bitmask_toggle_bit(bitmask, index, false) == OPIUM_RET_OK) {
            bitmask->count = bitmask->count - 1;
         }
      }
      bitmask->capacity = new_capacity;
   }

   /* Reallocates mask array to hold new number of words
    * If capacity increased, zero out the new words to ensure bits are clear 
    * and update bitmask structure */
   uint64_t *new_mask = realloc(bitmask->mask, new_wd * sizeof(uint64_t));
   if (!new_mask) {
      return OPIUM_RET_ERR;
   }

   if (new_capacity > bitmask->capacity) {
      memset(new_mask + old_wd, 0, (new_wd - old_wd) * sizeof(uint64_t));
   }

   bitmask->capacity = new_capacity;
   bitmask->word_capacity = new_wd;
   bitmask->mask = new_mask;

   return OPIUM_RET_OK;
}

   int 
opium_bitmask_toggle(opium_bitmask_t *bitmask, size_t pos, size_t len)
{
   /* Toggle a range of bits in the bitmask.
    *
    * This function flips bits in the range [pos, pos + len).
    * The bit count is updated accordingly. If the operation
    * would cause an underflow or overflow of the count, the
    * operation is reverted and an error is logged.
    *
    * Parameters:
    *   pos     - starting bit index
    *   len     - number of bits to toggle
    *
    */

   if (opium_bitmask_isvalid_index(bitmask, pos) != OPIUM_RET_OK ||
         (len > 0 && opium_bitmask_isvalid_index(bitmask, pos + len - 1) != OPIUM_RET_OK)) {
      return false;
   }

   if (len == 0 || pos + len > bitmask->capacity) {
      opium_log_err(bitmask->log, "%s: Invalid parameters (off=%zu, len=%zu)\n",
            __func__, pos, len);
      return false;
   }

   size_t word = pos / OPIUM_BITMASK_WORD_SIZE;
   size_t bit = pos % OPIUM_BITMASK_WORD_SIZE;

   uint64_t mask = (len >= OPIUM_BITMASK_WORD_SIZE) ? ~0ULL : ((1ULL << len) - 1);
   mask <<= bit;

   int before = __builtin_popcountll(bitmask->mask[word] & mask);
   bitmask->mask[word] ^= mask;
   int after = __builtin_popcountll(bitmask->mask[word] & mask);

   if (before > after && bitmask->count < (size_t)(before - after)) {
      opium_log_err(bitmask->log, "%s: Bit count underflow\n", __func__);
      bitmask->mask[word] ^= mask;
      return false;
   }

   if (after > before && bitmask->count > bitmask->capacity - (size_t)(after - before)) {
      opium_log_err(bitmask->log, "%s: Bit count overflow\n", __func__);
      bitmask->mask[word] ^= mask;
      return false;
   }

   bitmask->count += after - before;
   return true;
}

   int
opium_bitmask_push(opium_bitmask_t *bitmask, int pos, size_t len)
{
   if (opium_bitmask_isvalid_index(bitmask, pos + len) != OPIUM_RET_OK) {
      return OPIUM_RET_ERR;
   }

   if (len == 0 || (pos >= 0 && (size_t)pos + len > bitmask->capacity)) {
      opium_log_err(bitmask->log, "%s: Invalid parameters (pos=%d, len=%zu)\n", __func__, pos, len);
      return OPIUM_RET_ERR;
   }

   pos = (pos < 0) ? 0 : pos;

   int start = opium_bitmask_find_bits(bitmask, pos, len, 0);
   if (start < 0 || (size_t)start + len > bitmask->capacity) {
      return OPIUM_RET_ERR;
   }

   for (size_t index = start; index < start + len; index++) {
      opium_bitmask_toggle_bit(bitmask, index, true);
   }

   return start;
}

   void 
opium_bitmask_pop(opium_bitmask_t *bitmask, int pos, size_t len)
{
   if (opium_bitmask_isvalid_index(bitmask, pos + len) != OPIUM_RET_OK) {
      return;
   }

   if (len == 0 || (pos >= 0 && (size_t)pos + len > bitmask->capacity)) {
      opium_log_err(bitmask->log, "%s: Invalid parameters (start=%d, len=%zu)\n", __func__, pos, len);
      return;
   }

   if (pos < 0) {
      int index = opium_bitmask_find_bits(bitmask, 0, len, 1);
      if (index >= 0) {
         opium_bitmask_pop(bitmask, index, len);
         return;
      }
      return;
   }

   for (size_t index = pos; index < pos + len; index++) {
      opium_bitmask_toggle_bit(bitmask, index, false);
   }

   return;
}

   void
opium_bitmask_invert(opium_bitmask_t *bitmask)
{
   if (opium_bitmask_isvalid_index(bitmask, 0) != OPIUM_RET_OK) {
      return;
   }
   for (size_t index = 0; index < bitmask->word_capacity; index++) {
      bitmask->mask[index] = ~bitmask->mask[index];
   }
   bitmask->count = bitmask->capacity - bitmask->count;
}

   int
opium_bitmask_copy(opium_bitmask_t **dst, opium_bitmask_t *src)
{
   if (!opium_bitmask_isvalid_index(src, 0) || !dst || !src->log) {
      return OPIUM_RET_ERR;
   }

   opium_bitmask_destroy(*dst);
   *dst = opium_bitmask_create(src->capacity, src->log);

   if (!*dst) {
      return OPIUM_RET_ERR;
   }

   memcpy((*dst)->mask, src->mask, src->word_capacity * sizeof(uint64_t));
   (*dst)->count = src->count;
   return OPIUM_RET_OK;
}

   void
opium_bitmask_reset(opium_bitmask_t *bitmask)
{
   if (opium_bitmask_isvalid_index(bitmask, 0) != OPIUM_RET_OK) {
      return;
   }

   if (bitmask->mask) {
      memset(bitmask->mask, 0, bitmask->word_capacity * sizeof(uint64_t));
      bitmask->count = 0;
   }
}

   int
opium_bitmask_bit_test(opium_bitmask_t *bitmask, size_t off)
{
   if (opium_bitmask_isvalid_index(bitmask, off) != OPIUM_RET_OK) {
      return OPIUM_RET_ERR;
   }

   return (bitmask->mask[off / OPIUM_BITMASK_WORD_SIZE] >> (off % OPIUM_BITMASK_WORD_SIZE)) & 1;
}

   static int
opium_bitmask_check_index(opium_bitmask_t *bitmask, size_t index, const char *func_name)
{
   /* Check if a given bit index is valid for the bitmask.
    *
    * This function verifies that the bitmask is valid and that
    * the provided index is within the allowed range.
    *
    * Parameters:
    *   index     - bit index to check
    *   func_name - name of the calling function (for logging)
    *
    */

   if (opium_bitmask_isvalid_index(bitmask, index) != OPIUM_RET_OK) {
      if (opium_bitmask_isvalid(bitmask)) {
         opium_log_err(bitmask->log, "%s: Invalid index %zu\n", func_name, index);
      }
      return OPIUM_RET_ERR;
   }

   return OPIUM_RET_OK;
}

   static int
opium_bitmask_toggle_bit(opium_bitmask_t *bitmask, size_t index, bool set)
{
   /* Toggle a single bit in the bitmask.
    *
    * This function sets or clears the bit at the specified index.
    * It also updates the count of bits currently set in the bitmask.
    *
    * Parameters:
    *   index   - index of the bit to toggle
    *   set     - if true, set the bit; if false, clear the bit
    *
    */

   if (opium_bitmask_check_index(bitmask, index, __func__) != OPIUM_RET_OK) {
      return OPIUM_RET_ERR;
   }

   size_t word_index = index / OPIUM_BITMASK_WORD_SIZE;
   uint64_t mask = 1ULL << (index % OPIUM_BITMASK_WORD_SIZE);

   if (set) {
      if (!(bitmask->mask[word_index] & mask)) {
         bitmask->mask[word_index] |= mask;
         bitmask->count = bitmask->count + 1;
      }
   } else {
      if (bitmask->mask[word_index] & mask)  {
         bitmask->mask[word_index] &= ~mask;
         bitmask->count = bitmask->count - 1;
      }
   }

   return OPIUM_RET_OK;
}

   static int
opium_bitmask_find_bits(opium_bitmask_t *bitmask, size_t start_index, size_t length, int type)
{
   /**
    * Find a sequence of bits in a bitmask.
    *
    * Searches for `length` consecutive bits of the specified `type` (0 = unset, 1 = set)
    * starting from `start_index`. Returns the index of the first bit of the found sequence,
    * or OPIUM_RET_ERR if no such sequence exists.
    *
    * Parameters:
    *   start_index - starting bit index for the search
    *   length      - number of consecutive bits to find
    *   type        - 0 to search for unset bits, 1 to search for set bits
    *
    * Returns:
    *   Index of the first bit of the found sequence, or OPIUM_RET_ERR on failure.
    */

   if (opium_bitmask_check_index(bitmask, 0, __func__) != OPIUM_RET_OK) {
      return OPIUM_RET_ERR;
   }

   if (length == 0 || start_index + length > bitmask->capacity || (type != 0 && type != 1)) {
      opium_log_err(bitmask->log, "%s: Invalid parameters (start_index=%zu, length=%zu, type=%d)\n",
            __func__, start_index, length, type);
      return OPIUM_RET_ERR;
   }

   // Special case: searching for a single bit
   if (length == 1) {
      for (size_t wi = start_index / OPIUM_BITMASK_WORD_SIZE; wi < bitmask->word_capacity; wi++) {
         uint64_t word = type ? bitmask->mask[wi] : ~bitmask->mask[wi];
         if (word) {
            // Find first set/unset bit in this word
            size_t bit_pos = wi * OPIUM_BITMASK_WORD_SIZE + __builtin_ctzll(word);
            if (bit_pos >= bitmask->capacity) {
               return OPIUM_RET_ERR;
            }
            return bit_pos;
         }
      }
      return OPIUM_RET_ERR;
   }

   // Search for multiple consecutive bits
   size_t found_start = 0;   // Start index of current candidate sequence
   size_t found_count = 0;   // Number of consecutive bits found

   for (size_t wi = start_index / OPIUM_BITMASK_WORD_SIZE; wi < bitmask->word_capacity; wi++) {
      // Get the current word, invert if searching for 0 bits
      uint64_t word = type ? bitmask->mask[wi] : ~bitmask->mask[wi];

      // Mask out bits before start_index in the first word
      if (wi == start_index / OPIUM_BITMASK_WORD_SIZE && start_index % OPIUM_BITMASK_WORD_SIZE) {
         word = word & (~0ULL << (start_index % OPIUM_BITMASK_WORD_SIZE));
      }

      // Check each bit in the current word
      for (size_t bit_index = 0; bit_index < OPIUM_BITMASK_WORD_SIZE; bit_index++) {
         if (word & (1ULL << bit_index)) {
            // Bit matches search type
            if (found_count == 0) {
               found_start = wi * OPIUM_BITMASK_WORD_SIZE + bit_index;
            }
            found_count = found_count + 1;

            // Found enough consecutive bits
            if (found_count >= length) {
               return found_start;
            }
         } else {
            // Sequence broken, reset counter
            found_count = 0;
         }
      }
   }

   return OPIUM_RET_ERR;
}

   void
opium_bitmask_debug(opium_bitmask_t *bitmask)
{
   if (opium_bitmask_isvalid_index(bitmask, 0) != OPIUM_RET_OK) {
      return;
   }

   for (size_t i = 0; i < bitmask->capacity; i++) {
      opium_log_debug_inline(bitmask->log, "%c", opium_bitmask_bit_test(bitmask, i) ? '1' : '0');
      if ((i + 1) % OPIUM_BITMASK_WORD_SIZE == 0) {
         opium_log_debug_inline(bitmask->log, "\n");
      }
   }

   opium_log_debug_inline(bitmask->log, "\n");
   opium_log_debug(bitmask->log, "Bitmask busy: %zu\n", bitmask->count);
}
