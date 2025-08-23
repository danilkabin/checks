#include <stdio.h>

#include "core/uidq_bitmask.h"
#include "core/uidq_hash.h"
#include "core/uidq_pool.h"

int main() {
   struct yesyes {
      int hello;
   };

   size_t max = 10;
   size_t size = sizeof(struct yesyes);

   uidq_pool_t *pool = uidq_pool_create(max, size, NULL);
   if (!pool) {
      return -1;
   }

   uidq_hash_conf_t conf = {
      .key_size = 16,
      .value_size = 16,
      .capacity = 2,
      .coll_mult = 2,
   };

   uidq_hash_t *hash = uidq_hash_create(&conf, NULL);
   if (!hash) {
      return -1;
   }

   char *key = "key";
   char *value = "value";
   
   uidq_hash_push(hash, "try1", value);
   uidq_hash_push(hash, "try2", value);
   uidq_hash_push(hash, "try3", value);
   uidq_hash_push(hash, "try4", value);

   uidq_hash_pop(hash, "try4");
   uidq_hash_debug_tree(hash);
   uidq_hash_pop(hash, "try2");
   uidq_hash_debug_tree(hash);
   uidq_hash_pop(hash, "try1");
   uidq_hash_debug_tree(hash);

   return 0;
}
