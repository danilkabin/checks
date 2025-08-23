#ifndef UIDQ_BITMASK_H
#define UIDQ_BITMASK_H

#include "core/uidq_log.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
   UIDQ_BITMASK_OP_AND,
   UIDQ_BITMASK_OP_OR,
   UIDQ_BITMASK_OP_XOR,
   UIDQ_BITMASK_OP_NAND,
   UIDQ_BITMASK_OP_NOR,
   UIDQ_BITMASK_OP_XNOR
} uidq_bitmask_op_t;

typedef struct {
   bool initialized;
   uint64_t *mask;
   size_t bit_capacity;
   size_t bit_count;
   size_t word_bits;
   size_t word_capacity;
   uidq_log_t *log;
} uidq_bitmask_t;

// Creation and initialization
uidq_bitmask_t *uidq_bitmask_create(void);
uidq_bitmask_t *uidq_bitmask_create_and_init(uidq_log_t *log, size_t bits, size_t word_sz);
int uidq_bitmask_init(uidq_bitmask_t *bm, uidq_log_t *log, size_t bits, size_t word_sz);
void uidq_bitmask_free(uidq_bitmask_t *bm);
void uidq_bitmask_exit(uidq_bitmask_t *bm);

// Bitmask state and debugging
int uidq_bitmask_is_empty(const uidq_bitmask_t *bm);
void uidq_bitmask_debug(const uidq_bitmask_t *bm);

// Basic bit operations
bool uidq_bitmask_set(uidq_bitmask_t *bm, size_t off);
bool uidq_bitmask_clear(uidq_bitmask_t *bm, size_t off);
void uidq_bitmask_reset(uidq_bitmask_t *bm);
int uidq_bitmask_bit_test(const uidq_bitmask_t *bm, size_t off);
void uidq_bitmask_invert(uidq_bitmask_t *bm);

// Sequence operations
int uidq_bitmask_test_sequence(const uidq_bitmask_t *bm, int target, size_t off, size_t len);
int uidq_bitmask_find_first_bit(const uidq_bitmask_t *bm, int type);
int uidq_bitmask_find_grab_bit(const uidq_bitmask_t *bm, size_t off, size_t len, int type);
bool uidq_bitmask_toggle(uidq_bitmask_t *bm, size_t off, size_t len);
bool uidq_bitmask_del(uidq_bitmask_t *bm, size_t start, size_t len);

// Advanced bit operations
int uidq_bitmask_add(uidq_bitmask_t *bm, int pos, size_t len);
int uidq_bitmask_add_force(uidq_bitmask_t *bm, int pos, size_t len);
int uidq_bitmask_add_with_toggle(uidq_bitmask_t *bm, size_t off, size_t last_len, size_t len);
int uidq_bitmask_add_with_fallback(uidq_bitmask_t *bm, size_t off, size_t last_len, size_t len);
void uidq_bitmask_replace(uidq_bitmask_t *bm, size_t start, size_t len, size_t next);
int uidq_bitmask_copy(uidq_bitmask_t *dst, uidq_bitmask_t *src);

// File operations
int uidq_bitmask_save(uidq_bitmask_t *bm, int fd);
int uidq_bitmask_load(uidq_bitmask_t *bm, int fd);

// Bitwise operations
int uidq_bitmask_op(uidq_bitmask_t *dst, uidq_bitmask_t *src1, uidq_bitmask_t *src2, uidq_bitmask_op_t op);

#endif /* UIDQ_BITMASK_H */
