#ifndef OPIUM_HASH_INCLUDE_H
#define OPIUM_HASH_INCLUDE_H

#include "core/opium_core.h"

typedef uint64_t (*opium_hash_func_t)(void *key, size_t len);

typedef struct opium_hash_node_t {
   struct opium_hash_node_t *next;
   size_t index;
} opium_hash_node_t;

struct opium_hash_s {
   size_t nelts;
   size_t key_size, value_size;
   size_t pair_size;

   opium_slab_t buckets;

   opium_hash_func_t func;

   opium_log_t *log;
};

int opium_hash_init(opium_hash_t *hash, size_t nelts, size_t ks, size_t vs, opium_log_t *log);
int opium_hash_insert(opium_hash_t *hash, void *key, void *value);

#endif /* OPIUM_HASH_INCLUDE_H */
