#include "core/opium_alloc.h"
#include "core/opium_arena.h"
#include "core/opium_bitmask.h"
#include "core/opium_core.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include "core/opium_slab.h"
#include "core/opium_string.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ITEM_SIZE 2048
#define MAX_PTRS 1000000  // огромная куча

void print_memory_usage() {
   FILE *f = fopen("/proc/self/status", "r");
   if (!f) return;

   char line[256];
   while (fgets(line, sizeof(line), f)) {
      if (strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "VmData:", 7) == 0) {
         printf("%s", line);
      }
   }
   fclose(f);
}

int main() {
   srand((unsigned int)time(NULL));

   opium_log_t *log = opium_log_init(NULL, NULL, NULL);
   opium_arena_t arena;

   int yes = opium_arena_init(&arena, log);
   if (yes != OPIUM_RET_OK) {
      return -1;
   }

   size_t times = 10;

   void *ptrs[times];

   size_t size = 3;
   for (size_t index = 0; index < times; index++) {
      void *ptr = opium_arena_alloc(&arena, size);
      ptrs[index] = ptr; 
      size = size * 2;
   }

   for (size_t index = 0; index < times; index++) {
     opium_arena_free(&arena, ptrs[index]);
   }

   return 0;
}
