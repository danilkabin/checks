#include "core/opium_alloc.h"
#include "core/opium_arena.h"
#include "core/opium_core.h"
#include "core/opium_hash.h"
#include "core/opium_hashfuncs.h"
#include "core/opium_log.h"
#include "core/opium_slab.h"
#include "core/opium_string.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

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

void print_uchar(u_char yes) {
   for (int index = sizeof(u_char) * 8 - 1; index >= 0; index--) {
      printf("%d", (yes >> index) & 1);
   }
   printf("\n");
}

int main() {
   srand((unsigned int)time(NULL));

   opium_log_t *log = opium_log_init(NULL, NULL, NULL);
   opium_arena_t arena;

   int yes = opium_arena_init(&arena, log);
   if (yes != OPIUM_RET_OK) {
      return -1;
   }

   size_t key_size = 10;
   size_t value_size = 10;
   size_t nelts = 140;

   opium_hash_t hash;
   opium_hash_init(&hash, nelts, key_size, value_size, log);

   opium_hash_insert(&hash, "Hello", "Value");
   opium_hash_insert(&hash, "Hello1", "Value");
   opium_hash_insert(&hash, "hello", "Value");
   opium_hash_insert(&hash, "bye", "Value");
   opium_hash_insert(&hash, "yes", "Value");
   opium_hash_insert(&hash, "ah yes bro", "Value");
   opium_hash_insert(&hash, "bye bye bye", "Value");


   return 0;
}
