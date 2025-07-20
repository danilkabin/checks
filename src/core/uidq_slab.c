#include <stdint.h>
#include <stdlib.h>

#include "uidq_slab.h"
#include "uidq_bitmask.h"
#include "uidq_utils.h"

#define UIDQ_SLAB_DEBUG UIDQ_DEBUG_ENABLED
#if UIDQ_SLAB_DEBUG == UIDQ_DEBUG_DISABLED
#define DEBUG_INFO(fmt, ...) do {} while(0)
#define DEBUG_ERR(fmt, ...)  do {} while(0)
#endif

int uidq_block_add(uidq_slab_t *slab, void *data, size_t size) {
   UIDQ_STRUCT_CHECK_INIT(slab, goto fail);

   if (!data || size < 1) {
      DEBUG_ERR("Invalid output: data or size.\n");
      goto fail;
   }

   size_t block_free = slab->block_free;
   size_t block_demand = (size / slab->block_size) + 1;
   DEBUG_FUNC("Block demand: %zu\n", block_demand);

   if (block_demand > block_free) {
      DEBUG_ERR("Block demand > block_free.\n"); 
      goto fail;
   }

   uidq_bitmask_t *bitmask = slab->bitmask;

   int start_pos = uidq_bitmask_add(bitmask, 0, block_demand); 
   if (start_pos < 0) {
      DEBUG_ERR("Failed to find free bit\n");
      goto fail;
   }

  DEBUG_FUNC("Start pos: %d\n", start_pos); 

   return 0;
fail:
   return -1;
}

void uidq_block_del() {

}

int uidq_slab_init(uidq_slab_t *slab, size_t size, size_t block_size) {
   UIDQ_STRUCT_CHECK_INSTANCE(slab, goto fail); 

   if (slab->initialized) {
      DEBUG_ERR("Slab structure is initialized.\n");
      goto fail;
   }

   if (size <= 0 || block_size <= 0) {
      goto fail;
   }

   size_t round_size = (size_t)uidq_round_pow(size, 2);
   if (round_size <= 1) {
      printf("Failed to allocate, rounded size = %zu for requested size = %zu\n", round_size, size);
      goto fail;
   }
   size = round_size;

   size_t block_count = size / block_size;

   size_t block_busy = 0;
   size_t block_free = block_count;

   slab->size        = size;

   slab->block_count = block_count;
   slab->block_size  = block_size;

   slab->block_busy  = block_busy;
   slab->block_free  = block_free;

   slab->initialized = true;

   slab->bitmask = uidq_bitmask_create_and_init(slab->block_count, sizeof(uint64_t));
   if (!slab->bitmask) {
      DEBUG_ERR("Failed to initialize bitmask structure.\n");
      goto fail;
   }

   slab->data = calloc(1, size);
   if (!slab->data) {
      DEBUG_ERR("Failed to initialize data in slab structure.\n");
      goto struct_exit;
   }

   return 0;
struct_exit:
   uidq_slab_exit(slab); 
fail:
   return -1;
}

void uidq_slab_exit(uidq_slab_t *slab) {
   UIDQ_STRUCT_CHECK_INIT(slab, return);

   slab->size        = 0;
   slab->block_count = 0;
   slab->block_size  = 0;
   slab->block_busy  = 0;
   slab->block_free  = 0;

   if (slab->data) {
      free(slab->data);
      slab->data = NULL;
   }

   if (slab->bitmask) {
      uidq_bitmask_free(slab->bitmask);
      slab->bitmask = NULL;
   }
}

uidq_slab_t *uidq_slab_create() {
   uidq_slab_t *slab = malloc(sizeof(uidq_slab_t)); 
   if (!slab) {
      DEBUG_ERR("Failed to allocate slab structure.\n");
      return NULL;
   }
   slab->instance = true;
   return slab;
}

void uidq_slab_free(uidq_slab_t *slab) {
   UIDQ_STRUCT_CHECK_INSTANCE(slab, return); 
   if (slab->initialized) {
      uidq_slab_exit(slab);
   }
   free(slab);
}

uidq_slab_t *uidq_slab_create_and_init(size_t size, size_t block_size) {
   uidq_slab_t *slab = uidq_slab_create();
   if (!slab) {
      DEBUG_ERR("Failed to create slab: size=%zu, block_size=%zu\n", size, block_size);
      return NULL;
   }

   int ret = uidq_slab_init(slab, size, block_size);
   if (ret < 0) {
      DEBUG_ERR("Failed to initialize slab: size=%zu, block_size=%zu, ret=%d\n", size, block_size, ret);
      uidq_slab_free(slab);
      return NULL;
   }

   return slab;
}

