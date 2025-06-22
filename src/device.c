#include "device.h"
#include "parser.h"
#include "utils.h"
#include <unistd.h>

int dev_core_count = 0;

int flowbin_device_init() {
   int ret;
   dev_core_count = (int)sysconf(_SC_NPROCESSORS_ONLN);

   return 0;
free_everything:
   flowbin_device_exit();
free_this_trash:
   return -1; 
}

void flowbin_device_exit() {

}
