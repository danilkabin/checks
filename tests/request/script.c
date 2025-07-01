#include "utils.h"
#include <onion/onion.h>
#include <onion/poll.h>
#include <onion/ringbuff.h>
#include <onion/epoll.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

void *handler(void *arg, onion_epoll_tag_t *tag, onion_handler_ret_t handler_ret) {
   // onion_epoll_t *ep = (onion_epoll_t*)arg;
   return NULL;
}

#define TOTAL_SOCKETS_PER_CORE 1

int main() {
   onion_config_t onion_config1;
   onion_config_init(&onion_config1);

   sleep(100);

   return 0;
}
