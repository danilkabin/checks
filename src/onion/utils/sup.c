#define _GNU_SOURCE

#include "utils.h"
#include "sup.h"

#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int onion_cpu_set_core(pthread_t thread, long core) {
   cpu_set_t set;
   CPU_ZERO(&set);
   CPU_SET(core, &set);
   if (pthread_setaffinity_np(thread, sizeof(set), &set) != 0) {
      DEBUG_ERR("Failed to set core affinity.\n");
      return -1;
   }
   return 0;
}

void onion_set_bit(onion_bitmask *bitmask, size_t offset) {
   bitmask->mask[offset / 64] |= (1ULL << (offset % 64));
}

void onion_clear_bit(onion_bitmask *bitmask, size_t offset) {
   bitmask->mask[offset / 64] &= ~(1ULL << (offset % 64));
}

int onion_ffb(onion_bitmask *bitmask, int offset, int type) {
   if (type != 1 && type != 0) {
      DEBUG_FUNC("Correct type must be provided!\n");
      return -1;
   }

   size_t size = bitmask->conv_size;
   size_t bit_size = bitmask->size_per_frame;
   size_t count = bitmask->conv_size;

   int frame_offset = offset / bitmask->size_per_frame;
   int bit_offset = offset % bitmask->size_per_frame;
   for (size_t frame = frame_offset; frame < count; frame++) {
      uint64_t mask = bitmask->mask[frame];
      if (type == 0) {
         mask = ~mask;
      }

      for (int bit = ((int)frame == frame_offset ? bit_offset : 0); (size_t)bit < bit_size; bit++) {
         if ((mask >> bit) & 1) {
            int new_pos = frame * bit_size + bit;
            if (new_pos > (int)bitmask->size) {
               return -1;
            }
            return new_pos;
         } 
      }
   }
   return -1;
}

int onion_ffgb(onion_bitmask *bitmask, int grab, int type) {
   size_t offset = 0;

   int combo = 0;
   int last_pos = -2;
   int start_pos = -1;

   while ((start_pos = onion_ffb(bitmask, offset, type)) >= 0) {
      offset = start_pos + 1;
      if (start_pos - last_pos > 1) {
         combo = 0;
      }
      combo = combo + 1;
      last_pos = start_pos;
      if (combo >= grab) {
         return start_pos - grab + 1; 
         // for example grab = 3 
         // start_pos = 10[8,9,10]
         // start_pos[10] - grab[3] = 7 + 1
      }
   }
   return -1;
}

int onion_find_bit(onion_bitmask *bitmask, int type) {
   return onion_ffb(bitmask, 0, type); 
}

int onion_find_grab_bit(onion_bitmask *bitmask, int grab, int type) {
   return onion_ffgb(bitmask, grab, type); 
}

int onion_bitmask_add(onion_bitmask *bitmask, size_t count) {
   int start_pos = onion_find_bit(bitmask, 0);
   if (start_pos < 0) {
      DEBUG_ERR("No found bit!\n");
      return -1;
   }

   for (size_t index = 0; index < count; index++) {
      onion_set_bit(bitmask, start_pos + index);
   }

   return start_pos;
}

void onion_bitmask_del(onion_bitmask *bitmask, int start_pos, size_t count) {
   for (size_t index = 0; index < count; index++) {
      onion_clear_bit(bitmask, start_pos + index);
   }
}

int onion_bitmask_init(onion_bitmask **ptr, size_t size, size_t size_per_frame) {
   if (size < 1 || size_per_frame < 1) {
      DEBUG_FUNC("Size must be provided\n");
      goto unssuccessfull;
   }

   onion_bitmask *bitmask = malloc(sizeof(onion_bitmask));
   if (!bitmask) {
      DEBUG_ERR("Bitmask initialization failed!\n");
      goto unssuccessfull; 
   }

   bitmask->size = size;
   bitmask->size_per_frame = 8 * size_per_frame;
   bitmask->conv_size = (size + 63) / 64;

   bitmask->mask = malloc(bitmask->conv_size * sizeof(uint64_t));
   if (!bitmask->mask) {
      DEBUG_ERR("Mask initialization failed!\n");
      goto free_bitmask;
   }
   memset(bitmask->mask, 0, bitmask->conv_size * sizeof(uint64_t));
   *ptr = bitmask;
   return 0;
free_bitmask:
   free(bitmask);
unssuccessfull:
   return -1;
}

void onion_bitmask_exit(onion_bitmask *bitmask) {
   if (bitmask->mask) {
      free(bitmask->mask);
      bitmask->mask = NULL;
   }
}
