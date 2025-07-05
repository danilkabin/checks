#include "device.h"
#include "utils.h"
#include <onion/onion.h>
#include <onion/device.h>
#include <onion/ringbuff.h>
#include <onion/epoll.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define TOTAL_SOCKETS_PER_CORE 1

int main() {
   onion_config_t onion_config1;
   onion_config_init(&onion_config1);
   DEBUG_FUNC("dfsdfds : %d\n", onion_config.core_count);
   struct onion_worker_head *head;
   int ret = onion_device_init(&head, 51235, onion_config.core_count, 100, 5);

   sleep(100);

   return 0;
}
