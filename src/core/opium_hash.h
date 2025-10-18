#ifndef OPIUM_HASH_INCLUDE_H
#define OPIUM_HASH_INCLUDE_H

#include "core/opium_core.h"

typedef uint64_t (*opium_hash_func_t)(void *key, size_t len);

typedef struct opium_hash_elt_t {
   opium_list_head_t head;
   opium_uint_t      key_hash;
} opium_hash_elt_t;

struct opium_hash_s {
   size_t nelts;
   size_t key_size, value_size;
   size_t pair_size;

   opium_pool_t elts;
   opium_slab_t outcasts;

   opium_hash_func_t func;

   opium_log_t *log;
};

opium_ubyte_t *opium_hash_elt_key(opium_hash_elt_t *elt);
opium_ubyte_t *opium_hash_elt_value(opium_hash_elt_t *elt, size_t key_size);

int opium_hash_init(opium_hash_t *hash, size_t nelts, size_t ks, size_t vs, opium_log_t *log);
void opium_hash_exit(opium_hash_t *hash);

int opium_hash_insert(opium_hash_t *hash, void *key, void *value);
void opium_hash_delete(opium_hash_t *hash, void *key);

opium_hash_elt_t *opium_hash_find(opium_hash_t *hash, void *key);

void opium_hash_debug(opium_hash_t *hash);

#endif /* OPIUM_HASH_INCLUDE_H */
