#include "device.h"
#include "http.h"
#include "utils.h"
#include <unistd.h>

int dev_core_count = 0;

int flowbin_device_init() {
   int ret;
   dev_core_count = (int)sysconf(_SC_NPROCESSORS_ONLN);

   ret = http_parser_allocator_init(dev_core_count);
   if (ret < 0) {
      DEBUG_FUNC("Parser allocator initialization failed\n");
      goto free_this_trash;
   }
   return 0;
free_everything:
   flowbin_device_exit();
free_this_trash:
   return -1; 
}

void flowbin_device_exit() {

}
