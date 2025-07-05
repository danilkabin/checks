#include "onion.h"
#include "sup.h"
#include "utils.h"
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

const char *path = "config.ini"
onion_config_t onion_config = {0};

int onion_read_ini(const char *file_name) {
   FILE *file = fopen(file_name, "r");
   if (!file) {
      return -1;
   }
   char line[256];
   while (fgets(line, sizeof(line), file)) {
      if (line[0] == '#' || line[0] == '\n') {
         continue;
      }
      DEBUG_FUNC("line: %s\n", line);
   }
}

int onion_config_init(onion_config_t *user_cfg) {
   long core_count = sysconf(_SC_NPROCESSORS_CONF);
   if (core_count < 1) {
      DEBUG_FUNC("onion_config_init: no CPU cores found (core_count = %ld)\n", core_count);
      return -1;
   }
   onion_config.core_count = core_count;

   long sched_core = 0;
   if (onion_cpu_set_core(pthread_self(), sched_core) == -1) {
      DEBUG_FUNC("failed to set CPU affinity.\n");
      return -1;
   }
   onion_config.sched_core = sched_core;

   if (sched_core >= core_count) {
      DEBUG_FUNC("sched_core (%ld) >= core_count (%ld)\n", sched_core, core_count);
      return -1;
   }

   return 0;
}
