#include "uidq_config.h"
#include "types.h"
#include "uidq_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int uidq_save_config(const char *real, const char *name, char *newpath) {
   size_t pathsize = 256;
   char fullpath[pathsize];

   snprintf(fullpath, pathsize, "%s/%s", newpath, name); 

   FILE *file = fopen(fullpath, "rw");
   if (file == NULL) {
      DEBUG_ERR("Failed to open file. Path: %s\n", fullpath);
   }



   return UIDQ_OK;
}
