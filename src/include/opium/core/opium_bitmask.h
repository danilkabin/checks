#ifndef OPIUM_BITMASK_INCLUDE_H
#define OPIUM_BITMASK_INCLUDE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "core/opium_log.h"

typedef struct {
   size_t count;
   size_t capacity;
   size_t word_capacity;
   
   uint64_t *mask;

   opium_log_t *log;
} opium_bitmask_t;

int opium_bitmask_isvalid(opium_bitmask_t *bitmask);
int opium_bitmask_isvalid_index(opium_bitmask_t *bitmask, size_t index);
int opium_bitmask_is_empty(opium_bitmask_t *bitmask);
int opium_bitmask_is_full(opium_bitmask_t *bitmask);
int opium_bitmask_count(opium_bitmask_t *bitmask);

opium_bitmask_t *opium_bitmask_create(size_t capacity, opium_log_t *log);
void opium_bitmask_destroy(opium_bitmask_t *bitmask);
int opium_bitmask_realloc(opium_bitmask_t *bitmask, size_t new_capacity);

void opium_bitmask_invert(opium_bitmask_t *bitmask);
void opium_bitmask_reset(opium_bitmask_t *bitmask);
int opium_bitmask_copy(opium_bitmask_t **dst, opium_bitmask_t *src);

int opium_bitmask_toggle(opium_bitmask_t *bitmask, size_t pos, size_t len);
int opium_bitmask_push(opium_bitmask_t *bitmask, int pos, size_t len);
void opium_bitmask_pop(opium_bitmask_t *bitmask, int pos, size_t len);

int opium_bitmask_bit_test(opium_bitmask_t *bitmask, size_t off);

void opium_bitmask_debug(opium_bitmask_t *bitmask);

#endif /* OPIUM_BITMASK_INCLUDE_H */
