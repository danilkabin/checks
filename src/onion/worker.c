#include "utils.h"
#include "onion.h"

#include "worker.h"
#include "receive.h"
#include "send.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

void *onion_worker_thread(void *args) {
   struct onion_worker *work = (struct onion_worker*)args;
   if (!work) {
      DEBUG_ERR("no onion_worker!\n");
      return NULL;
   }
   struct onion_server_sock *server_sock = work->server_sock;
   if (!server_sock) {
      DEBUG_ERR("no server sock\n");
      return NULL;
   }
   int timeout = -1;
   while (!server_sock->released) {
      if (!server_sock->initialized) {
         continue;
      }
      int work_count = epoll_wait(work->epoll_fd, work->events, ONION_BS_EPOLL_MAX_EVENTS, timeout);
      if (work_count > 0) {
         for (int index = 0; index < work_count; index++) {
            struct epoll_event event = work->events[index];
            if (event.data.ptr == server_sock) {
               if (event.events & EPOLLIN) {
                  DEBUG_FUNC("FDSdsfsd\n");
                  onion_server_sock_accept(server_sock);
               }
            } else {
               struct onion_peer_sock *peer = event.data.ptr;
               if (!peer) {
                  continue;
               }
               if (event.events & EPOLLIN) {
                  onion_peer_sock_recv(server_sock, peer, MSG_DONTWAIT, ONION_BS_CLIENT_BUFF_CAPACITY);
               } else if (event.events & EPOLLOUT) {
                  //DEBUG_FUNC("send!\n");
               }
          //     onion_peer_sock_release(server_sock, peer);
            }
         }
      }
   }
   return NULL;
}

