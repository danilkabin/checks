#ifndef OPIUM_BITMASK_INCLUDE_H
#define OPIUM_BITMASK_INCLUDE_H

#include "core/opium_log.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
   OPIUM_BITMASK_OP_AND,
   OPIUM_BITMASK_OP_OR,
   OPIUM_BITMASK_OP_XOR,
   OPIUM_BITMASK_OP_NAND,
   OPIUM_BITMASK_OP_NOR,
   OPIUM_BITMASK_OP_XNOR
} opium_bitmask_op_t;

typedef struct {
   bool initialized;

   size_t count;
   size_t capacity;
   size_t allocated_capacity;
   size_t word_capacity;

   uint64_t *mask;

   opium_log_t *log;
} opium_bitmask_t;

/* Validity Checks */
int opium_bitmask_isvalid(opium_bitmask_t *bitmask);
int opium_bitmask_isvalid_index(opium_bitmask_t *bitmask, size_t index);

/* State Checks */
int    opium_bitmask_is_empty(opium_bitmask_t *bitmask);
int    opium_bitmask_is_full(opium_bitmask_t *bitmask);
size_t opium_bitmask_get_capacity(opium_bitmask_t *bitmask);
void   opium_bitmask_lim_capacity(opium_bitmask_t *bitmask, size_t new_capacity);

/* Creation and Destruction */
opium_bitmask_t *opium_bitmask_create(size_t capacity, opium_log_t *log);
void             opium_bitmask_abort(opium_bitmask_t *bitmask);
int              opium_bitmask_init(opium_bitmask_t *bitmask, size_t capacity, opium_log_t *log);
void             opium_bitmask_exit(opium_bitmask_t *bitmask);
int              opium_bitmask_realloc(opium_bitmask_t *bitmask, size_t new_capacity);

/* Memory and State Management */
//void opium_bitmask_replace(opium_bitmask_t *bitmask, size_t start, size_t len, size_t next);
void opium_bitmask_invert(opium_bitmask_t *bitmask);
int  opium_bitmask_copy(opium_bitmask_t **dst, opium_bitmask_t *src);
void opium_bitmask_reset(opium_bitmask_t *bitmask);

/* Bitwise Operations */
int opium_bitmask_op(opium_bitmask_t *dst, opium_bitmask_t *src1, opium_bitmask_t *src2, opium_bitmask_op_t op);

/* Bit Operations */
int  opium_bitmask_bit_test(opium_bitmask_t *bitmask, size_t off);
int  opium_bitmask_ffb(opium_bitmask_t *bitmask, int type);
int  opium_bitmask_fgb(opium_bitmask_t *bitmask, size_t off, size_t len, int type);
bool opium_bitmask_toggle(opium_bitmask_t *bitmask, size_t off, size_t len);
int  opium_bitmask_push(opium_bitmask_t *bitmask, int pos, size_t len);
bool opium_bitmask_pop(opium_bitmask_t *bitmask, int start, size_t len);

/* Debugging */
void opium_bitmask_debug(opium_bitmask_t *bitmask);

#endif /* OPIUM_BITMASK_INCLUDE_H */
