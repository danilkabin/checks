#ifndef UIDQ_BITMASK_INCLUDE_H
#define UIDQ_BITMASK_INCLUDE_H 

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

   size_t capacity;
   size_t allocated_capacity;
   size_t word_count;

   uint64_t *mask;

   uidq_log_t *log;
} uidq_bitmask_t;

// API

int uidq_bitmask_isvalid(uidq_bitmask_t *bm);
int uidq_bitmask_isvalid_index(uidq_bitmask_t *bm, size_t index);

uidq_bitmask_t *uidq_bitmask_create(size_t capacity, uidq_log_t *log); 
void uidq_bitmask_abort(uidq_bitmask_t *bm); 

int uidq_bitmask_init(uidq_bitmask_t *bm, size_t capacity, uidq_log_t *log);
void uidq_bitmask_exit(uidq_bitmask_t *bm);

int uidq_bitmask_realloc(uidq_bitmask_t *bm, size_t new_capacity); 
void uidq_bitmask_trim(uidq_bitmask_t *bm, size_t new_capacity);

int  uidq_bitmask_copy(uidq_bitmask_t **dst,  uidq_bitmask_t *src);
void uidq_bitmask_reset(uidq_bitmask_t *bm);

size_t uidq_bitmask_get_capacity(uidq_bitmask_t *bm);
void uidq_bitmask_lim_capacity(uidq_bitmask_t *bm, size_t new_capacity);
int  uidq_bitmask_is_empty(uidq_bitmask_t *bm);
int  uidq_bitmask_is_full(uidq_bitmask_t *bm);

int  uidq_bitmask_bit_test(uidq_bitmask_t *bm, size_t off);
int  uidq_bitmask_test_sequence(uidq_bitmask_t *bm, int target, size_t off, size_t len);

int  uidq_bitmask_ffb(uidq_bitmask_t *bm, int type);
int  uidq_bitmask_fgb(uidq_bitmask_t *bm, size_t off, size_t len, int type);

bool uidq_bitmask_toggle(uidq_bitmask_t *bm, size_t off, size_t len);

int  uidq_bitmask_push(uidq_bitmask_t *bm, int pos, size_t len);
bool uidq_bitmask_pop(uidq_bitmask_t *bm, int start, size_t len);

void uidq_bitmask_invert(uidq_bitmask_t *bm);
void uidq_bitmask_replace(uidq_bitmask_t *bm, size_t start, size_t len, size_t next);
int  uidq_bitmask_op(uidq_bitmask_t *dst, uidq_bitmask_t *src1, uidq_bitmask_t *src2, uidq_bitmask_op_t op);

void uidq_bitmask_debug(uidq_bitmask_t *bm);

#endif /* UIDQ_BITMASK_INCLUDE_H */
