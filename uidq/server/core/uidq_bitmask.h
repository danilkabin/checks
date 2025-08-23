#ifndef UIDQ_BITMASK_INCLUDE_H
#define UIDQ_BITMASK_INCLUDE_H 

#include "core/uidq_log.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
   size_t capacity;
} uidq_bitmask_conf_t;

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
   size_t count;
   size_t bits;
   size_t word_capacity;

   uint64_t *mask;

   uidq_log_t *log;
} uidq_bitmask_t;

// API

uidq_bitmask_t   *uidq_bitmask_create(uidq_bitmask_conf_t *conf, uidq_log_t *log);
void             uidq_bitmask_abort(uidq_bitmask_t *bm);

int  uidq_bitmask_init(uidq_bitmask_conf_t *conf, uidq_log_t *log);
int  uidq_bitmask_exit(uidq_bitmask_t *bm);

uidq_bitmask_t *uidq_bitmask_realloc(size_t new_capacity, uidq_log_t *log);

int  uidq_bitmask_copy(uidq_bitmask_t **dst, const uidq_bitmask_t *src);
void uidq_bitmask_reset(uidq_bitmask_t *bm);

int  uidq_bitmask_is_empty(const uidq_bitmask_t *bm);

int  uidq_bitmask_bit_test(const uidq_bitmask_t *bm, size_t off);
int  uidq_bitmask_test_sequence(const uidq_bitmask_t *bm, int target, size_t off, size_t len);

int  uidq_bitmask_ffb(const uidq_bitmask_t *bm, int type);
int  uidq_bitmask_fgb(const uidq_bitmask_t *bm, size_t off, size_t len, int type);

bool uidq_bitmask_toggle(uidq_bitmask_t *bm, size_t off, size_t len);

int  uidq_bitmask_push(uidq_bitmask_t *bm, int pos, size_t len);
bool uidq_bitmask_pop(uidq_bitmask_t *bm, size_t start, size_t len);

void uidq_bitmask_invert(uidq_bitmask_t *bm);
void uidq_bitmask_replace(uidq_bitmask_t *bm, size_t start, size_t len, size_t next);
int  uidq_bitmask_op(uidq_bitmask_t *dst, uidq_bitmask_t *src1, uidq_bitmask_t *src2, uidq_bitmask_op_t op);

void uidq_bitmask_debug(const uidq_bitmask_t *bm);

#endif /* UIDQ_BITMASK_INCLUDE_H */
