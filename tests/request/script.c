#include "epoll.h"
#include "utils.h"
#include <fcntl.h>
#include <onion/onion.h>
#include <onion/poll.h>
#include <onion/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

void *handler(void *arg, onion_handler_ret_t handler_ret) {
   onion_epoll_t *ep = (onion_epoll_t*)arg;
   return NULL;
}

#define TOTAL_SOCKETS_PER_CORE 1

int main() {
   onion_config_t onion_config1;
   onion_config_init(&onion_config1);

   int ret = onion_epoll_static_init(onion_config.core_count);
   if (ret < 0) {
      DEBUG_FUNC("No epoll!\n");
      return -1;
   }
   onion_epoll_t *childs[onion_config.core_count];

   for (int i = 0; i < onion_config.core_count; i++) {
      onion_epoll_t *ep = onion_epoll1_init(handler, 100000);
      if (!ep) {
         DEBUG_ERR("epoll init failed for core %d\n", i);
         childs[i] = NULL;
         continue;
      }
      childs[i] = ep;
   }

   sleep(1);

   for (int i = 0; i < onion_config.core_count; i++) {
      if (!childs[i] || !childs[i]->active || childs[i]->fd <= 0) {
         DEBUG_ERR("childs[%d] invalid\n", i);
         continue;
      }

      for (int j = 0; j < TOTAL_SOCKETS_PER_CORE; j++) {
         int fd = socket(AF_INET, SOCK_STREAM, 0);
         if (fd < 0) {
            perror("socket");
            continue;
         }

         int flags = fcntl(fd, F_GETFL, 0);
         fcntl(fd, F_SETFL, flags | O_NONBLOCK);

         ret = onion_epoll_slot_add(childs[i], fd, NULL, NULL, NULL, NULL);
         if (ret < 0) {
            DEBUG_ERR("Failed to add fd=%d to epoll core %d, index=%d\n", fd, i, j);
            close(fd);
         } else {
            DEBUG_FUNC("Added fd=%d to epoll core %d, index=%d\n", fd, i, j);
         }
               usleep(1000);
      }
      usleep(10000);
   }

   sleep(100);

   for (int i = 0; i < onion_config.core_count; i++) {
      if (childs[i] && childs[i]->active) {
         onion_epoll1_exit(childs[i]);
      }
   }

   onion_epoll_static_exit();

   return 0;
}
