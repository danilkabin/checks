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
      int start_pos = uidq_slab_alloc(slab, data, 5);
      uidq_bitmask_debug(slab->bitmask);
      uidq_slab_info(slab);
      uidq_slab_info_block(slab, start_pos);
   }

   uidq_slab_info(slab);
   char *ye = "abcdaye";
   DEBUG_FUNC("sdffds\n");
   char *datochka = (char*)uidq_slab_get(slab, 0);
   printf("datocakcskack: %s\n", datochka);

   char *eye = "123456789";
   uidq_bitmask_debug(slab->bitmask);
   int pos = uidq_block_realloc(slab, 0, eye, 13); 
   datochka = (char*)uidq_slab_get(slab, pos);
   printf("datocakcskack: %s\n", datochka);

      uidq_slab_info_block(slab, pos);
   uidq_bitmask_debug(slab->bitmask);
   for (int index = 0; index < 6; index++) {
      uidq_slab_dealloc(slab, index);
      uidq_bitmask_debug(slab->bitmask);
      uidq_slab_info(slab);
   }

   uidq_bitmask_debug(slab->bitmask);
   DEBUG_INFO("All tests passed successfully!\n");
   return 0;
}
