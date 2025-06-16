#include "utils.h"
#include "binsocket.h"

#include "sock_worker.h"
#include "receive.h"
#include "send.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

void *bs_worker_thread(void *args) {
   struct worker *work = (struct worker*)args;
   if (!work) {
      DEBUG_ERR("no worker!\n");
      return NULL;
   }
   struct server_sock *bs = work->bs;
   if (!bs) {
      DEBUG_ERR("no server sock\n");
      return NULL;
   }
   int timeout = -1;
   while (!bs->released) {
      if (!bs->initialized) {
         continue;
      }
      int work_count = epoll_wait(work->epoll_fd, work->events, BS_EPOLL_MAX_EVENTS, timeout);
      if (work_count > 0) {
         for (int index = 0; index < work_count; index++) {
            struct epoll_event event = work->events[index];
            if (event.data.ptr == bs) {
               if (event.events & EPOLLIN) {
                  server_sock_accept(bs);
               }
            } else {
               struct peer_sock *peer = event.data.ptr;
               if (!peer) {
                  continue;
               }
               if (event.events & EPOLLIN) {
                  peer_sock_recv(bs, peer, MSG_DONTWAIT, BS_CLIENT_BUFF_CAPACITY);
               } else if (event.events & EPOLLOUT) {
                  DEBUG_FUNC("send!\n");
               }
          //     peer_sock_release(bs, peer);
            }
         }
      }
   }
   return NULL;
}

