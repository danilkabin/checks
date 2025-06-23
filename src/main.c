#include "device.h"
#include "utils.h"
#include "main.h"
#include "binsocket.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "slab.h"

int main() {
   struct tcp_port_conf port_conf = {
      .domain = AF_INET,
      .type = SOCK_STREAM,
      .port = htons(51235),
      .addr.s_addr = htonl(INADDR_ANY)
   };

   flowbin_device_init();
   sock_syst_init();

   struct slab *allocator = slab_init(1256);
   if (!allocator) {
      return -1;
   }

   sleep(300);
   sock_syst_exit(NULL);
   DEBUG_FUNC("heehllo1oo1o1\n");
   return 0;
}
