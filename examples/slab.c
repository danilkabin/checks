#include "../src/core/uidq.h"

#include "core/uidq_slab.h"
#include "misc/uidq_bitmask.h"
#include "misc/uidq_utils.h"
#include <fcntl.h>
#include <stdint.h>
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
   uidq_slab_t *slab = uidq_slab_create_and_init(25, 4);
   if (!slab) {
      DEBUG_ERR("Failed to allocate slab structure.\n");
      return -1;
   }

   DEBUG_FUNC("SLAB. size:%zu, block count:%zu, block size:%zu\n",
         slab->size, slab->block_count, slab->block_size);
   char *data = "yes";
   for (int index = 0; index < 15; index++) {
      uidq_block_add(slab, data, 19);
      uidq_bitmask_debug(slab->bitmask);
   }

   DEBUG_INFO("All tests passed successfully!\n");
   return 0;
}
