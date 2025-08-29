#include "core/uidq_bitmask.h"
#include "core/uidq_bitmask.h"
#include "core/uidq_alloc.h"
#include "core/uidq_conf.h"
#include "core/uidq_log.h"
#include "core/uidq_core.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define UIDQ_BITMASK_WORD_SIZE (sizeof(uint64_t) * 8) 

   int
uidq_bitmask_isvalid(uidq_bitmask_t *bitmask)
{
   return bitmask && bitmask->initialized == 1;
}

   int
uidq_bitmask_isvalid_index(uidq_bitmask_t *bitmask, size_t index)
{
   return uidq_bitmask_isvalid(bitmask) && bitmask->mask && index < bitmask->capacity;
}

   uidq_bitmask_t *
uidq_bitmask_create(size_t capacity, uidq_log_t *log) 
{
   uidq_bitmask_t *bitmask = uidq_calloc(sizeof(uidq_bitmask_t), log);
   if (!bitmask) {
      uidq_err(log, "Bitmask allocation failed\n");
      return NULL;
   }
   bitmask->initialized = 1;

   if (uidq_bitmask_init(bitmask, capacity, log) != UIDQ_RET_OK) {
      uidq_err(log, "Failed to init bitmask struct.\n"); 
      uidq_bitmask_abort(bitmask);
      return NULL;
   }

   return bitmask;
}

   void
uidq_bitmask_abort(uidq_bitmask_t *bitmask)
{
   if (!uidq_bitmask_isvalid(bitmask)) return;

   uidq_bitmask_exit(bitmask);
   bitmask->initialized = 0;
   uidq_free(bitmask, NULL);
}

int
uidq_bitmask_init(uidq_bitmask_t *bitmask, size_t capacity, uidq_log_t *log) {
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   if (capacity == 0) {
      uidq_err(log, "%s: Invalid Parameters.\n", __func__);
      return UIDQ_RET_ERR;
   }

   bitmask->word_count = (capacity + UIDQ_BITMASK_WORD_SIZE - 1) / UIDQ_BITMASK_WORD_SIZE;
   bitmask->capacity = capacity;
   bitmask->allocated_capacity = capacity;
   bitmask->log = log;

   bitmask->mask = uidq_calloc(bitmask->capacity, log);
   if (!bitmask->mask) {
      uidq_err(bitmask->log, "%s: Failed to allocate bitmask data.\n", __func__);
      goto failed;
   }

   return UIDQ_RET_OK;

failed:
   uidq_bitmask_exit(bitmask);
   return UIDQ_RET_ERR;
}

   void
uidq_bitmask_exit(uidq_bitmask_t *bitmask)
{
   if (!uidq_bitmask_isvalid(bitmask)) return;
   bitmask->bits = 0;
   bitmask->count = 0;
   bitmask->word_capacity = 0;
   bitmask->capacity = 0;
   bitmask->log = NULL;

   if (bitmask->mask) {
      uidq_free(bitmask->mask, NULL);
      bitmask->mask = NULL;
   }
}

   int
uidq_bitmask_realloc(uidq_bitmask_t *bitmask, size_t new_capacity)
{
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   if (new_capacity == 0) {
      uidq_err(bitmask->log, "bitmask: invalid conf in init");
      return UIDQ_RET_ERR;
   }

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(bitmask);

   size_t old_capacity = config->capacity;
   size_t old_wd = bitmask->word_capacity;
   size_t new_wd = (new_capacity + WORD_BITS - 1) / WORD_BITS;

   if (new_capacity < old_capacity) {
      uidq_bitmask_trim(bitmask, new_capacity);
   }

   uint64_t *new_mask = realloc(bitmask->mask, new_wd * sizeof(uint64_t));
   if (!new_mask) return UIDQ_RET_ERR;

   if (new_capacity > old_capacity) {
      memset(new_mask + old_wd, 0, (new_wd - old_wd) * sizeof(uint64_t));
   }

   config->capacity = new_capacity;
   bitmask->word_capacity = new_wd;
   bitmask->mask = new_mask;

   return UIDQ_RET_OK;
}

   void
uidq_bitmask_trim(uidq_bitmask_t *bitmask, size_t new_capacity)
{
   if (!uidq_bitmask_isvalid(bitmask)) return;

   size_t old_capacity = uidq_bitmask_conf_get(bitmask)->capacity;
   if (new_capacity >= old_capacity) return;

   size_t start_word = new_capacity / WORD_BITS;
   size_t bit_offset = new_capacity % WORD_BITS;

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

int 
uidq_bitmask_copy(uidq_bitmask_t **dst, uidq_bitmask_t *src) {
   if (!uidq_bitmask_isvalid(src)) return UIDQ_RET_ERR;

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(src);

   uidq_bitmask_abort(*dst);

   *dst = uidq_bitmask_create(config, src->log);
   if (!*dst) return UIDQ_RET_ERR;

   memcpy((*dst)->mask, src->mask, src->word_capacity * sizeof(uint64_t));
   (*dst)->count = src->count;
   return UIDQ_RET_OK;
}

   static inline void
bitmask_clear_mem(uidq_bitmask_t *bitmask)
{
   if (!uidq_bitmask_isvalid(bitmask)) return;

   if (bitmask->mask) {
      memset(bitmask->mask, 0, bitmask->word_capacity * sizeof(uint64_t));
      bitmask->count = 0;
      uidq_debug(bitmask->log, "Bitmask cleared\n");
   }
}

   void
uidq_bitmask_reset(uidq_bitmask_t *bitmask)
{
   if (!uidq_bitmask_isvalid(bitmask)) return;

   bitmask_clear_mem(bitmask);
}

   int
uidq_bitmask_is_empty(uidq_bitmask_t *bitmask)
{
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   return bitmask->count <= 0 ? UIDQ_RET_OK : UIDQ_RET_ERR;
}

   int
uidq_bitmask_is_full(uidq_bitmask_t *bitmask)
{
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   return bitmask->count >= bitmask->capacity ? UIDQ_RET_OK : UIDQ_RET_ERR;
}

   size_t
uidq_bitmask_get_capacity(uidq_bitmask_t *bitmask)
{
   if (!uidq_bitmask_isvalid(bitmask)) return 0;

   return bitmask->capacity;
}

   void
uidq_bitmask_lim_capacity(uidq_bitmask_t *bitmask, size_t new_capacity)
{
   if (!uidq_bitmask_isvalid(bitmask)) return;

   if (new_capacity > bitmask->real_capacity) {
      return;
   }
   bitmask->capacity = new_capacity;
}

   bool
uidq_bitmask_set(uidq_bitmask_t *bitmask, size_t off)
{
   if (!uidq_bitmask_isvalid_index(bitmask, off)) return false;

   size_t word = off / bitmask->bits;
   uint64_t mask = 1ULL << (off % bitmask->bits);

   if (!(bitmask->mask[word] & mask)) {
      bitmask->mask[word] |= mask;
      bitmask->count++;
      // uidq_debug(bitmask->log, "Set bit at %zu\n", off);
   }

   return true;
}

   bool
uidq_bitmask_clear(uidq_bitmask_t *bitmask, size_t off)
{
   if (!uidq_bitmask_isvalid_index(bitmask, off)) return false;

   size_t word = off / bitmask->bits;
   uint64_t mask = 1ULL << (off % bitmask->bits);

   if (bitmask->mask[word] & mask) {
      bitmask->mask[word] &= ~mask;
      bitmask->count--;
      uidq_debug(bitmask->log, "Cleared bit at %zu\n", off);
   }

   return true;
}

   int 
uidq_bitmask_bit_test(uidq_bitmask_t *bitmask, size_t off)
{
   if (!uidq_bitmask_isvalid_index(bitmask, off)) return UIDQ_RET_ERR;

   return (bitmask->mask[off / bitmask->bits] >> (off % bitmask->bits)) & 1;
}

   int 
uidq_bitmask_test_sequence(uidq_bitmask_t *bitmask, int target, size_t off, size_t len)
{
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(bitmask);
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;
   if (len == 0 || off + len > config->capacity) {
      uidq_err(bitmask->log, "Invalid sequence parameters\n");
      return UIDQ_RET_ERR;
   }

   for (size_t i = off; i < off + len; i++) {
      if (uidq_bitmask_bit_test(bitmask, i) != target) return UIDQ_RET_ERR;
   }

   return off;
}

   int
uidq_bitmask_ffb(uidq_bitmask_t *bitmask, int type)
{
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(bitmask);
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;
   if (type != 0 && type != 1) {
      uidq_err(bitmask->log, "Invalid find parameters\n");
      return UIDQ_RET_ERR;
   }

   for (size_t i = 0; i < bitmask->word_capacity; i++) {
      uint64_t word = type ? bitmask->mask[i] : ~bitmask->mask[i];
      if (word) {
         size_t pos = i * bitmask->bits + __builtin_ctzll(word);
         if (pos >= config->capacity) return UIDQ_RET_ERR;
         return pos;
      }
   }

   return UIDQ_RET_ERR;
}

   int
uidq_bitmask_fgb(uidq_bitmask_t *bitmask, size_t off, size_t len, int type)
{
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(bitmask);
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;
   if ((type != 0 && type != 1) || len == 0 || off + len > config->capacity) {
      uidq_err(bitmask->log, "Invalid grab parameters\n");
      return UIDQ_RET_ERR;
   }

   size_t bits_per_word = bitmask->bits;
   size_t start = 0;
   size_t count = 0;
   for (size_t w = off / bits_per_word; w < bitmask->word_capacity; w++) {
      uint64_t word = type ? bitmask->mask[w] : ~bitmask->mask[w];

      if (w == off / bits_per_word && off % bits_per_word) {
         word &= (~0ULL << (off % bits_per_word));
      }

      size_t bit_pos = 0;
      while (bit_pos < bits_per_word) {
         if (word & (1ULL << bit_pos)) {
            if (count == 0) start = w * bits_per_word + bit_pos;
            count++;
            if (count >= len) return start;
         } else {
            count = 0;
         }
         bit_pos++;
      }
   }

   return UIDQ_RET_ERR;
}

   bool
uidq_bitmask_toggle(uidq_bitmask_t *bitmask, size_t off, size_t len)
{
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(bitmask);
   if (!uidq_bitmask_isvalid_index(bitmask, off)) return UIDQ_RET_ERR;
   if (off + len > config->capacity) {
      uidq_err(bitmask->log, "Invalid toggle parameters\n");
      return false;
   }

   size_t word = off / bitmask->bits;
   size_t bit  = off % bitmask->bits;

   uint64_t mask = (len >= bitmask->bits) ? ~0ULL : ((1ULL << len) - 1);
   mask <<= bit;

   int before = __builtin_popcountll(bitmask->mask[word] & mask);
   bitmask->mask[word] ^= mask;
   int after  = __builtin_popcountll(bitmask->mask[word] & mask);

   if (before > after && bitmask->count < (size_t)(before - after)) {
      uidq_err(bitmask->log, "Bit count underflow\n");
      bitmask->mask[word] ^= mask;
      return false;
   }

   if (after > before && bitmask->count > config->capacity - (size_t)(after - before)) {
      uidq_err(bitmask->log, "Bit count overflow\n");
      bitmask->mask[word] ^= mask;
      return false;
   }

   bitmask->count += after - before;
   uidq_debug(bitmask->log, "Toggled %zu bits at %zu\n", len, off);
   return true;
}

   int 
uidq_bitmask_push(uidq_bitmask_t *bitmask, int pos, size_t len)
{
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(bitmask);
   pos = pos < 0 ? 0 : pos;
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;
   if ((size_t)pos >= config->capacity || len == 0 || (size_t)pos + len > config->capacity) {
      uidq_err(bitmask->log, "Invalid add parameters\n");
      return UIDQ_RET_ERR;
   }

   int start = uidq_bitmask_fgb(bitmask, pos, len, 0);
   if (start < 0 || (size_t)start + len > config->capacity) {
      uidq_err(bitmask->log, "No sufficient free bits\n");
      return UIDQ_RET_ERR;
   }

   for (size_t i = start; i < start + len; i++) uidq_bitmask_set(bitmask, i);
   return start;
}

   bool
uidq_bitmask_pop(uidq_bitmask_t *bitmask, int start, size_t len)
{
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(bitmask);
   if (!uidq_bitmask_isvalid(bitmask)) return UIDQ_RET_ERR;

   if (len == 0 || (start + len > config->capacity)) {
      uidq_err(bitmask->log, "Invalid add parameters\n");
      return UIDQ_RET_ERR;
   }

   if (start < 0) {
      int index = uidq_bitmask_fgb(bitmask, 0, len, 1);
      if (index >= 0) {
         uidq_bitmask_pop(bitmask, index, len);
      }
      return UIDQ_RET_OK;
   }

   for (size_t index = start; index < start + len; index++) {
      uidq_bitmask_clear(bitmask, index);
   }

   return true;
}

   void
uidq_bitmask_invert(uidq_bitmask_t *bitmask)
{
   if (!uidq_bitmask_isvalid(bitmask)) return;

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(bitmask);
   if (!uidq_bitmask_isvalid(bitmask)) return;

   for (size_t i = 0; i < bitmask->word_capacity; i++) bitmask->mask[i] = ~bitmask->mask[i];
   bitmask->count = config->capacity - bitmask->count;
   //uidq_debug(bitmask->log, "Bitmask inverted\n");
}

   void 
uidq_bitmask_replace(uidq_bitmask_t *bitmask, size_t start, size_t len, size_t next)
{
   if (!uidq_bitmask_isvalid(bitmask)) return;

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(bitmask);
   if (!uidq_bitmask_isvalid(bitmask)) return;
   if (start + len > config->capacity || next + len > config->capacity) {
      uidq_err(bitmask->log, "Invalid replace parameters\n");
      return;
   }

   uint64_t *tmp = uidq_malloc(bitmask->word_capacity * sizeof(uint64_t), bitmask->log);
   if (!tmp) {
      uidq_err(bitmask->log, "Temp allocation failed\n");
      return;
   }
   memcpy(tmp, bitmask->mask, bitmask->word_capacity * sizeof(uint64_t));

   size_t old_count = 0, new_count = 0;

   /* clear bits at start */
   size_t bits_left = len, pos = start;
   while (bits_left > 0) {
      size_t w_idx = pos / bitmask->bits;
      size_t b_idx = pos % bitmask->bits;
      size_t bits_in_word = bitmask->bits - b_idx;
      size_t to_clear = uidq_min(bits_left, bits_in_word);
      uint64_t mask = (to_clear == bitmask->bits) ? ~0ULL : ((1ULL << to_clear) - 1) << b_idx;
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
      size_t sw = pos / bitmask->bits, sb = pos % bitmask->bits;
      size_t dw = n_pos / bitmask->bits, db = n_pos % bitmask->bits;
      size_t src_left = bitmask->bits - sb, dst_left = bitmask->bits - db;
      size_t copy = uidq_min(bits_left, uidq_min(src_left, dst_left));
      uint64_t mask = (copy == bitmask->bits) ? ~0ULL : ((1ULL << copy) - 1);
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

   free(tmp);
}

   int
uidq_bitmask_op(uidq_bitmask_t *dst, uidq_bitmask_t *src1, uidq_bitmask_t *src2,
      uidq_bitmask_op_t op)
{
   uidq_bitmask_conf_t *dst_conf = uidq_bitmask_conf_get(dst);
   uidq_bitmask_conf_t *src1_conf = uidq_bitmask_conf_get(src1);
   uidq_bitmask_conf_t *src2_conf = uidq_bitmask_conf_get(src2);
   if (!uidq_bitmask_isvalid(src1) || !uidq_bitmask_isvalid(src2) || !uidq_bitmask_isvalid(dst))
      return UIDQ_RET_ERR;

   if (dst_conf->capacity != src1_conf->capacity || src1_conf->capacity != src2_conf->capacity) {
      uidq_err(dst->log, "Invalid op parameters\n");
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

   if (count > dst_conf->capacity) {
      uidq_err(dst->log, "Bit count overflow in operation\n");
      return UIDQ_RET_ERR;
   }

   dst->count = count;
   return UIDQ_RET_OK;
}

   void 
uidq_bitmask_debug(uidq_bitmask_t *bitmask)
{
   if (!uidq_bitmask_isvalid(bitmask)) return;

   uidq_bitmask_conf_t *config = uidq_bitmask_conf_get(bitmask);
   if (!uidq_bitmask_isvalid(bitmask)) return;

   for (size_t i = 0; i < config->capacity; i++) {
      putchar(uidq_bitmask_bit_test(bitmask, i) ? '1' : '0');
      if ((i + 1) % bitmask->bits == 0) putchar('\n');
   }

   printf("\n");
   uidq_debug(bitmask->log, "Bitmask busy: %zu\n", bitmask->count);
}
