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

   uidq_bitmask_t *bitmask = uidq_bitmask_create({.capacity = 10}, log); 

 
 /*  uidq_slab_conf_t confii = {.count = 4};
   uidq_slab_t *slab = uidq_slab_create(&confii, log);
   if (!slab) {
      return -1;
   }

   size_t sizeee = 1;
   void *ptr[5] = {0};
   for (size_t index = 0; index < sizeee; index++) {
      ptr[index] = uidq_slab_push(slab, 16000);
   } 

   uidq_slab_pop(slab, ptr[0]);

   uidq_slab_chains_debug(slab);

   for (size_t index = 0; index < sizeee; index++) {
      ptr[index] = uidq_slab_push(slab, 16000);
   } 

   uidq_slab_pop(slab, ptr[0]);

   uidq_slab_chains_debug(slab);*/
   return 0;
}
