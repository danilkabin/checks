#include "utils.h"
#include "main.h"
#include "binsocket.h"
#include <netinet/in.h>
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
   int ebalo = 1;
   struct hesosksks {
      int yes;
      int xyes;
      char *ebasos[33];
   };
   struct hesosksks ebasos;
   ebasos.yes = 4308;

   int start_index;
   int end_index;
   slab_add(allocator, &ebasos, sizeof(struct hesosksks), &start_index, &end_index);
   
   struct hesosksks *ebasos1 = (struct hesosksks*)((u_int8_t*)allocator->pool + start_index * allocator->block_size);
   if (ebasos1->yes != 4308) {
      DEBUG_FUNC("NO EBASOS!\n");
   }
  slab_del(allocator, start_index, end_index);
     slab_add(allocator, &ebasos, sizeof(struct hesosksks), &start_index, &end_index);
 
  DEBUG_FUNC("ebasos yes: %d\n", ebasos1->yes);
   sleep(30);
   sock_syst_exit(NULL);
   DEBUG_FUNC("heehllo1oo1o1\n");
   return 0;
}
