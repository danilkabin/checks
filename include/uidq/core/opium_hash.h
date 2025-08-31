#ifndef OPIUM_HASH_INCLUDE_H
#define OPIUM_HASH_INCLUDE_H

#include "core/opium_pool.h"
#include "opium_list.h"
#include "opium_log.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef uint64_t (*opium_hash_func_t)(void *key, size_t len);

typedef struct {
   size_t key_size;
   size_t value_size;
   size_t capacity;
   size_t coll_mult;
   opium_hash_func_t func;
} opium_hash_conf_t;

typedef struct opium_hash_node_t {
   struct opium_hash_node_t *next;
   size_t index;
} opium_hash_node_t;

typedef struct {
   int initialized;
   size_t threshold;
   size_t pair_size;

   opium_hash_conf_t conf;

   opium_pool_t *buckets;
   opium_pool_t *collision;

   opium_log_t *log;
} opium_hash_t;

/* Validity Checks */
int opium_hash_isvalid(opium_hash_t *hash);

/* State Checks */
opium_hash_conf_t *opium_hash_conf_get(opium_hash_t *hash);
opium_hash_node_t *opium_hash_get(opium_hash_t *hash,  char *key);

/* Creation and Destruction */
opium_hash_t *opium_hash_create( opium_hash_conf_t *conf, opium_log_t *log);
void          opium_hash_abort(opium_hash_t *hash);
int           opium_hash_init(opium_hash_t *hash,  opium_hash_conf_t *conf, opium_log_t *log);
void          opium_hash_exit(opium_hash_t *hash);
int           opium_hash_realloc(opium_hash_t *hash, size_t new_capacity);

/* Hash Operations */
int  opium_hash_push(opium_hash_t *hash,  void *key,  void *value);
void opium_hash_pop(opium_hash_t *hash,  void *key);
void opium_hash_pop_branch(opium_hash_t *hash,  void *key);

/* Node Operations */
opium_hash_node_t *opium_hash_find_node(opium_hash_t *hash,  void *key, opium_hash_node_t **out_prev);
opium_hash_node_t *opium_hash_create_node(opium_hash_t *hash,  uint8_t *key, int rindex, opium_hash_node_t *prev);

/* Debugging */
void opium_hash_debug_tree(opium_hash_t *hash);

/* Hash Functions */
uint64_t opium_hash_djb2(void *raw_key, size_t key_size);
uint64_t opium_hash_sdbm(void *raw_key, size_t key_size);
uint64_t opium_hash_fnv1a( void *raw_key, size_t key_size);
uint64_t opium_hash_jenkins( void *raw_key, size_t key_size);
uint64_t opium_hash_murmur3( void *raw_key, size_t key_size);

#define list_for_hash_index(hash, node, index) \
   for (opium_hash_node_t *node = opium_pool_get((hash)->buckets, (index)); \
         node; \
         node = node->next)

#define list_for_hash(hash, node) \
   for (size_t index = 0; \
         opium_hash_isvalid(hash) && index < opium_hash_conf_get(hash)->capacity; \
         index++) \
for (opium_hash_node_t *node = opium_pool_get((hash)->buckets, index); \
      node; \
      node = node->next)

#endif /* OPIUM_HASH_INCLUDE_H */
