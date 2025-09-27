#include "core/opium_core.h"

#define OPIUM_HASHT_KEY_SIZE   8
#define OPIUM_HASHT_VALUE_SIZE 8
#define OPIUM_HASHT_COLL_MULT  1

static inline size_t opium_hash_text(opium_hash_t *hash, void *key, opium_ubyte_t *normalized) {
   size_t len = opium_strnlen(key, hash->key_size);
   size_t write = len < hash->key_size ? len : hash->key_size;

   opium_memzero(normalized, hash->key_size);
   opium_memcpy(normalized, key, write);

   return hash->func(key, write) % hash->nelts;
}

   static opium_hash_node_t *
opium_hash_find(opium_hash_t *hash, void *key)
{
   opium_ubyte_t normalized[hash->key_size];
   int index = opium_hash_text(hash, key, normalized);

   printf("key: %s, index: %d\n", (char*)key, index);

   return NULL;
}

   int
opium_hash_init(opium_hash_t *hash, size_t nelts, size_t ks, size_t vs, opium_log_t *log)
{
   int ret;
   assert(hash != NULL);
   assert(ks >= OPIUM_HASHT_KEY_SIZE);
   assert(vs >= OPIUM_HASHT_VALUE_SIZE);

   hash->nelts = nelts;
   hash->key_size = ks;
   hash->value_size = vs;
   hash->func = opium_hash_djb2;

   hash->pair_size = sizeof(opium_hash_node_t) + hash->key_size + hash->value_size;

   ret = opium_slab_init(&hash->buckets, hash->pair_size, log);
   if (ret != OPIUM_RET_OK) {
      opium_log_err(hash->log, "Failed to initialize buckets in hash table.\n");
      return OPIUM_RET_ERR;
   }

   hash->log = log;

   return OPIUM_RET_OK;
}

   int
opium_hash_insert(opium_hash_t *hash, void *key, void *value)
{
   assert(hash != NULL);
   assert(key != NULL);
   assert(value != NULL);
   opium_hash_find(hash, key);

   return 1;
}
