#ifndef UIDQ_BITMASK_H
#define UIDQ_BITMASK_H

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
   uint64_t *mask;
   size_t bit_capacity;
   size_t bit_count;
   size_t word_bits;
   size_t word_capacity;
   bool initialized;
} uidq_bitmask_t;

int uidq_bitmask_is_empty(const uidq_bitmask_t *bitmask);
void uidq_bitmask_bits_clear(uidq_bitmask_t *bitmask);
void uidq_bitmask_debug(const uidq_bitmask_t *bitmask);

uidq_bitmask_t *uidq_bitmask_create(void);
void uidq_bitmask_free(uidq_bitmask_t *bitmask);
int uidq_bitmask_init(uidq_bitmask_t *bitmask, size_t bit_capacity, size_t word_size);
void uidq_bitmask_exit(uidq_bitmask_t *bitmask);
uidq_bitmask_t *uidq_bitmask_create_and_init(size_t bit_capacity, size_t word_size);

int uidq_bitmask_copy(uidq_bitmask_t *dest, uidq_bitmask_t *src);

bool uidq_bitmask_set(uidq_bitmask_t *bitmask, size_t offset);
bool uidq_bitmask_clear(uidq_bitmask_t *bitmask, size_t offset);
bool uidq_bitmask_toggle(uidq_bitmask_t *bitmask, size_t offset, size_t grab);
int uidq_bitmask_add_with_toggle(uidq_bitmask_t *bitmask, size_t offset, size_t last_grab, size_t grab);
int uidq_bitmask_add_with_fallback(uidq_bitmask_t *bitmask, size_t offset, size_t last_grab, size_t grab);

int uidq_bitmask_bit_test(const uidq_bitmask_t *bitmask, size_t offset);
int uidq_bitmask_test_sequence(const uidq_bitmask_t *bitmask, int target, size_t offset, size_t grab);

int uidq_bitmask_find_first_bit(const uidq_bitmask_t *bitmask, int type);
int uidq_bitmask_find_grab_bit(const uidq_bitmask_t *bitmask, size_t offset, size_t grab, int type);

int uidq_bitmask_add(uidq_bitmask_t *bitmask, int pos, size_t offset);
int uidq_bitmask_add_out_of_range(uidq_bitmask_t *bitmask, int pos, size_t offset);
bool uidq_bitmask_del(uidq_bitmask_t *bitmask, size_t start_pos, size_t count);

void uidq_bitmask_replace(uidq_bitmask_t *bitmask, size_t start_pos, size_t offset, size_t next_pos);

int uidq_bitmask_save(uidq_bitmask_t *bitmask, int file);
int uidq_bitmask_load(uidq_bitmask_t *bitmask, int file);

void uidq_bitmask_invert(uidq_bitmask_t *bitmask);
int uidq_bitmask_op(uidq_bitmask_t *dest, uidq_bitmask_t *src1, uidq_bitmask_t *src2, uidq_bitmask_op_t opt);

#endif
