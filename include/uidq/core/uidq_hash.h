#ifndef UIDQ_HASH_INCLUDE_H
#define UIDQ_HASH_INCLUDE_H

#include "core/uidq_pool.h"
#include "uidq_list.h"
#include "uidq_log.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef uint64_t (*uidq_hash_func_t)(void *key, size_t len);

typedef struct {
   size_t key_size;
   size_t value_size;
   size_t capacity;
   size_t coll_mult;
   uidq_hash_func_t func;
} uidq_hash_conf_t;

typedef struct uidq_hash_node_t {
   struct uidq_hash_node_t *next;
   size_t index;
} uidq_hash_node_t;

typedef struct {
   int initialized;
   size_t threshold;
   size_t pair_size;

   uidq_hash_conf_t conf;

   uidq_pool_t *buckets;
   uidq_pool_t *collision;

   uidq_log_t *log;
} uidq_hash_t;

uidq_hash_t *uidq_hash_create( uidq_hash_conf_t *conf, uidq_log_t *log);
void uidq_hash_abort(uidq_hash_t *hash);
int uidq_hash_init(uidq_hash_t *hash,  uidq_hash_conf_t *conf, uidq_log_t *log);
void uidq_hash_exit(uidq_hash_t *hash);
int uidq_hash_realloc(uidq_hash_t *hash, size_t new_capacity);
void uidq_hash_trim(uidq_hash_t *hash, size_t start, size_t end);

int uidq_hash_push(uidq_hash_t *hash,  void *key,  void *value);
void uidq_hash_pop(uidq_hash_t *hash,  void *key);
void uidq_hash_pop_branch(uidq_hash_t *hash,  void *key);
uidq_hash_node_t *uidq_hash_get(uidq_hash_t *hash,  char *key);
uidq_hash_node_t *uidq_hash_find_node(uidq_hash_t *hash,  void *key, uidq_hash_node_t **out_prev);

uidq_hash_node_t *uidq_hash_create_node(uidq_hash_t *hash,  uint8_t *key, int rindex, uidq_hash_node_t *prev);
uint8_t *uidq_node_key( uidq_hash_node_t *node);
uint8_t *uidq_node_value( uidq_hash_node_t *node, size_t key_size);
uidq_hash_node_t *uidq_hash_index( uidq_hash_t *hash, size_t index);

uidq_hash_conf_t *uidq_hash_conf_get(uidq_hash_t *hash);
int uidq_hash_isvalid(uidq_hash_t *hash);

void uidq_hash_normalize_key(void *dst, void *src, size_t key_size);
size_t uidq_hash_text(uidq_hash_t *hash, void *key, uint8_t *normalized);
uint64_t uidq_hash_djb2(void *raw_key, size_t key_size);
uint64_t uidq_hash_sdbm(void *raw_key, size_t key_size);
uint64_t uidq_hash_fnv1a(void *raw_key, size_t key_size);
uint64_t uidq_hash_jenkins(void *raw_key, size_t key_size);
uint64_t uidq_hash_murmur3(void *raw_key, size_t key_size);

void uidq_hash_debug_tree(uidq_hash_t *hash);

#define list_for_hash_index(hash, node, index) \
   for (uidq_hash_node_t *node = uidq_pool_get((hash)->buckets, (index)); \
         node; \
         node = node->next)

#define list_for_hash(hash, node) \
   for (size_t index = 0; \
         uidq_hash_isvalid(hash) && index < uidq_hash_conf_get(hash)->capacity; \
         index++) \
for (uidq_hash_node_t *node = uidq_pool_get((hash)->buckets, index); \
      node; \
      node = node->next)

#endif /* UIDQ_HASH_INCLUDE_H */
