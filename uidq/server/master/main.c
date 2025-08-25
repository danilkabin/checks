#include <stdio.h>
#include <string.h>

#include "core/uidq_bitmask.h"
#include "core/uidq_conf.h"
#include "core/uidq_hash.h"
#include "core/uidq_log.h"
#include "core/uidq_pool.h"

int main() {
   uidq_log_conf_t log_conf = {.debug = NULL, .warn = NULL, .err = NULL};
   uidq_log_t *log = uidq_log_create(&log_conf);

   uidq_hash_conf_t conf = {
      .key_size = 16,
      .value_size = 16,
      .capacity = 4,
      .coll_mult = 2,
   };

   uidq_hash_t *hash = uidq_hash_create(&conf, log);
   if (!hash) {
      return -1;
   }

   char *value = "value";

   char key[16];
   for (int index = 0; index < 23; index++) {
      snprintf(key, sizeof(key), "key%d", index);
      uidq_hash_push(hash, key, value); 
   }

   uidq_hash_push(hash, "key3", value);

   uidq_hash_debug_tree(hash);
   uidq_hash_realloc(hash, 220);
   printf("capacity: %zu\n", hash->conf.capacity);
   uidq_hash_debug_tree(hash);


   return 0;
}
