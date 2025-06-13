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
      .port = htons(51234),
      .addr.s_addr = htonl(INADDR_ANY)
   };

   sock_syst_init();

   struct slab *allocator = slab_init(1256);
   if (!allocator) {
      return -1;
   }
   DEBUG_FUNC("blocks: %zu pool size: %zu\n", allocator->block_capacity, allocator->pool_size);

   void *data = slab_malloc(allocator, 128);
   memcpy(data, &port_conf, sizeof(struct tcp_port_conf));
   struct tcp_port_conf *port_conf2 = (struct tcp_port_conf*)data;
   DEBUG_FUNC("port: %d\n", ntohs(port_conf2->port));
   sleep(30);
   sock_syst_exit(NULL);
   DEBUG_FUNC("heehllo1oo1o1\n");
   return 0;
}
