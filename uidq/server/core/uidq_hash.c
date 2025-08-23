#include "core/uidq_hash.h"
#include "core/uidq_alloc.h"
#include "core/uidq_log.h"
#include "core/uidq_pool.h"
#include "core/uidq_types.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define UIDQ_HT_LOAD_FACTOR 5

#define UIDQ_HT_MINIMUM_KEY_SIZE   8
#define UIDQ_HT_MINIMUM_VALUE_SIZE 8
#define UIDQ_HT_MINIMUM_CAPACITY   8
#define UIDQ_HT_MINIMUM_COLL_MULT  1

   int
uidq_hash_isvalid(uidq_hash_t *hash) 
{
   return hash && hash->initialized == 1;
}

uidq_hash_conf_t *
uidq_hash_conf_get(uidq_hash_t *hash) {
   return &hash->conf;
}

   uidq_hash_t *
uidq_hash_create(uidq_hash_conf_t *conf, uidq_log_t *log) 
{
   uidq_hash_t *hash = uidq_calloc(sizeof(uidq_hash_t), log);
   if (!hash) {
      uidq_err(log, "Failed to allocate hash table.\n"); 
      return NULL;
   }
   hash->initialized = 1;

   if (uidq_hash_init(hash, conf, log) != UIDQ_RET_OK) {
      uidq_err(log, "Failed to init hash table.\n"); 
      uidq_hash_abort(hash);
      return NULL;
   }

   return hash;
}

   void
uidq_hash_abort(uidq_hash_t *hash) 
{
   if (!uidq_hash_isvalid(hash)) return;

   uidq_hash_exit(hash);
   uidq_free(hash, NULL);
}

   int 
uidq_hash_init(uidq_hash_t *hash, uidq_hash_conf_t *conf, uidq_log_t *log) 
{
   if (!uidq_hash_isvalid(hash)) return UIDQ_RET_ERR;
   if (!conf) return UIDQ_RET_ERR;

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);

   config->key_size = conf->key_size > 0 ? conf->key_size : UIDQ_HT_MINIMUM_KEY_SIZE;
   config->value_size = conf->value_size > 0 ? conf->value_size : UIDQ_HT_MINIMUM_VALUE_SIZE;
   config->capacity = conf->capacity > 0 ? conf->capacity : UIDQ_HT_MINIMUM_CAPACITY;
   config->coll_mult = conf->coll_mult > 0 ? conf->coll_mult : UIDQ_HT_MINIMUM_COLL_MULT;
   config->func = conf->func ? conf->func : uidq_hash_djb2;

   hash->log = log;

   size_t node_size = sizeof(uidq_hash_node_t) + config->key_size + config->value_size;
   hash->buckets = uidq_pool_create(config->capacity, node_size, log);
   if (!hash->buckets) {
      uidq_err(log, "Failed to init buckets in hash table.\n"); 
      goto fail;
   }

   hash->collision = uidq_pool_create(config->capacity * config->coll_mult, node_size, log); 
   if (!hash->collision) {
      uidq_err(log, "Failed to init collision in hash table.\n"); 
      goto fail;
   }

   return UIDQ_RET_OK;

fail:
   uidq_hash_exit(hash);
   return UIDQ_RET_ERR;
}

   void
uidq_hash_exit(uidq_hash_t *hash) 
{
   if (!uidq_hash_isvalid(hash)) return;

   if (hash->collision) {
      uidq_pool_abort(hash->collision);
      hash->collision = NULL;
   }

   if (hash->buckets) {
      uidq_pool_abort(hash->buckets);
      hash->buckets = NULL;
   }
}

   uint8_t *
uidq_node_key(uidq_hash_node_t* node) 
{
   return (uint8_t*)(node + 1); 
}

   uint8_t *
uidq_node_value(uidq_hash_node_t* node, size_t key_size) 
{
   return uidq_node_key(node) + key_size;
}

   size_t
uidq_hash_text(uidq_hash_t *hash, void *key)
{
   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   return config->func(key, config->key_size) % (config->capacity - 1);
}

   uint64_t 
uidq_hash_djb2(void *raw_key, size_t key_size) 
{
   size_t hash = 5381;
   char *key = raw_key;

   for (size_t byte = 0; byte < key_size; ++byte) {
      hash = ((hash << 5) + hash) ^ key[byte];
   }

   return hash;
}

   uint64_t
uidq_hash_sdbm(void *raw_key, size_t key_size) 
{
   uint64_t hash = 0;
   unsigned char *key = raw_key;
   for (size_t i = 0; i < key_size; i++) {
      hash = key[i] + (hash << 6) + (hash << 16) - hash;
   }
   return hash;
}

   uint64_t
uidq_hash_fnv1a(void *raw_key, size_t key_size) 
{
   uint64_t hash = 14695981039346656037ULL;
   unsigned char *key = raw_key;
   for (size_t i = 0; i < key_size; i++) {
      hash ^= key[i];
      hash *= 1099511628211ULL;
   }
   return hash;
}

   uint64_t
uidq_hash_jenkins(void *raw_key, size_t key_size) 
{
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

uint64_t uidq_hash_murmur3(void *raw_key, size_t key_size) {
   const uint64_t seed = 0xc70f6907UL;
   const uint64_t m = 0xc6a4a7935bd1e995ULL;
   const int r = 47;

   uint64_t hash = seed ^ (key_size * m);
   const uint64_t *data = (uint64_t *)raw_key;
   const uint64_t *end = data + (key_size / 8);

   while (data != end) {
      uint64_t k = *data++;
      k *= m; k ^= k >> r; k *= m;
      hash ^= k;
      hash *= m;
   }

   const unsigned char *data2 = (const unsigned char*)data;
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

uidq_hash_node_t *
uidq_hash_find_node(uidq_hash_t *hash, void *key, uidq_hash_node_t **out_prev) {
   if (!uidq_hash_isvalid(hash) || !key) return NULL;
   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);

   int index = uidq_hash_text(hash, key);

   uidq_hash_node_t *node = uidq_pool_get(hash->buckets, index);
   uidq_hash_node_t *prev = NULL;

   if (uidq_pool_block_check(hash->buckets, index)) {
      while (node) {
         if (memcmp(uidq_node_key(node), key, config->key_size) == 0) {
            if (out_prev) *out_prev = prev;
            return node;
         }
         prev = node;
         node = node->next;
      }
   }

   if (out_prev) *out_prev = prev;
   return NULL;
}

   uidq_hash_node_t *
uidq_hash_get(uidq_hash_t *hash, char *key) 
{
   return uidq_hash_find_node(hash, key, NULL);
}

   uidq_hash_node_t *
uidq_hash_create_node(uidq_hash_t *hash, char *key, int rindex, uidq_hash_node_t *prev) 
{
   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   uidq_hash_node_t *node;
   int index;
   if (prev) {
      index = uidq_pool_push(hash->collision, NULL);
      if (index < 0) { printf("hash collision pool full\n"); return NULL; }
      node = uidq_pool_get(hash->collision, index);
      prev->next = node;
   } else {
      index = uidq_pool_pushim(hash->buckets, NULL, rindex);
      if (index < 0) { printf("hash bucket full\n"); return NULL; }
      node = uidq_pool_get(hash->buckets, index);
   }

   node->index = index;

   memcpy(uidq_node_key(node), key, config->key_size);
   printf("key: %s, hash key: %s\n", (uint8_t*)key, uidq_node_key(node));

   return node;
}

   int 
uidq_hash_push(uidq_hash_t *hash, void *key, void *value) 
{
   if (!uidq_hash_isvalid(hash)) return UIDQ_RET_ERR;
   if (key == NULL) return UIDQ_RET_ERR;
   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);

   uidq_hash_node_t *prev = NULL;
   uidq_hash_node_t *node = uidq_hash_find_node(hash, key, &prev);

   if (node) {
      if (value) {
         memcpy(uidq_node_value(node, config->key_size), value, config->value_size);
      }
      return UIDQ_RET_OK;
   }

   int index = uidq_hash_text(hash, key);
   node = uidq_hash_create_node(hash, key, index, prev);
   if (!node) {
      printf("no node!!\n");
      return UIDQ_RET_ERR;
   }

   if (value) memcpy(uidq_node_value(node, config->key_size), value, config->value_size);

   /*printf("created key='%.*s', value='%.*s'\n",
     (int)config->key_size, (char*)uidq_node_key(node),
     (int)config->value_size, (char*)uidq_node_value(node, config->key_size));
     */
   return UIDQ_RET_OK;
}

void
uidq_hash_pop(uidq_hash_t *hash, void *key) {
   uidq_hash_node_t *prev = NULL;
   uidq_hash_node_t *node = uidq_hash_find_node(hash, key, &prev);
   if (!node) return;

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   int index = uidq_hash_text(hash, key);

   if (prev) {
      prev->next = node->next;
      uidq_pool_pop(hash->collision, node->index);
   } else {
      uidq_hash_node_t *next = node->next;
      if (next) {
         memcpy(node, next, sizeof(uidq_hash_node_t) + config->key_size + config->value_size);
         node->index = index;
         uidq_pool_pop(hash->collision, next->index);
      } else {
         uidq_pool_pop(hash->buckets, index);
      }
   }
}

void
uidq_hash_debug_tree(uidq_hash_t *hash) {
   if (!uidq_hash_isvalid(hash)) return;

   uidq_hash_conf_t *config = uidq_hash_conf_get(hash);
   printf("=== HASH TABLE DEBUG ===\n");

   for (size_t index = 0; index < config->capacity; ++index) {
      uidq_hash_node_t *node = uidq_pool_get(hash->buckets, index);
      if (!uidq_pool_block_check(hash->buckets, index)) continue;

      printf("Bucket %zu:\n", index);

      while (node) {
         printf("  Key: '%.*s', Value: '%.*s'\n",
               (int)config->key_size, uidq_node_key(node),
               (int)config->value_size, uidq_node_value(node, config->key_size));
         node = node->next;
      }
   }

   printf("\n\n");
}
