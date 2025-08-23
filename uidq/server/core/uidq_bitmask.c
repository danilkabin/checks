#include "core/uidq_bitmask.h"
#include "core/uidq_alloc.h"
#include "core/uidq_log.h"
#include "core/uidq_types.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define WORD_BITS (sizeof(uint64_t) * 8)

#define UIDQ_BM_MINIMUM_CAPACITY 64 

int
uida_bm_isvalid(uidq_bitmask_t *bm) 
{
   return bm && bm->initialized == 1;
}

   static inline bool 
is_valid_index(const uidq_bitmask_t *bm, size_t index) 
{
   return is_valid(bm) && index < bm->capacity;
}

uidq_hash_conf_t *
uidq_hash_conf_get(uidq_hash_t *hash) {
   return &hash->conf;
}

   uidq_bitmask_t *
uidq_bitmask_create(uidq_bitmask_conf_t *bm, uidq_log_t *log) 
{
   if (bm->capacity == 0) {
      uidq_err(log, "Invalid parameters\n");
      return NULL;
   }

   uidq_bitmask_t *bm = calloc(1, sizeof(uidq_bitmask_t));
   if (!bm) {
      uidq_err(log, "Bitmask allocation failed\n");
      return NULL;
   }

   bm->bits          = WORD_BITS;
   bm->capacity      = capacity;
   bm->word_capacity = (capacity + WORD_BITS - 1) / WORD_BITS;
   bm->log           = log;
   bm->mask          = calloc(bm->word_capacity, sizeof(uint64_t));

   if (!bm->mask) {
      uidq_err(log, "Memory allocation failed\n");
      free(bm);
      return NULL;
   }

   bm->initialized = true;
   return bm;
}

   void
uidq_bitmask_abort(uidq_bitmask_t *bm) 
{
   if (!bm) return;
   free(bm->mask);
   free(bm);
}

int
uidq_bitmask_init(uidq_bitmask_conf_t *conf, uidq_log_t *log) {

}

int 
uidq_bitmask_copy(uidq_bitmask_t **dst, const uidq_bitmask_t *src) {
   if (!is_valid(src)) return UIDQ_RET_ERR;

   uidq_bitmask_free(*dst);
   *dst = uidq_bitmask_create(src->capacity, src->log);
   if (!*dst) return UIDQ_RET_ERR;

   memcpy((*dst)->mask, src->mask, src->word_capacity * sizeof(uint64_t));
   (*dst)->count = src->count;
   return UIDQ_RET_OK;
}

   static inline void
bitmask_clear_mem(uidq_bitmask_t *bm) 
{
   if (bm->mask) {
      memset(bm->mask, 0, bm->word_capacity * sizeof(uint64_t));
      bm->count = 0;
      uidq_debug(bm->log, "Bitmask cleared\n");
   }
}

   void
uidq_bitmask_reset(uidq_bitmask_t *bm)
{
   if (!is_valid(bm)) return;
   bitmask_clear_mem(bm);
}

   int
uidq_bitmask_is_empty(const uidq_bitmask_t *bm)
{
   if (!is_valid(bm)) return UIDQ_RET_ERR;
   return bm->count == 0 ? UIDQ_RET_OK : UIDQ_RET_ERR;
}

   bool
uidq_bitmask_set(uidq_bitmask_t *bm, size_t off) 
{
   if (!is_valid_index(bm, off)) return false;
   size_t word = off / bm->bits;
   uint64_t mask = 1ULL << (off % bm->bits);

   if (!(bm->mask[word] & mask)) {
      bm->mask[word] |= mask;
      bm->count++;
      uidq_debug(bm->log, "Set bit at %zu\n", off);
   }

   return true;
}

   bool
uidq_bitmask_clear(uidq_bitmask_t *bm, size_t off) 
{
   if (!is_valid_index(bm, off)) return false;
   size_t word = off / bm->bits;
   uint64_t mask = 1ULL << (off % bm->bits);

   if (bm->mask[word] & mask) {
      bm->mask[word] &= ~mask;
      bm->count--;
      uidq_debug(bm->log, "Cleared bit at %zu\n", off);
   }

   return true;
}

   int 
uidq_bitmask_bit_test(const uidq_bitmask_t *bm, size_t off) 
{
   if (!is_valid_index(bm, off)) return UIDQ_RET_ERR;
   return (bm->mask[off / bm->bits] >> (off % bm->bits)) & 1;
}

   int 
uidq_bitmask_test_sequence(const uidq_bitmask_t *bm, int target, size_t off, size_t len)
{
   if (!is_valid(bm) || len == 0 || off + len > bm->capacity) {
      uidq_err(bm->log, "Invalid sequence parameters\n");
      return UIDQ_RET_ERR;
   }

   for (size_t i = off; i < off + len; i++) {
      if (uidq_bitmask_bit_test(bm, i) != target) return UIDQ_RET_ERR;
   }

   return off;
}

   int
uidq_bitmask_ffb(const uidq_bitmask_t *bm, int type) 
{
   if (!is_valid(bm) || (type != 0 && type != 1)) {
      uidq_err(bm->log, "Invalid find parameters\n");
      return UIDQ_RET_ERR;
   }

   for (size_t i = 0; i < bm->word_capacity; i++) {
      uint64_t word = type ? bm->mask[i] : ~bm->mask[i];
      if (word) {
         size_t pos = i * bm->bits + __builtin_ctzll(word);
         if (pos >= bm->capacity) return UIDQ_RET_ERR;
         return pos;
      }
   }

   return UIDQ_RET_ERR;
}

   int
uidq_bitmask_fgb(const uidq_bitmask_t *bm, size_t off, size_t len, int type) 
{
   if (!is_valid(bm) || (type != 0 && type != 1) || len == 0 || off + len > bm->capacity) {
      uidq_err(bm->log, "Invalid grab parameters\n");
      return UIDQ_RET_ERR;
   }
   // Track consecutive bits of the desired type
   size_t count = 0, start = (size_t)-1;
   for (size_t w = off / bm->bits; w < bm->word_capacity; w++) {
      uint64_t word = type ? bm->mask[w] : ~bm->mask[w];
      // Mask bits before offset in the first word
      if (w == off / bm->bits && off % bm->bits) {
         word &= (~0ULL << (off % bm->bits));
      }
      while (word) {
         int bit_pos = __builtin_ctzll(word);
         if (count == 0) start = w * bm->bits + bit_pos;
         // Count consecutive bits in current run
         int run = 0;
         while (bit_pos + run < (int)bm->bits && (word & (1ULL << (bit_pos + run)))) {
            run++;
         }
         count += run;
         if (count >= len) return start;
         word &= ~(((1ULL << run) - 1) << bit_pos);
         count = 0;
      }
      if (word != ~0ULL) count = 0;
   }
   return UIDQ_RET_ERR;

}

   bool
uidq_bitmask_toggle(uidq_bitmask_t *bm, size_t off, size_t len)
{
   if (!is_valid_index(bm, off) || off + len > bm->capacity) {
      uidq_err(bm->log, "Invalid toggle parameters\n");
      return false;
   }

   size_t word = off / bm->bits;
   size_t bit  = off % bm->bits;

   uint64_t mask = (len >= bm->bits) ? ~0ULL : ((1ULL << len) - 1);
   mask <<= bit;

   int before = __builtin_popcountll(bm->mask[word] & mask);
   bm->mask[word] ^= mask;
   int after  = __builtin_popcountll(bm->mask[word] & mask);

   if (before > after && bm->count < (size_t)(before - after)) {
      uidq_err(bm->log, "Bit count underflow\n");
      bm->mask[word] ^= mask;
      return false;
   }

   if (after > before && bm->count > bm->capacity - (size_t)(after - before)) {
      uidq_err(bm->log, "Bit count overflow\n");
      bm->mask[word] ^= mask;
      return false;
   }

   bm->count += after - before;
   uidq_debug(bm->log, "Toggled %zu bits at %zu\n", len, off);
   return true;
}

   int 
uidq_bitmask_push(uidq_bitmask_t *bm, int pos, size_t len) 
{
   pos = pos < 0 ? 0 : pos;

   if (!is_valid(bm) || (size_t)pos >= bm->capacity || len == 0 || (size_t)pos + len > bm->capacity) {
      uidq_err(bm->log, "Invalid add parameters\n");
      return UIDQ_RET_ERR;
   }

   int start = uidq_bitmask_fgb(bm, pos, len, 0);
   if (start < 0 || (size_t)start + len > bm->capacity) {
      uidq_err(bm->log, "No sufficient free bits\n");
      return UIDQ_RET_ERR;
   }

   for (size_t i = start; i < start + len; i++) uidq_bitmask_set(bm, i);
   return start;
}

   bool
uidq_bitmask_pop(uidq_bitmask_t *bm, size_t start, size_t len)
{
   if (!is_valid_index(bm, start) || len == 0 || start + len > bm->capacity) {
      uidq_err(bm->log, "Invalid del parameters\n");
      return false;
   }

   for (size_t i = start; i < start + len; i++) uidq_bitmask_clear(bm, i);
   return true;
}

   void
uidq_bitmask_invert(uidq_bitmask_t *bm) 
{
   if (!is_valid(bm)) return;
   for (size_t i = 0; i < bm->word_capacity; i++) bm->mask[i] = ~bm->mask[i];
   bm->count = bm->capacity - bm->count;
   uidq_debug(bm->log, "Bitmask inverted\n");
}

   void 
uidq_bitmask_replace(uidq_bitmask_t *bm, size_t start, size_t len, size_t next) 
{
   if (!is_valid(bm) || start + len > bm->capacity || next + len > bm->capacity) {
      uidq_err(bm->log, "Invalid replace parameters\n");
      return;
   }

   uint64_t *tmp = uidq_malloc(bm->word_capacity * sizeof(uint64_t), bm->log);
   if (!tmp) {
      uidq_err(bm->log, "Temp allocation failed\n");
      return;
   }
   memcpy(tmp, bm->mask, bm->word_capacity * sizeof(uint64_t));

   size_t old_count = 0, new_count = 0;

   /* clear bits at start */
   size_t bits_left = len, pos = start;
   while (bits_left > 0) {
      size_t w_idx = pos / bm->bits;
      size_t b_idx = pos % bm->bits;
      size_t bits_in_word = bm->bits - b_idx;
      size_t to_clear = UIDQ_MIN(bits_left, bits_in_word);
      uint64_t mask = (to_clear == bm->bits) ? ~0ULL : ((1ULL << to_clear) - 1) << b_idx;
      old_count += __builtin_popcountll(tmp[w_idx] & mask);
      tmp[w_idx] &= ~mask;
      bits_left -= to_clear;
      pos += to_clear;
   }

   /* copy bits to new position */
   bits_left = len;
   pos = start;
   size_t n_pos = next;
   while (bits_left > 0) {
      size_t sw = pos / bm->bits, sb = pos % bm->bits;
      size_t dw = n_pos / bm->bits, db = n_pos % bm->bits;
      size_t src_left = bm->bits - sb, dst_left = bm->bits - db;
      size_t copy = UIDQ_MIN(bits_left, UIDQ_MIN(src_left, dst_left));
      uint64_t mask = (copy == bm->bits) ? ~0ULL : ((1ULL << copy) - 1);
      uint64_t src_mask = mask << sb;
      uint64_t dst_mask = mask << db;
      uint64_t extracted = (bm->mask[sw] & src_mask) >> sb;
      new_count += __builtin_popcountll(tmp[dw] & dst_mask);
      tmp[dw] &= ~dst_mask;
      tmp[dw] |= (extracted << db);
      new_count += __builtin_popcountll(extracted << db);
      bits_left -= copy;
      pos += copy;
      n_pos += copy;
   }

   bm->count = bm->count - old_count + new_count;
   memcpy(bm->mask, tmp, bm->word_capacity * sizeof(uint64_t));

   free(tmp);
}

   int
uidq_bitmask_op(uidq_bitmask_t *dst, uidq_bitmask_t *src1, uidq_bitmask_t *src2, uidq_bitmask_op_t op)
{
   if (!is_valid(dst) || !is_valid(src1) || !is_valid(src2) ||
         dst->capacity != src1->capacity || src1->capacity != src2->capacity) {
      uidq_err(dst ? dst->log : NULL, "Invalid op parameters\n");
      return UIDQ_RET_ERR;
   }

   size_t count = 0;
   for (size_t i = 0; i < dst->word_capacity; i++) {
      switch (op) {
         case UIDQ_BITMASK_OP_AND:  dst->mask[i] = src1->mask[i] & src2->mask[i]; break;
         case UIDQ_BITMASK_OP_OR:   dst->mask[i] = src1->mask[i] | src2->mask[i]; break;
         case UIDQ_BITMASK_OP_XOR:  dst->mask[i] = src1->mask[i] ^ src2->mask[i]; break;
         case UIDQ_BITMASK_OP_NAND: dst->mask[i] = ~(src1->mask[i] & src2->mask[i]); break;
         case UIDQ_BITMASK_OP_NOR:  dst->mask[i] = ~(src1->mask[i] | src2->mask[i]); break;
         case UIDQ_BITMASK_OP_XNOR: dst->mask[i] = ~(src1->mask[i] ^ src2->mask[i]); break;
         default:
                                    uidq_err(dst->log, "Invalid bitwise operation\n");
                                    return UIDQ_RET_ERR;
      }
      count += __builtin_popcountll(dst->mask[i]);
   }

   if (count > dst->capacity) {
      uidq_err(dst->log, "Bit count overflow in operation\n");
      return UIDQ_RET_ERR;
   }

   dst->count = count;
   return UIDQ_RET_OK;
}

   void 
uidq_bitmask_debug(const uidq_bitmask_t *bm) 
{
   if (!is_valid(bm)) return;

   for (size_t i = 0; i < bm->capacity; i++) {
      putchar(uidq_bitmask_bit_test(bm, i) ? '1' : '0');
      if ((i + 1) % bm->bits == 0) putchar('\n');
   }

   printf("\n");
   uidq_debug(bm->log, "Bitmask busy: %zu\n", bm->count);
}
