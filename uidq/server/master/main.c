#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/uidq_bitmask.h"
#include "core/uidq_conf.h"
#include "core/uidq_core.h"
#include "core/uidq_hash.h"
#include "core/uidq_list.h"
#include "core/uidq_log.h"
#include "core/uidq_pool.h"
#include "core/uidq_slab.h"

int main() {
   uidq_log_conf_t log_conf = {.debug = NULL, .warn = NULL, .err = NULL};
   uidq_log_t *log = uidq_log_create(&log_conf);

   uidq_slab_conf_t confii = {.count = 4};
   uidq_slab_t *slab = uidq_slab_create(&confii, log);
   if (!slab) {
      return -1;
   }

   void *objects[100] = {0};
   for (int index = 0; index < 5; index++) {
      for (int index = 0; index < 73; index++) {
//         objects[index] = uidq_slab_push(slab, 2000);
      }
         objects[index] = uidq_slab_push(slab, 2000);

      for (int index = 0; index < 100; index++) {
         if (objects[index]) {
            uidq_slab_pop(slab, objects[index]);
            objects[index] = NULL;
         }
      }
   }


   uidq_slab_chains_debug(slab);

   /*char *value = "value";

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

*/
   return 0;
}
