#include "device.h"
#include "utils.h"
#include <onion/onion.h>
#include <onion/device.h>
#include <onion/ringbuff.h>
#include <onion/epoll.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>

#define TOTAL_SOCKETS_PER_CORE 1

int main() {
   onion_config_t *onion_config = onion_config_init();
   //DEBUG_FUNC("dfsdfds : %d\n", onion_config.core_count);
   struct onion_worker_head_t *head = onion_device_init(&onion_config->server_triad);

   sleep(100);

   return 0;
}
