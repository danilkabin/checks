#include "core/opium_hash.h"
#include "core/opium_alloc.h"
#include "core/opium_bitmask.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include "core/opium_core.h"
#include "core/opium_hashfunc.h"

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define OPIUM_HT_MINIMUM_KEY_SIZE   8
#define OPIUM_HT_MINIMUM_VALUE_SIZE 8
#define OPIUM_HT_MINIMUM_CAPACITY   8
#define OPIUM_HT_MINIMUM_COLL_MULT  1

/* Statics */
static opium_hash_node_t *opium_hash_index(opium_hash_t *hash, size_t index);
static void opium_hash_normalize_key(void *dst, void *src, size_t key_size);
static size_t opium_hash_text(opium_hash_t *hash, void *key, uint8_t *normalized);

static opium_hash_node_t *opium_hash_find_node(opium_hash_t *hash, void *key, opium_hash_node_t **out_prev); 
static opium_hash_node_t *opium_hash_create_node(opium_hash_t *hash, uint8_t *key, int rindex, opium_hash_node_t *prev); 


/* Validity Checks */
   int
opium_hash_isvalid(opium_hash_t *hash) 
{
   return hash && hash->initialized == 1;
}

/* State Checks */
   static uint8_t *
opium_node_key(opium_hash_node_t *node)
{
   if (!node) {
      return NULL;
   }
   return (uint8_t *)(node + 1);
}

   static uint8_t *
opium_node_value(opium_hash_node_t *node, size_t key_size)
{
   if (!node) {
      return NULL;
   }
   return opium_node_key(node) + key_size;
}

   opium_hash_conf_t *
opium_hash_conf_get(opium_hash_t *hash) 
{
   if (!opium_hash_isvalid(hash)) return NULL;
   return (opium_hash_conf_t *)&hash->conf;
}

   opium_hash_node_t *
opium_hash_get(opium_hash_t *hash, char *key) 
{
   if (!opium_hash_isvalid(hash)) {
      return NULL;
   }

   if (!key) {
      opium_err(hash->log, "Invalid key.\n");
      return NULL;
   }

   return opium_hash_find_node(hash, key, NULL);
}

/* Creation and Destruction */
   opium_hash_t *
opium_hash_create(opium_hash_conf_t *conf, opium_log_t *log)
{
   if (!conf) {
      opium_err(log, "Invalid configuration.\n");
      return NULL;
   }

   opium_hash_t *hash = opium_calloc(sizeof(opium_hash_t), log);
   if (!hash) {
      opium_err(log, "Failed to allocate hash table.\n");
      return NULL;
   }
   hash->initialized = 1;

   if (opium_hash_init(hash, conf, log) != OPIUM_RET_OK) {
      opium_err(log, "Failed to initialize hash table.\n");
      opium_hash_abort(hash);
      return NULL;
   }

   return hash;
}

   void 
opium_hash_abort(opium_hash_t *hash) 
{
   if (!opium_hash_isvalid(hash)) {
      return;
   }

   opium_hash_exit(hash);
   hash->initialized = 0;
   opium_free(hash, NULL);
}

   int 
opium_hash_init(opium_hash_t *hash, opium_hash_conf_t *conf, opium_log_t *log)
{
   if (!opium_hash_isvalid(hash)) {
      return OPIUM_RET_ERR;
   }

   if (!conf) {
      opium_err(log, "Invalid configuration.\n");
      return OPIUM_RET_ERR;
   }

   opium_hash_conf_t *config = opium_hash_conf_get(hash);

   config->key_size = conf->key_size > 0 ? conf->key_size : OPIUM_HT_MINIMUM_KEY_SIZE;
   config->value_size = conf->value_size > 0 ? conf->value_size : OPIUM_HT_MINIMUM_VALUE_SIZE;
   config->capacity = conf->capacity > 0 ? conf->capacity : OPIUM_HT_MINIMUM_CAPACITY;
   config->coll_mult = conf->coll_mult > 0 ? conf->coll_mult : OPIUM_HT_MINIMUM_COLL_MULT;
   config->func = conf->func ? conf->func : opium_hash_djb2;

   hash->log = log;

   size_t node_size = sizeof(opium_hash_node_t) + config->key_size + config->value_size;
   hash->buckets = opium_pool_create(config->capacity, node_size, log);
   if (!hash->buckets) {
      opium_err(log, "Failed to initialize buckets pool.\n");
      goto fail;
   }

   hash->collision = opium_pool_create(config->capacity * config->coll_mult, node_size, log);
   if (!hash->collision) {
      opium_err(log, "Failed to initialize collision pool.\n");
      goto fail;
   }

   hash->pair_size = node_size;
   return OPIUM_RET_OK;

fail:
   opium_hash_exit(hash);
   return OPIUM_RET_ERR;
}

   void 
opium_hash_exit(opium_hash_t *hash)
{
   if (!opium_hash_isvalid(hash)) {
      return;
   }

   opium_hash_conf_t *config = opium_hash_conf_get(hash);

   config->key_size = 0;
   config->value_size = 0;
   config->capacity = 0;
   config->coll_mult = 0;
   config->func = NULL;

   if (hash->collision) {
      opium_pool_abort(hash->collision);
      hash->collision = NULL;
   }

   if (hash->buckets) {
      opium_pool_abort(hash->buckets);
      hash->buckets = NULL;
   }

   hash->log = NULL;
   hash->pair_size = 0;
   hash->threshold = 0;
}

   int 
opium_hash_realloc(opium_hash_t *hash, size_t new_capacity)
{
   if (!opium_hash_isvalid(hash)) {
      return OPIUM_RET_ERR;
   }

   if (new_capacity == 0) {
      opium_err(hash->log, "Invalid new capacity.\n");
      return OPIUM_RET_ERR;
   }

   opium_hash_conf_t *config = opium_hash_conf_get(hash);

   size_t old_capacity = config->capacity;
   size_t max_elements = old_capacity + old_capacity * config->coll_mult;

   if (new_capacity == old_capacity) return OPIUM_RET_OK;

   size_t pair_count = 0;

   opium_pool_t *temp_pool = opium_pool_create(max_elements, hash->pair_size, hash->log);
   if (!temp_pool) {
      opium_err(hash->log, "Failed to allocate temp pool.\n");
      goto fail;
   }

   for (size_t index = 0; index < old_capacity; index++) {
      if (!opium_pool_block_check(hash->buckets, index)) continue;
      opium_hash_node_t *node = opium_pool_get(hash->buckets, index);
      if (!node) continue;
      list_for_hash_index(hash, node, index) {
         if (pair_count >= max_elements) {
            opium_err(hash->log, "Too many elements during rehashing.\n");
            goto tp_free;
         }

         int new_idx = opium_pool_push(temp_pool, NULL);
         if (new_idx < 0) {
            opium_err(hash->log, "Failed to push to temp pool.\n");
            goto tp_free;
         }

         opium_hash_node_t *tmp_node = opium_pool_get(temp_pool, new_idx);
         opium_hash_conf_t *config = opium_hash_conf_get(hash);

         memcpy(opium_node_key(tmp_node), opium_node_key(node), config->key_size);
         memcpy(opium_node_value(tmp_node, config->key_size), opium_node_value(node, config->key_size),
               config->value_size);

         tmp_node->next = NULL;
         tmp_node->index = node->index;

         pair_count++;
      }
   }

   opium_pool_t *old_buckets = hash->buckets;
   opium_pool_t *old_collision = hash->collision;

   hash->buckets = opium_pool_create(new_capacity, hash->pair_size, hash->log);
   if (!hash->buckets) {
      opium_err(hash->log, "Failed to create new buckets pool.\n");
      hash->buckets = old_buckets;
      goto tp_free;
   }

   hash->collision = opium_pool_create(new_capacity *config->coll_mult, hash->pair_size, hash->log);
   if (!hash->collision) {
      opium_err(hash->log, "Failed to create new collision pool.\n");
      opium_pool_abort(hash->buckets);
      hash->buckets = old_buckets;
      goto tp_free;
   }

   opium_pool_abort(old_buckets);
   opium_pool_abort(old_collision);

   config->capacity = new_capacity;

   for (size_t index = 0; index < pair_count; index++) {
      opium_hash_node_t *node = opium_pool_get(temp_pool, index);
      if (!node->index || node->index > new_capacity) {
         continue;
      }

      if (opium_hash_push(hash, opium_node_key(node), opium_node_value(node, config->key_size)) != OPIUM_RET_OK) {
         opium_err(hash->log, "Failed to push during rehashing.\n");
         goto tp_free;
      }
   }

   opium_pool_abort(temp_pool);
   return OPIUM_RET_OK;

tp_free:
   config->capacity = old_capacity;
   opium_pool_abort(temp_pool);
fail:
   return OPIUM_RET_ERR;
}

/* Hash Operations */
   int
opium_hash_push(opium_hash_t *hash, void *key, void *value) 
{
   if (!opium_hash_isvalid(hash)) {
      return OPIUM_RET_ERR;
   }

   if (!key) {
      opium_err(hash->log, "Invalid key.\n");
      return OPIUM_RET_ERR;
   }

   opium_hash_conf_t *config = opium_hash_conf_get(hash);

   opium_hash_node_t *prev = NULL;
   opium_hash_node_t *node = opium_hash_find_node(hash, key, &prev);

   if (node) {
      if (value) {
         memcpy(opium_node_value(node, config->key_size), value, config->value_size);
      }
      return OPIUM_RET_OK;
   }

   uint8_t normalized[config->key_size];
   int index = opium_hash_text(hash, key, normalized);

   node = opium_hash_create_node(hash, normalized, index, prev);
   if (!node) {
      return OPIUM_RET_ERR;
   }

   if (value) {
      memcpy(opium_node_value(node, config->key_size), value, config->value_size);
   }



   return OPIUM_RET_OK;
}

void opium_hash_pop(opium_hash_t *hash, void *key) {
   if (!opium_hash_isvalid(hash)) {
      return;
   }

   if (!key) {
      opium_err(hash->log, "Invalid key.\n");
      return;
   }

   opium_hash_conf_t *config = opium_hash_conf_get(hash);

   uint8_t normalized[config->key_size];
   int index = opium_hash_text(hash, key, normalized);

   opium_hash_node_t *prev = NULL;
   opium_hash_node_t *node = opium_hash_find_node(hash, key, &prev);

   if (!node) return;

   if (prev) {
      prev->next = node->next;
      opium_pool_pop(hash->collision, node->index);
   } else {
      opium_hash_node_t *next = node->next;
      if (next) {
         memcpy(opium_node_key(node), opium_node_key(next), config->key_size + config->value_size);
         node->next = next->next;
         opium_pool_pop(hash->collision, next->index);
      } else {
         opium_pool_pop(hash->buckets, index);
      }
   }
}

void opium_hash_pop_branch(opium_hash_t *hash, void *key) {
   if (!opium_hash_isvalid(hash)) {
      return;
   }

   if (!key) {
      opium_err(hash->log, "Invalid key.\n");
      return;
   }

   opium_hash_conf_t *config = opium_hash_conf_get(hash);

   uint8_t normalized[config->key_size];
   int index = opium_hash_text(hash, key, normalized);

   if (!opium_pool_block_check(hash->buckets, index)) {
      return;
   }

   int is_first = 1;
   list_for_hash_index(hash, node, index) {
      if (is_first) {
         opium_pool_pop(hash->buckets, node->index);
         is_first = 0;
      } else {
         opium_pool_pop(hash->collision, node->index);
      }
   }
}

/* Debugging */
   void
opium_hash_debug_tree(opium_hash_t *hash) 
{
   if (!opium_hash_isvalid(hash)) {
      return;
   }

   opium_hash_conf_t *config = opium_hash_conf_get(hash);

   opium_debug(hash->log, "=== HASH TABLE DEBUG ===\n");

   for (size_t index = 0; index < config->capacity; ++index) {
      if (!opium_pool_block_check(hash->buckets, index)) continue;
      opium_hash_node_t *node = opium_pool_get(hash->buckets, index);
      if (!node) continue;

      opium_debug_inline(hash->log, "Bucket %zu:\n", index);
      list_for_hash_index(hash, node, index) {
         opium_debug_inline(hash->log, "  Key: '%.*s', Value: '%.*s'\n",
               (int)config->key_size, opium_node_key(node),
               (int)config->value_size, opium_node_value(node, config->key_size));
      }
   }

   opium_debug_inline(hash->log, "\n");
}

/* Statics */
static opium_hash_node_t *
opium_hash_find_node(opium_hash_t *hash, void *key, opium_hash_node_t **out_prev) 
{
   if (!opium_hash_isvalid(hash)) {
      return NULL;
   }

   if (!key) {
      opium_err(hash->log, "Invalid key.\n");
      return NULL;
   }

   opium_hash_conf_t *config = opium_hash_conf_get(hash);

   uint8_t normalized[config->key_size];
   int index = opium_hash_text(hash, key, normalized);

   opium_hash_node_t *prev = NULL;

   if (opium_pool_block_check(hash->buckets, index)) {
      list_for_hash_index(hash, node, index) {
         if (memcmp(opium_node_key(node), normalized, config->key_size) == 0) {
            if (out_prev) *out_prev = prev;
            return node;
         }
         prev = node;
      }
   }

   if (out_prev) {
      *out_prev = prev;
   }
   return NULL;
}

static opium_hash_node_t *
opium_hash_create_node(opium_hash_t *hash, uint8_t *key, int rindex, opium_hash_node_t *prev) 
{
   if (!opium_hash_isvalid(hash)) {
      return NULL;
   }

   if (!key) {
      opium_err(hash->log, "Invalid key.\n");
      return NULL;
   }

   opium_hash_conf_t *config = opium_hash_conf_get(hash);

   opium_hash_node_t *node;
   int index;

   if (prev) {
      index = opium_pool_push(hash->collision, NULL);
      if (index < 0) return NULL;
      node = opium_pool_get(hash->collision, index);
      prev->next = node;
      node->index = index;
   } else {
      index = opium_pool_pushim(hash->buckets, NULL, rindex);
      if (index != rindex) {
         opium_pool_pop(hash->buckets, index);
         return NULL;
      }
      node = opium_pool_get(hash->buckets, index);
      node->index = rindex;
   }

   node->next = NULL;
   memcpy(opium_node_key(node), key, config->key_size);

   return node;
}

   static opium_hash_node_t *
opium_hash_index(opium_hash_t *hash, size_t index) 
{
   if (!opium_hash_isvalid(hash)) {
      return NULL;
   }
   if (index >= opium_hash_conf_get(hash)->capacity) {
      return NULL;
   }
   return opium_pool_get(hash->buckets, index);
}

   static void
opium_hash_normalize_key(void *dst, void *src, size_t key_size)
{
   if (!dst || !src || key_size == 0) {
      return;
   }

   size_t len = strnlen(src, key_size);
   memset(dst, 0, key_size);
   memcpy(dst, src, len < key_size ? len : key_size);
}

   static size_t 
opium_hash_text(opium_hash_t *hash, void *key, uint8_t *normalized) 
{
   if (!opium_hash_isvalid(hash)) {
      return 0;
   }

   if (!key || !normalized) {
      opium_err(hash->log, "Invalid key or normalized buffer.\n");
      return 0;
   }

   opium_hash_conf_t *config = opium_hash_conf_get(hash);

   size_t len = strnlen(key, config->key_size);
   memset(normalized, 0, config->key_size);
   memcpy(normalized, key, len < config->key_size ? len : config->key_size);

   return config->func(normalized, config->key_size) % config->capacity;
}
