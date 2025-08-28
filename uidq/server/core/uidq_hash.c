#include "core/uidq_hash.h"
#include "core/uidq_alloc.h"
#include "core/uidq_bitmask.h"
#include "core/uidq_log.h"
#include "core/uidq_pool.h"
#include "core/uidq_core.h"
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define UIDQ_HT_LOAD_FACTOR 5

#define UIDQ_HT_MINIMUM_KEY_SIZE   8
#define UIDQ_HT_MINIMUM_VALUE_SIZE 8
#define UIDQ_HT_MINIMUM_CAPACITY   8
#define UIDQ_HT_MINIMUM_COLL_MULT  1

int uidq_hash_isvalid(uidq_hash_t *hash) {
   return hash && hash->initialized == 1;
}

uidq_hash_conf_t *uidq_hash_conf_get(uidq_hash_t *hash) {
   if (!uidq_hash_isvalid(hash)) return NULL;
   return (uidq_hash_conf_t *)&hash->conf;
}

uidq_hash_t *uidq_hash_create(uidq_hash_conf_t *conf, uidq_log_t *log) {
   if (!conf) {
      uidq_err(log, "Invalid configuration.\n");
      return NULL;
   }

   uidq_hash_t *hash = uidq_calloc(sizeof(uidq_hash_t), log);
   if (!hash) {
      uidq_err(log, "Failed to allocate hash table.\n");
      return NULL;
   }
   hash->initialized = 1;

   if (uidq_hash_init(hash, conf, log) != UIDQ_RET_OK) {
      uidq_err(log, "Failed to initialize hash table.\n");
      uidq_hash_abort(hash);
      return NULL;
   }

   return hash;
}

int uidq_hash_init(uidq_hash_t *hash, uidq_hash_conf_t *conf, uidq_log_t *log) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(log, "Invalid hash table.\n");
      return UIDQ_RET_ERR;
   }
   if (!conf) {
      uidq_err(log, "Invalid configuration.\n");
      return UIDQ_RET_ERR;
   }

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (!config) {
      uidq_err(log, "Failed to get configuration.\n");
      return UIDQ_RET_ERR;
   }

   config->key_size = conf->key_size > 0 ? conf->key_size : UIDQ_HT_MINIMUM_KEY_SIZE;
   config->value_size = conf->value_size > 0 ? conf->value_size : UIDQ_HT_MINIMUM_VALUE_SIZE;
   config->capacity = conf->capacity > 0 ? conf->capacity : UIDQ_HT_MINIMUM_CAPACITY;
   config->coll_mult = conf->coll_mult > 0 ? conf->coll_mult : UIDQ_HT_MINIMUM_COLL_MULT;
   config->func = conf->func ? conf->func : uidq_hash_djb2;

   hash->log = log;

   size_t node_size = sizeof(uidq_hash_node_t) + config->key_size + config->value_size;
   uidq_pool_conf_t pool_conf = {.capacity = config->capacity, .size = node_size};
   hash->buckets = uidq_pool_create(&pool_conf, log);
   if (!hash->buckets) {
      uidq_err(log, "Failed to initialize buckets pool.\n");
      goto fail;
   }

   pool_conf.capacity = config->capacity * config->coll_mult;
   hash->collision = uidq_pool_create(&pool_conf, log);
   if (!hash->collision) {
      uidq_err(log, "Failed to initialize collision pool.\n");
      goto fail;
   }

   hash->pair_size = node_size;
   return UIDQ_RET_OK;

fail:
   uidq_hash_exit(hash);
   return UIDQ_RET_ERR;
}

void uidq_hash_exit(uidq_hash_t *hash) {
   if (!uidq_hash_isvalid(hash)) return;

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (config) {
      config->key_size = 0;
      config->value_size = 0;
      config->capacity = 0;
      config->coll_mult = 0;
      config->func = NULL;
   }

   if (hash->collision) {
      uidq_pool_abort(hash->collision);
      hash->collision = NULL;
   }

   if (hash->buckets) {
      uidq_pool_abort(hash->buckets);
      hash->buckets = NULL;
   }

   hash->log = NULL;
   hash->pair_size = 0;
   hash->threshold = 0;
}

void uidq_hash_abort(uidq_hash_t *hash) {
   if (!hash) return;

   uidq_hash_exit(hash);
   hash->initialized = 0;
   uidq_free(hash, NULL);
}

int uidq_hash_realloc(uidq_hash_t *hash, size_t new_capacity) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(hash->log, "Invalid hash table.\n");
      return UIDQ_RET_ERR;
   }
   if (new_capacity == 0) {
      uidq_err(hash->log, "Invalid new capacity.\n");
      return UIDQ_RET_ERR;
   }

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (!config) {
      uidq_err(hash->log, "Failed to get configuration.\n");
      return UIDQ_RET_ERR;
   }

   size_t old_capacity = config->capacity;
   size_t max_elements = old_capacity + old_capacity * config->coll_mult;

   if (new_capacity == old_capacity) return UIDQ_RET_OK;

   size_t pair_count = 0;

   uidq_pool_conf_t pool_conf = {.capacity = max_elements, .size = hash->pair_size};
   uidq_pool_t *temp_pool = uidq_pool_create(&pool_conf, hash->log);
   if (!temp_pool) {
      uidq_err(hash->log, "Failed to allocate temp pool.\n");
      goto fail;
   }

   for (size_t index = 0; index < old_capacity; index++) {
      if (!uidq_pool_block_check(hash->buckets, index)) continue;
      uidq_hash_node_t *node = uidq_pool_get(hash->buckets, index);
      if (!node) continue;
      list_for_hash_index(hash, node, index) {
         if (pair_count >= max_elements) {
            uidq_err(hash->log, "Too many elements during rehashing.\n");
            goto tp_free;
         }

         int new_idx = uidq_pool_push(temp_pool, NULL);
         if (new_idx < 0) {
            uidq_err(hash->log, "Failed to push to temp pool.\n");
            goto tp_free;
         }

         uidq_hash_node_t *tmp_node = uidq_pool_get(temp_pool, new_idx);
         uidq_hash_conf_t *config = uidq_hash_conf_get(hash);

         memcpy(uidq_node_key(tmp_node), uidq_node_key(node), config->key_size);
         memcpy(uidq_node_value(tmp_node, config->key_size), uidq_node_value(node, config->key_size),
               config->value_size);

         tmp_node->next = NULL;
         tmp_node->index = node->index;

         pair_count++;
      }
   }

   uidq_pool_t *old_buckets = hash->buckets;
   uidq_pool_t *old_collision = hash->collision;

   pool_conf.capacity = new_capacity;
   pool_conf.size = hash->pair_size;
   hash->buckets = uidq_pool_create(&pool_conf, hash->log);
   if (!hash->buckets) {
      uidq_err(hash->log, "Failed to create new buckets pool.\n");
      hash->buckets = old_buckets;
      goto tp_free;
   }

   pool_conf.capacity = new_capacity * config->coll_mult;
   hash->collision = uidq_pool_create(&pool_conf, hash->log);
   if (!hash->collision) {
      uidq_err(hash->log, "Failed to create new collision pool.\n");
      uidq_pool_abort(hash->buckets);
      hash->buckets = old_buckets;
      goto tp_free;
   }

   uidq_pool_abort(old_buckets);
   uidq_pool_abort(old_collision);

   config->capacity = new_capacity;

   for (size_t index = 0; index < pair_count; index++) {
      uidq_hash_node_t *node = uidq_pool_get(temp_pool, index);
      if (!node->index || node->index > new_capacity) {
         continue;
      }

      if (uidq_hash_push(hash, uidq_node_key(node), uidq_node_value(node, config->key_size)) != UIDQ_RET_OK) {
         uidq_err(hash->log, "Failed to push during rehashing.\n");
         goto tp_free;
      }
   }

   uidq_pool_abort(temp_pool);
   return UIDQ_RET_OK;

tp_free:
   config->capacity = old_capacity;
   uidq_pool_abort(temp_pool);
fail:
   return UIDQ_RET_ERR;
}

void uidq_hash_trim(uidq_hash_t *hash, size_t start, size_t end) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(hash->log, "Invalid hash table.\n");
      return;
   }

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (!config) {
      uidq_err(hash->log, "Failed to get configuration.\n");
      return;
   }

   if (start >= config->capacity || end > config->capacity || start > end) {
      uidq_err(hash->log, "Invalid trim range.\n");
      return;
   }

   for (size_t index = start; index < end; index++) {
      uidq_hash_node_t *node = uidq_hash_index(hash, index);
      if (!node || !uidq_node_key(node)) continue;
      uidq_hash_pop_branch(hash, uidq_node_key(node));
   }
}

int uidq_hash_push(uidq_hash_t *hash, void *key, void *value) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(hash->log, "Invalid hash table.\n");
      return UIDQ_RET_ERR;
   }
   if (!key) {
      uidq_err(hash->log, "Invalid key.\n");
      return UIDQ_RET_ERR;
   }

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (!config) {
      uidq_err(hash->log, "Failed to get configuration.\n");
      return UIDQ_RET_ERR;
   }

   uidq_hash_node_t *prev = NULL;
   uidq_hash_node_t *node = uidq_hash_find_node(hash, key, &prev);

   if (node) {
      if (value) {
         memcpy(uidq_node_value(node, config->key_size), value, config->value_size);
      }
      return UIDQ_RET_OK;
   }

   uint8_t normalized[config->key_size];
   int index = uidq_hash_text(hash, key, normalized);

   node = uidq_hash_create_node(hash, normalized, index, prev);
   if (!node) {
      return UIDQ_RET_ERR;
   }

   if (value) memcpy(uidq_node_value(node, config->key_size), value, config->value_size);

   return UIDQ_RET_OK;
}

void uidq_hash_pop(uidq_hash_t *hash, void *key) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(hash->log, "Invalid hash table.\n");
      return;
   }
   if (!key) {
      uidq_err(hash->log, "Invalid key.\n");
      return;
   }

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (!config) {
      uidq_err(hash->log, "Failed to get configuration.\n");
      return;
   }

   uint8_t normalized[config->key_size];
   int index = uidq_hash_text(hash, key, normalized);

   uidq_hash_node_t *prev = NULL;
   uidq_hash_node_t *node = uidq_hash_find_node(hash, key, &prev);

   if (!node) return;

   if (prev) {
      prev->next = node->next;
      uidq_pool_pop(hash->collision, node->index);
   } else {
      uidq_hash_node_t *next = node->next;
      if (next) {
         memcpy(uidq_node_key(node), uidq_node_key(next), config->key_size + config->value_size);
         node->next = next->next;
         uidq_pool_pop(hash->collision, next->index);
      } else {
         uidq_pool_pop(hash->buckets, index);
      }
   }
}

void uidq_hash_pop_branch(uidq_hash_t *hash, void *key) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(hash->log, "Invalid hash table.\n");
      return;
   }
   if (!key) {
      uidq_err(hash->log, "Invalid key.\n");
      return;
   }

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (!config) {
      uidq_err(hash->log, "Failed to get configuration.\n");
      return;
   }

   uint8_t normalized[config->key_size];
   int index = uidq_hash_text(hash, key, normalized);

   uidq_hash_node_t *node = uidq_pool_get(hash->buckets, index);
   if (!uidq_pool_block_check(hash->buckets, index)) return;

   int is_first = 1;
   list_for_hash_index(hash, node, index) {
      if (is_first) {
         uidq_pool_pop(hash->buckets, node->index);
         is_first = 0;
      } else {
         uidq_pool_pop(hash->collision, node->index);
      }
   }
}

uidq_hash_node_t *uidq_hash_get(uidq_hash_t *hash, char *key) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(hash->log, "Invalid hash table.\n");
      return NULL;
   }
   if (!key) {
      uidq_err(hash->log, "Invalid key.\n");
      return NULL;
   }

   return uidq_hash_find_node(hash, key, NULL);
}

uidq_hash_node_t *uidq_hash_find_node(uidq_hash_t *hash, void *key, uidq_hash_node_t **out_prev) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(hash->log, "Invalid hash table.\n");
      return NULL;
   }
   if (!key) {
      uidq_err(hash->log, "Invalid key.\n");
      return NULL;
   }

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (!config) {
      uidq_err(hash->log, "Failed to get configuration.\n");
      return NULL;
   }

   uint8_t normalized[config->key_size];
   int index = uidq_hash_text(hash, key, normalized);

   uidq_hash_node_t *node = uidq_pool_get(hash->buckets, index);
   uidq_hash_node_t *prev = NULL;

   if (uidq_pool_block_check(hash->buckets, index)) {
      list_for_hash_index(hash, node, index) {
         if (memcmp(uidq_node_key(node), normalized, config->key_size) == 0) {
            if (out_prev) *out_prev = prev;
            return node;
         }
         prev = node;
      }
   }

   if (out_prev) *out_prev = prev;
   return NULL;
}

uidq_hash_node_t *uidq_hash_create_node(uidq_hash_t *hash, uint8_t *key, int rindex, uidq_hash_node_t *prev) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(hash->log, "Invalid hash table.\n");
      return NULL;
   }
   if (!key) {
      uidq_err(hash->log, "Invalid key.\n");
      return NULL;
   }

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (!config) {
      uidq_err(hash->log, "Failed to get configuration.\n");
      return NULL;
   }

   uidq_hash_node_t *node;
   int index;

   if (prev) {
      index = uidq_pool_push(hash->collision, NULL);
      if (index < 0) return NULL;
      node = uidq_pool_get(hash->collision, index);
      prev->next = node;
      node->index = index;
   } else {
      index = uidq_pool_pushim(hash->buckets, NULL, rindex);
      if (index != rindex) {
         uidq_pool_pop(hash->buckets, index);
         return NULL;
      }
      node = uidq_pool_get(hash->buckets, index);
      node->index = rindex;
   }

   node->next = NULL;
   memcpy(uidq_node_key(node), key, config->key_size);

   return node;
}

uint8_t *uidq_node_key(uidq_hash_node_t *node) {
   if (!node) return NULL;
   return (uint8_t *)(node + 1);
}

uint8_t *uidq_node_value(uidq_hash_node_t *node, size_t key_size) {
   if (!node) return NULL;
   return uidq_node_key(node) + key_size;
}

uidq_hash_node_t *uidq_hash_index(uidq_hash_t *hash, size_t index) {
   if (!uidq_hash_isvalid(hash)) return NULL;
   if (index >= uidq_hash_conf_get(hash)->capacity) return NULL;
   return uidq_pool_get(hash->buckets, index);
}

void uidq_hash_normalize_key(void *dst, void *src, size_t key_size) {
   if (!dst || !src) return;
   size_t len = strnlen(src, key_size);
   memset(dst, 0, key_size);
   memcpy(dst, src, len < key_size ? len : key_size);
}

size_t uidq_hash_text(uidq_hash_t *hash, void *key, uint8_t *normalized) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(hash->log, "Invalid hash table.\n");
      return 0;
   }
   if (!key || !normalized) {
      uidq_err(hash->log, "Invalid key or normalized buffer.\n");
      return 0;
   }

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (!config) {
      uidq_err(hash->log, "Failed to get configuration.\n");
      return 0;
   }

   size_t len = strnlen(key, config->key_size);
   memset(normalized, 0, config->key_size);
   memcpy(normalized, key, len < config->key_size ? len : config->key_size);

   return config->func(normalized, config->key_size) % config->capacity;
}

uint64_t uidq_hash_djb2(void *raw_key, size_t key_size) {
   if (!raw_key) return 0;
   size_t hash = 5381;
   char *key = raw_key;

   for (size_t byte = 0; byte < key_size; ++byte) {
      hash = ((hash << 5) + hash) ^ key[byte];
   }

   return hash;
}

uint64_t uidq_hash_sdbm(void *raw_key, size_t key_size) {
   if (!raw_key) return 0;
   uint64_t hash = 0;
   unsigned char *key = raw_key;

   for (size_t i = 0; i < key_size; i++) {
      hash = key[i] + (hash << 6) + (hash << 16) - hash;
   }

   return hash;
}

uint64_t uidq_hash_fnv1a( void *raw_key, size_t key_size) {
   if (!raw_key) return 0;
   uint64_t hash = 14695981039346656037ULL;
   unsigned char *key = raw_key;

   for (size_t i = 0; i < key_size; i++) {
      hash ^= key[i];
      hash *= 1099511628211ULL;
   }

   return hash;
}

uint64_t uidq_hash_jenkins( void *raw_key, size_t key_size) {
   if (!raw_key) return 0;
   uint64_t hash = 0;
   unsigned char *key = raw_key;

   for (size_t i = 0; i < key_size; i++) {
      hash += key[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
   }
   hash += (hash << 3);
   hash ^= (hash >> 11);
   hash += (hash << 15);

   return hash;
}

uint64_t uidq_hash_murmur3( void *raw_key, size_t key_size) {
   if (!raw_key) return 0;
   uint64_t seed = 0xc70f6907UL;
   uint64_t m = 0xc6a4a7935bd1e995ULL;
   int r = 47;

   uint64_t hash = seed ^ (key_size * m);
   uint64_t *data = ( uint64_t *)raw_key;
   uint64_t *end = data + (key_size / 8);

   while (data != end) {
      uint64_t k = *data++;
      k *= m; k ^= k >> r; k *= m;
      hash ^= k;
      hash *= m;
   }

   unsigned char *data2 = ( unsigned char *)data;
   switch (key_size & 7) {
      case 7: hash ^= (uint64_t)data2[6] << 48;
      case 6: hash ^= (uint64_t)data2[5] << 40;
      case 5: hash ^= (uint64_t)data2[4] << 32;
      case 4: hash ^= (uint64_t)data2[3] << 24;
      case 3: hash ^= (uint64_t)data2[2] << 16;
      case 2: hash ^= (uint64_t)data2[1] << 8;
      case 1: hash ^= (uint64_t)data2[0]; hash *= m;
   }

   hash ^= hash >> r;
   hash *= m;
   hash ^= hash >> r;

   return hash;
}

void uidq_hash_debug_tree(uidq_hash_t *hash) {
   if (!uidq_hash_isvalid(hash)) {
      uidq_err(hash->log, "Invalid hash table.\n");
      return;
   }

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   if (!config) {
      uidq_err(hash->log, "Failed to get configuration.\n");
      return;
   }

   uidq_debug(hash->log, "=== HASH TABLE DEBUG ===\n");

   for (size_t index = 0; index < config->capacity; ++index) {
      if (!uidq_pool_block_check(hash->buckets, index)) continue;
      uidq_hash_node_t *node = uidq_pool_get(hash->buckets, index);
      if (!node) continue;

      uidq_debug_inline(hash->log, "Bucket %zu:\n", index);
      list_for_hash_index(hash, node, index) {
         uidq_debug_inline(hash->log, "  Key: '%.*s', Value: '%.*s'\n",
               (int)config->key_size, uidq_node_key(node),
               (int)config->value_size, uidq_node_value(node, config->key_size));
      }
   }

   uidq_debug_inline(hash->log, "\n");
}
