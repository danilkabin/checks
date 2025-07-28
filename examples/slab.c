#include "../src/core/uidq.h"

#include "core/uidq_slab.h"
#include "misc/uidq_bitmask.h"
#include "misc/uidq_utils.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ASSERT(cond, msg) \
   do { \
      if (!(cond)) { \
         DEBUG_ERR("ASSERT FAILED: %s\n", msg); \
         exit(EXIT_FAILURE); \
      } \
   } while (0)

int main() {
   uidq_slab_t *slab = uidq_slab_create_and_init(30, 5);
   if (!slab) {
      DEBUG_ERR("Failed to allocate slab structure.\n");
      return -1;
   }

   uidq_slab_info(slab);
   char *data = "yes";
   for (int index = 0; index < 3; index++) {
      int start_pos = uidq_slab_alloc(slab, data, 7);
      uidq_bitmask_debug(slab->bitmask);
   }

   for (int index = slab->block_count; index >= 0; index--) {
      uidq_slab_dealloc(slab, index);
      uidq_bitmask_debug(slab->bitmask);
   }

   DEBUG_INFO("All tests passed successfully!\n");
   return 0;
}
