#include "core/opium_bitmask.h"
#include "core/opium_alloc.h"
#include "core/opium_log.h"
#include "core/opium_core.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPIUM_BITMASK_WORD_SIZE (sizeof(uint64_t) * 8)

static int opium_bitmask_set(opium_bitmask_t *bitmask, size_t off);
static int opium_bitmask_clear(opium_bitmask_t *bitmask, size_t off);
static void opium_bitmask_trim(opium_bitmask_t *bitmask, size_t new_capacity);

/* Validity Checks */
   int
opium_bitmask_isvalid(opium_bitmask_t *bitmask)
{
   return bitmask && bitmask->initialized == 1;
}

   int
opium_bitmask_isvalid_index(opium_bitmask_t *bitmask, size_t index)
{
   if (!opium_bitmask_isvalid(bitmask) || !bitmask->mask || index >= bitmask->capacity || bitmask->word_capacity == 0) {
      return OPIUM_RET_ERR;
   }
   return OPIUM_RET_OK;
}

/* State Checks */
   int
opium_bitmask_is_empty(opium_bitmask_t *bitmask)
{
   if (!opium_bitmask_isvalid(bitmask)) {
      return OPIUM_RET_ERR;
   }

   return bitmask->count == 0 ? OPIUM_RET_OK : OPIUM_RET_ERR;
}

   int
opium_bitmask_is_full(opium_bitmask_t *bitmask)
{
   if (!opium_bitmask_isvalid(bitmask)) {
      return OPIUM_RET_ERR;
   }

   return bitmask->count >= bitmask->capacity ? OPIUM_RET_OK : OPIUM_RET_ERR;
}

   size_t
opium_bitmask_get_capacity(opium_bitmask_t *bitmask)
{
   if (!opium_bitmask_isvalid(bitmask)) {
      return 0;
   }

   return bitmask->capacity;
}

   void
opium_bitmask_lim_capacity(opium_bitmask_t *bitmask, size_t new_capacity)
{
   if (!opium_bitmask_isvalid(bitmask)) {
      return;
   }

   if (new_capacity > bitmask->allocated_capacity) {
      opium_err(bitmask->log, "%s: New capacity exceeds allocated capacity\n", __func__);
      return;
   }

   bitmask->capacity = new_capacity;
}

/* Creation and Destruction */
   opium_bitmask_t *
opium_bitmask_create(size_t capacity, opium_log_t *log)
{
   if (log == NULL) {
      return NULL;
   }

   opium_bitmask_t *bitmask = opium_calloc(sizeof(opium_bitmask_t), log);
   if (!bitmask) {
      opium_err(log, "Bitmask allocation failed\n");
      return NULL;
   }
   bitmask->initialized = 1;

   if (opium_bitmask_init(bitmask, capacity, log) != OPIUM_RET_OK) {
      opium_err(log, "Failed to init bitmask struct\n");
      opium_bitmask_abort(bitmask);
      return NULL;
   }

   return bitmask;
}

   void
opium_bitmask_abort(opium_bitmask_t *bitmask)
{
   if (!opium_bitmask_isvalid(bitmask)) {
      return;
   }

   opium_bitmask_exit(bitmask);
   bitmask->initialized = 0;
   opium_free(bitmask, NULL);
}

   int
opium_bitmask_init(opium_bitmask_t *bitmask, size_t capacity, opium_log_t *log)
{
   if (!opium_bitmask_isvalid(bitmask)) {
      return OPIUM_RET_ERR;
   }

   if (capacity == 0 || log == NULL) {
      opium_err(log, "%s: Invalid parameters\n", __func__);
      return OPIUM_RET_ERR;
   }

   bitmask->count = 0;
   bitmask->capacity = capacity;
   bitmask->allocated_capacity = capacity;
   bitmask->word_capacity = (capacity + OPIUM_BITMASK_WORD_SIZE - 1) / OPIUM_BITMASK_WORD_SIZE;
   bitmask->log = log;

   bitmask->mask = opium_calloc(bitmask->word_capacity * sizeof(uint64_t), log);
   if (!bitmask->mask) {
      opium_err(log, "%s: Failed to allocate bitmask data\n", __func__);
      opium_bitmask_exit(bitmask);
      return OPIUM_RET_ERR;
   }

   return OPIUM_RET_OK;
}

   void
opium_bitmask_exit(opium_bitmask_t *bitmask)
{
   if (!opium_bitmask_isvalid(bitmask)) {
      return;
   }

   bitmask->count = 0;
   bitmask->capacity = 0;
   bitmask->allocated_capacity = 0;
   bitmask->word_capacity = 0;
   bitmask->log = NULL;

   if (bitmask->mask) {
      opium_free(bitmask->mask, NULL);
      bitmask->mask = NULL;
   }
}

   int
opium_bitmask_realloc(opium_bitmask_t *bitmask, size_t new_capacity)
{
   if (!opium_bitmask_isvalid(bitmask)) {
      return OPIUM_RET_ERR;
   }

   if (new_capacity == 0) {
      opium_err(bitmask->log, "%s: Invalid capacity\n", __func__);
      return OPIUM_RET_ERR;
   }

   size_t old_capacity = bitmask->allocated_capacity;
   size_t old_wd = bitmask->word_capacity;
   size_t new_wd = (new_capacity + OPIUM_BITMASK_WORD_SIZE - 1) / OPIUM_BITMASK_WORD_SIZE;

   if (new_capacity < old_capacity) {
      opium_bitmask_trim(bitmask, new_capacity);
   }

   uint64_t *new_mask = realloc(bitmask->mask, new_wd * sizeof(uint64_t));
   if (!new_mask) {
      opium_err(bitmask->log, "%s: Failed to reallocate bitmask data\n", __func__);
      return OPIUM_RET_ERR;
   }

   if (new_capacity > old_capacity) {
      memset(new_mask + old_wd, 0, (new_wd - old_wd) * sizeof(uint64_t));
   }

   bitmask->allocated_capacity = new_capacity;
   bitmask->word_capacity = new_wd;
   bitmask->mask = new_mask;

   return OPIUM_RET_OK;
}

/* Memory and State Management */

/*
   void
   opium_bitmask_replace(opium_bitmask_t *bitmask, size_t start, size_t len, size_t next)
   {
   if (!opium_bitmask_isvalid(bitmask)) {
   return;
   }

   if (len == 0 || start + len > bitmask->capacity || next + len > bitmask->capacity) {
   opium_err(bitmask->log, "%s: Invalid parameters (start=%zu, len=%zu, next=%zu)\n",
   __func__, start, len, next);
   return;
   }

   uint64_t *tmp = opium_calloc(bitmask->word_capacity * sizeof(uint64_t), bitmask->log);
   if (!tmp) {
   opium_err(bitmask->log, "%s: Temp allocation failed\n", __func__);
   return;
   }

   size_t old_count = 0, new_count = 0;

   size_t bits_left = len, pos = start;
   while (bits_left > 0) {
   size_t w_idx = pos / OPIUM_BITMASK_WORD_SIZE;
   size_t b_idx = pos % OPIUM_BITMASK_WORD_SIZE;
   size_t bits_in_word = OPIUM_BITMASK_WORD_SIZE - b_idx;
   size_t to_clear = bits_left < bits_in_word ? bits_left : bits_in_word;
   uint64_t mask = (to_clear == OPIUM_BITMASK_WORD_SIZE) ? ~0ULL : ((1ULL << to_clear) - 1) << b_idx;
   old_count += __builtin_popcountll(tmp[w_idx] & mask);
   tmp[w_idx] &= ~mask;
   bits_left -= to_clear;
   pos += to_clear;
   }

   bits_left = len;
   pos = start;
   size_t n_pos = next;
   while (bits_left > 0) {
   size_t sw = pos / OPIUM_BITMASK_WORD_SIZE, sb = pos % OPIUM_BITMASK_WORD_SIZE;
   size_t dw = n_pos / OPIUM_BITMASK_WORD_SIZE, db = n_pos % OPIUM_BITMASK_WORD_SIZE;
   size_t src_left = OPIUM_BITMASK_WORD_SIZE - sb, dst_left = OPIUM_BITMASK_WORD_SIZE - db;
   size_t copy = bits_left < src_left ? (bits_left < dst_left ? bits_left : dst_left) : (src_left < dst_left ? src_left : dst_left);
   uint64_t mask = (copy == OPIUM_BITMASK_WORD_SIZE) ? ~0ULL : ((1ULL << copy) - 1);
   uint64_t src_mask = mask << sb;
   uint64_t dst_mask = mask << db;
   uint64_t extracted = (bitmask->mask[sw] & src_mask) >> sb;
   new_count += __builtin_popcountll(tmp[dw] & dst_mask);
   tmp[dw] &= ~dst_mask;
   tmp[dw] |= (extracted << db);
   new_count += __builtin_popcountll(extracted << db);
   bits_left -= copy;
   pos += copy;
   n_pos += copy;
   }

   bitmask->count = bitmask->count - old_count + new_count;
   memcpy(bitmask->mask, tmp, bitmask->word_capacity * sizeof(uint64_t));
   opium_free(tmp, bitmask->log);
   }
   */

   void
opium_bitmask_invert(opium_bitmask_t *bitmask)
{
   if (opium_bitmask_isvalid_index(bitmask, 0) != OPIUM_RET_OK) {
      return;
   }

   for (size_t i = 0; i < bitmask->word_capacity; i++) {
      bitmask->mask[i] = ~bitmask->mask[i];
   }
   bitmask->count = bitmask->capacity - bitmask->count;
}

   int
opium_bitmask_copy(opium_bitmask_t **dst, opium_bitmask_t *src)
{
   if (opium_bitmask_isvalid_index(src, 0) != OPIUM_RET_OK) {
      return OPIUM_RET_ERR;
   }

   if (dst == NULL || src->log == NULL) {
      opium_err(src->log, "%s: Invalid parameters\n", __func__);
      return OPIUM_RET_ERR;
   }

   opium_bitmask_abort(*dst);

   *dst = opium_bitmask_create(src->allocated_capacity, src->log);
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

/* Bitwise Operations */
   int
opium_bitmask_op(opium_bitmask_t *dst, opium_bitmask_t *src1, opium_bitmask_t *src2, opium_bitmask_op_t op)
{
   if (opium_bitmask_isvalid_index(dst, 0) != OPIUM_RET_OK ||
         opium_bitmask_isvalid_index(src1, 0) != OPIUM_RET_OK ||
         opium_bitmask_isvalid_index(src2, 0) != OPIUM_RET_OK) {
      return OPIUM_RET_ERR;
   }

   if (dst->capacity != src1->capacity || src1->capacity != src2->capacity) {
      opium_err(dst->log, "%s: Mismatched capacities\n", __func__);
      return OPIUM_RET_ERR;
   }

   size_t count = 0;
   for (size_t i = 0; i < dst->word_capacity; i++) {
      switch (op) {
         case OPIUM_BITMASK_OP_AND:  dst->mask[i] = src1->mask[i] & src2->mask[i]; break;
         case OPIUM_BITMASK_OP_OR:   dst->mask[i] = src1->mask[i] | src2->mask[i]; break;
         case OPIUM_BITMASK_OP_XOR:  dst->mask[i] = src1->mask[i] ^ src2->mask[i]; break;
         case OPIUM_BITMASK_OP_NAND: dst->mask[i] = ~(src1->mask[i] & src2->mask[i]); break;
         case OPIUM_BITMASK_OP_NOR:  dst->mask[i] = ~(src1->mask[i] | src2->mask[i]); break;
         case OPIUM_BITMASK_OP_XNOR: dst->mask[i] = ~(src1->mask[i] ^ src2->mask[i]); break;
         default:
                                     opium_err(dst->log, "%s: Invalid bitwise operation\n", __func__);
                                     return OPIUM_RET_ERR;
      }
      count += __builtin_popcountll(dst->mask[i]);
   }

   if (count > dst->capacity) {
      opium_err(dst->log, "%s: Bit count overflow\n", __func__);
      return OPIUM_RET_ERR;
   }

   dst->count = count;
   return OPIUM_RET_OK;
}

/* Bit Operations */
   int
opium_bitmask_bit_test(opium_bitmask_t *bitmask, size_t off)
{
   if (opium_bitmask_isvalid_index(bitmask, off) != OPIUM_RET_OK) {
      if (opium_bitmask_isvalid(bitmask)) {
         //opium_err(bitmask->log, "%s: Invalid offset %zu\n", __func__, off);
      }
      return OPIUM_RET_ERR;
   }

   return (bitmask->mask[off / OPIUM_BITMASK_WORD_SIZE] >> (off % OPIUM_BITMASK_WORD_SIZE)) & 1;
}

   int
opium_bitmask_ffb(opium_bitmask_t *bitmask, int type)
{
   if (opium_bitmask_isvalid_index(bitmask, 0) != OPIUM_RET_OK) {
      return OPIUM_RET_ERR;
   }

   if (type != 0 && type != 1) {
      opium_err(bitmask->log, "%s: Invalid type %d\n", __func__, type);
      return OPIUM_RET_ERR;
   }

   for (size_t i = 0; i < bitmask->word_capacity; i++) {
      uint64_t word = type ? bitmask->mask[i] : ~bitmask->mask[i];
      if (word) {
         size_t pos = i * OPIUM_BITMASK_WORD_SIZE + __builtin_ctzll(word);
         if (pos >= bitmask->capacity) {
            return OPIUM_RET_ERR;
         }
         return pos;
      }
   }

   return OPIUM_RET_ERR;
}

   int
opium_bitmask_fgb(opium_bitmask_t *bitmask, size_t off, size_t len, int type)
{
   if (opium_bitmask_isvalid_index(bitmask, 0) != OPIUM_RET_OK) {
      return OPIUM_RET_ERR;
   }

   if (len == 0 || off + len > bitmask->capacity || (type != 0 && type != 1)) {
      opium_err(bitmask->log, "%s: Invalid parameters (off=%zu, len=%zu, type=%d)\n",
            __func__, off, len, type);
      return OPIUM_RET_ERR;
   }

   size_t start = 0;
   size_t count = 0;
   for (size_t w = off / OPIUM_BITMASK_WORD_SIZE; w < bitmask->word_capacity; w++) {
      uint64_t word = type ? bitmask->mask[w] : ~bitmask->mask[w];

      if (w == off / OPIUM_BITMASK_WORD_SIZE && off % OPIUM_BITMASK_WORD_SIZE) {
         word &= (~0ULL << (off % OPIUM_BITMASK_WORD_SIZE));
      }

      size_t bit_pos = 0;
      while (bit_pos < OPIUM_BITMASK_WORD_SIZE) {
         if (word & (1ULL << bit_pos)) {
            if (count == 0) {
               start = w * OPIUM_BITMASK_WORD_SIZE + bit_pos;
            }
            count++;
            if (count >= len) {
               return start;
            }
         } else {
            count = 0;
         }
         bit_pos++;
      }
   }

   return OPIUM_RET_ERR;
}

   bool
opium_bitmask_toggle(opium_bitmask_t *bitmask, size_t pos, size_t len)
{
   if (opium_bitmask_isvalid_index(bitmask, pos) != OPIUM_RET_OK ||
         (len > 0 && opium_bitmask_isvalid_index(bitmask, pos + len - 1) != OPIUM_RET_OK)) {
      return false;
   }

   if (len == 0 || pos + len > bitmask->capacity) {
      opium_err(bitmask->log, "%s: Invalid parameters (off=%zu, len=%zu)\n",
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
      opium_err(bitmask->log, "%s: Bit count underflow\n", __func__);
      bitmask->mask[word] ^= mask;
      return false;
   }

   if (after > before && bitmask->count > bitmask->capacity - (size_t)(after - before)) {
      opium_err(bitmask->log, "%s: Bit count overflow\n", __func__);
      bitmask->mask[word] ^= mask;
      return false;
   }

   bitmask->count += after - before;
   opium_debug(bitmask->log, "Toggled %zu bits at %zu\n", len, pos);
   return true;
}

   int
opium_bitmask_push(opium_bitmask_t *bitmask, int pos, size_t len)
{
   if (opium_bitmask_isvalid_index(bitmask, pos + len) != OPIUM_RET_OK) {
      return OPIUM_RET_ERR;
   }

   if (len == 0 || (pos >= 0 && (size_t)pos + len > bitmask->capacity)) {
      opium_err(bitmask->log, "%s: Invalid parameters (pos=%d, len=%zu)\n",
            __func__, pos, len);
      return OPIUM_RET_ERR;
   }

   pos = pos < 0 ? 0 : pos;
   int start = opium_bitmask_fgb(bitmask, pos, len, 0);
   if (start < 0 || (size_t)start + len > bitmask->capacity) {
      return OPIUM_RET_ERR;
   }

   for (size_t i = start; i < start + len; i++) {
      opium_bitmask_set(bitmask, i);
   }

   return start;
}

   bool
opium_bitmask_pop(opium_bitmask_t *bitmask, int pos, size_t len)
{
   if (opium_bitmask_isvalid_index(bitmask, pos + len) != OPIUM_RET_OK) {
      return false;
   }

   if (len == 0 || (pos >= 0 && (size_t)pos + len > bitmask->capacity)) {
      opium_err(bitmask->log, "%s: Invalid parameters (start=%d, len=%zu)\n",
            __func__, pos, len);
      return false;
   }

   if (pos < 0) {
      int index = opium_bitmask_fgb(bitmask, 0, len, 1);
      if (index >= 0) {
         return opium_bitmask_pop(bitmask, index, len);
      }
      return true;
   }

   for (size_t i = pos; i < pos + len; i++) {
      opium_bitmask_clear(bitmask, i);
   }

   return true;
}

/* Debugging */
   void
opium_bitmask_debug(opium_bitmask_t *bitmask)
{
   if (opium_bitmask_isvalid_index(bitmask, 0) != OPIUM_RET_OK) {
      return;
   }

   for (size_t i = 0; i < bitmask->capacity; i++) {
      opium_debug_inline(bitmask->log, "%c", opium_bitmask_bit_test(bitmask, i) ? '1' : '0');
      if ((i + 1) % OPIUM_BITMASK_WORD_SIZE == 0) {
         opium_debug_inline(bitmask->log, "\n");
      }
   }

   opium_debug_inline(bitmask->log, "\n");
   opium_debug(bitmask->log, "Bitmask busy: %zu\n", bitmask->count);
}

/* Statics */
   static int 
opium_bitmask_set(opium_bitmask_t *bitmask, size_t off)
{
   if (opium_bitmask_isvalid_index(bitmask, off) != OPIUM_RET_OK) {
      if (opium_bitmask_isvalid(bitmask)) {
         opium_err(bitmask->log, "%s: Invalid offset %zu\n", __func__, off);
      }
      return OPIUM_RET_ERR;
   }

   size_t word = off / OPIUM_BITMASK_WORD_SIZE;
   uint64_t mask = 1ULL << (off % OPIUM_BITMASK_WORD_SIZE);

   if (!(bitmask->mask[word] & mask)) {
      bitmask->mask[word] |= mask;
      bitmask->count++;
   }

   return OPIUM_RET_OK;
}

   static int
opium_bitmask_clear(opium_bitmask_t *bitmask, size_t off)
{
   if (opium_bitmask_isvalid_index(bitmask, off) != OPIUM_RET_OK) {
      if (opium_bitmask_isvalid(bitmask)) {
         opium_err(bitmask->log, "%s: Invalid offset %zu\n", __func__, off);
      }
      return OPIUM_RET_ERR;
   }

   size_t word = off / OPIUM_BITMASK_WORD_SIZE;
   uint64_t mask = 1ULL << (off % OPIUM_BITMASK_WORD_SIZE);

   if (bitmask->mask[word] & mask) {
      bitmask->mask[word] &= ~mask;
      bitmask->count--;
   }

   return OPIUM_RET_OK;
}

   static void
opium_bitmask_trim(opium_bitmask_t *bitmask, size_t new_capacity)
{
   if (opium_bitmask_isvalid_index(bitmask, 0) != OPIUM_RET_OK) {
      return;
   }

   if (new_capacity >= bitmask->allocated_capacity) {
      return;
   }

   size_t start_word = new_capacity / OPIUM_BITMASK_WORD_SIZE;
   size_t bit_offset = new_capacity % OPIUM_BITMASK_WORD_SIZE;

   for (size_t w = start_word + (bit_offset ? 1 : 0); w < bitmask->word_capacity; w++) {
      bitmask->count -= __builtin_popcountll(bitmask->mask[w]);
      bitmask->mask[w] = 0;
   }

   if (bit_offset) {
      uint64_t mask = (1ULL << bit_offset) - 1;
      bitmask->count -= __builtin_popcountll(bitmask->mask[start_word] & ~mask);
      bitmask->mask[start_word] &= mask;
   }
}
