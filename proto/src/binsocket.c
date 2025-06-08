#include <asm-generic/socket.h>
#define _GNU_SOURCE

#include "listhead.h"
#include "lock.h"
#include "main.h"
#include "binsocket.h"
#include "pool.h"
#include "utils.h"
#include "vector.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

int CORE_COUNT;
struct worker **sock_workers;

uint8_t *bs_get_peers_bitmap(struct server_sock *bs) {
   return bs->peerIDs;
}

void set_worker_affinity(pthread_t thread, int core_id) {
   cpu_set_t set_t;
   CPU_ZERO(&set_t);
   CPU_SET(core_id, &set_t);
   int ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &set_t);
   if (ret != 0) {
      DEBUG_ERR("NO CORE!\n");
   }
}

void worker_release(struct worker *work) {
   if (work->bs) {
      server_sock_release(work->bs);
   }
   if (work->epoll_fd) {
      close(work->epoll_fd);
      work->epoll_fd = -1;
   }
   if (work->core.flow) {
      pthread_cancel(work->core.flow);
      pthread_join(work->core.flow, NULL);
      work->core.flow = 0;
   }
   work->bs = NULL;
   free(work);
}

int sock_t_init(sock_t *sock) {
   sock->state = SOCKET_STATUS_OPEN;
   sock->packets_sent = 0;
   sock->packets_received = 0;
   return 0;
}

struct server_sock *server_sock_create(struct tcp_port_conf *port_conf, struct worker *work, int work_index, int peer_capable) {
   int ret;

   if (!port_conf->port || !port_conf->domain || !port_conf->type) {
      DEBUG_FUNC("Invalid port configuration provided.\n");
      goto free_this_trash;
   }

   struct server_sock *bs = malloc(sizeof(struct server_sock));
   if (!bs) {
      DEBUG_ERR("Failed to allocate memory for server socket.\n");
      goto free_this_trash;
   }

   struct sockaddr_in sock_addr;
   sock_addr.sin_addr.s_addr = port_conf->addr.s_addr;
   sock_addr.sin_port = port_conf->port;
   sock_addr.sin_family = port_conf->domain;

   bs->initialized = false;
   bs->released = false;
   bs->worker = work;
   bs->worker_index = work_index;
   bs->sock.state = SOCKET_STATUS_CLOSE;
   bs->peer_current = 0;
   bs->peer_capable = peer_capable;
   bs->peer_queue_capable = 30;

   bs->sock.fd = socket(port_conf->domain, port_conf->type, port_conf->protocol);
   if (bs->sock.fd < 0) {
      DEBUG_FUNC("Failed to create server socket descriptor: %s\n", strerror(errno));
      goto free_everything;
   }

   ret = memoryPool_init(&bs->peer_pool, MAX_CLIENTS_CAPABLE * sizeof(struct peer_sock), sizeof(struct peer_sock));
   if (ret < 0) {
      DEBUG_FUNC("Client memory pool initialization failed.\n");
      goto free_everything;
   }

   bs->peerIDs_size = (bs->peer_capable + 7) / 8;
   bs->peerIDs = malloc(bs->peerIDs_size);
   if (!bs->peerIDs) {
      DEBUG_ERR("Failed to allocate memory for peer ID bitmap.\n");
      goto free_everything;
   }
   memset(bs->peerIDs, 0, bs->peerIDs_size);

   int turn = 1;
   ret = setsockopt(bs->sock.fd, SOL_SOCKET, SO_REUSEPORT, &turn, sizeof(turn));
   if (ret < 0) {
      DEBUG_ERR("Failed to set socket options (SO_REUSEPORT).\n");
      goto free_everything;
   }

   ret = bind(bs->sock.fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
   if (ret < 0) {
      DEBUG_ERR("Failed to bind server socket.\n");
      goto free_everything;
   }

   ret = listen(bs->sock.fd, bs->peer_queue_capable);
   if (ret < 0) {
      DEBUG_ERR("Failed to listen on server socket.\n");
      goto free_everything;
   }

   work->event.events = EPOLLIN | EPOLLOUT | EPOLLET;
   work->event.data.ptr = bs;
  
   ret = epoll_ctl(bs->worker->epoll_fd, EPOLL_CTL_ADD, bs->sock.fd, &bs->worker->event);
   if (ret < 0) {
      DEBUG_ERR("Failed to add socket to epoll.\n");
      goto free_everything;
   }

   sock_t_init(&bs->sock);
   INIT_LIST_HEAD(&bs->peers);
   bs->initialized = true;
   DEBUG_FUNC("Server socket created successfully. Descriptor: %d\n", bs->sock.fd);
   return bs;
free_everything:
   server_sock_release(bs);
free_this_trash:
   return NULL;
}

void server_sock_release(struct server_sock *bs) {
   int ret;

   if (bs->released) {
      return;
   }

   bs->released = true;
   bs->initialized = false;

   ret = epoll_ctl(bs->worker->epoll_fd, EPOLL_CTL_DEL, bs->sock.fd, &bs->worker->event);
   if (ret < 0) {
      if (errno != ENOENT) {
         DEBUG_ERR("Failed to remove fd from epoll: %s\n", strerror(errno));
      }
   }

   if (bs->peerIDs) {
      free(bs->peerIDs);
      bs->peerIDs = NULL;
   }

   if (bs->peer_pool) {
      struct peer_sock *pos, *temp;
      list_for_each_entry_safe(pos, temp, &bs->peers, list) {
         peer_sock_release(bs, pos);
      }
      free(bs->peer_pool);
   }

   close(bs->sock.fd);
   free(bs);
}

struct peer_sock *peer_sock_create(struct server_sock *bs, sockType fd) {
   int ret;
   int fcntl_result = fcntl(fd, F_GETFL, 0);
   if (fcntl_result == -1) {
      DEBUG_ERR("failed to get peer socket flags\n");
      goto free_this_trash;
   }

   fcntl_result = fcntl(fd, F_SETFL, fcntl_result | O_NONBLOCK);
   if (fcntl_result == -1) {
      DEBUG_ERR("failed to set O_NONBLOCK flag on peer socket\n");
      goto free_this_trash;
   } 

   struct peer_sock *peer = (struct peer_sock*)memoryPool_allocBlock(bs->peer_pool);
   if (!peer) {
      DEBUG_ERR("failed to allocate peer socket from peer pool\n");
      goto free_this_trash;
   }

   peer->worker = bs->worker;

   peer->sock.fd = fd;
   peer->buff_len = 0;

   peer->released = false;
   peer->initialized = true;

   list_add_tail(&peer->list, &bs->peers);
   return peer;
free_everything:
   peer_sock_release(bs, peer);
free_this_trash:
   return NULL;
}

void peer_sock_release(struct server_sock *bs, struct peer_sock *peer) {
   int ret;
   if (peer->released) {
      return;
   }
   list_del(&peer->list);
   peer->released = true;

   if (peer->sock.fd > 0) {
      struct worker *work = peer->worker;
      if (work) {
         epoll_ctl(work->epoll_fd, EPOLL_CTL_DEL, peer->sock.fd, NULL);
         atomic_fetch_sub(&work->peer_count, 1);
      }
      close(peer->sock.fd);
   }

   memoryPool_freeBlock(bs->peer_pool, peer);
   DEBUG_FUNC("Peer fd %d released on core %d\n", peer->sock.fd, peer->worker ? peer->worker->core_id : -1);
}

int server_sock_accept(struct server_sock *bs, accept_callback_sk work) {
   int ret = -1;

   if (!bs) {
      DEBUG_FUNC("no server socket provided\n");
      goto free_this_trash;
   }

   struct sockaddr_in sock_addr;
   socklen_t addrlen = sizeof(sock_addr);
   
   sockType peer_fd = accept(bs->sock.fd, (struct sockaddr*)&sock_addr, &addrlen);
   if (peer_fd < 0) {
      DEBUG_ERR("accept() failed with error: %s\n", strerror(errno));
      goto free_this_trash;
   }

   struct peer_sock *peer = peer_sock_create(bs, peer_fd);
   if (!peer) {
      DEBUG_ERR("peer initialization failed for fd %d\n", peer_fd);
      goto free_sock;
   }

   struct epoll_event peer_event;
   peer_event.events = EPOLLIN | EPOLLOUT | EPOLLET;
   peer_event.data.ptr = peer;

   ret = epoll_ctl(bs->worker->epoll_fd, EPOLL_CTL_ADD, peer->sock.fd, &peer_event);
   if (ret == -1) {
      DEBUG_ERR("failed to add peer fd %d to epoll: %s\n", peer->sock.fd, strerror(errno));      
      goto free_everything;
   }
   DEBUG_FUNC("new peer fd %d added to epoll\n", peer->sock.fd);
   return peer_fd;
free_everything:
   peer_sock_release(bs, peer);
   return ret;
free_sock:
   close(peer_fd);
free_this_trash:
   return ret;
}

size_t peer_sock_send(struct server_sock *bs, struct peer_sock *peer, int flags) {
   if (!bs || !peer) {
      DEBUG_FUNC("no bs/peer\n");
      return -1;
   }
   size_t bytes_sent = send(peer->sock.fd, peer->buff, peer->buff_len, flags);
   if (bytes_sent > 0) {
      if (bytes_sent < peer->buff_len) {
         memmove(peer->buff, peer->buff + bytes_sent, peer->buff_len - bytes_sent);
      } 
      peer->buff_len = peer->buff_len - bytes_sent;
     // DEBUG_FUNC("bytes sent: %zu from: %d\n", bytes_sent, peer->sock.fd);
   }
   return bytes_sent; 
}

size_t peer_sock_recv(struct server_sock *bs, struct peer_sock *peer, size_t buff_size, int flags) {
   if (!bs || !peer) {
      DEBUG_FUNC("no bs/peer\n");
      return -1;
   }
   if (peer->buff_len >= BS_CLIENT_BUFF_SIZE) {
      DEBUG_FUNC("max buff size\n");
      return -1;
   }
   size_t bytes_read = recv(peer->sock.fd, peer->buff + peer->buff_len, buff_size - peer->buff_len, flags);
   if (bytes_read > 0) {
      peer->buff_len += bytes_read;
      peer->sock.packets_received++;
     // DEBUG_FUNC("bytes read: %zu from: %d\n", bytes_read, peer->sock.fd);
   } else if (bytes_read == 0) {
      // DEBUG_FUNC("peer disconnected: %d\n", peer->sock.fd);
   } else {
      DEBUG_FUNC("recv failed: %s\n", strerror(errno));
   }
   return bytes_read;
}

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
            if (work->event.data.fd == event.data.fd) {
               if (event.events & EPOLLIN) {
                  int peer_fd = server_sock_accept(bs, NULL);
               }
            } else {
               struct peer_sock *peer = event.data.ptr;
               if (!peer) {
                  continue;
               } 
               if (event.events & EPOLLIN) {
                  size_t bytes_read = peer_sock_recv(bs, peer, MSG_DONTWAIT, BS_CLIENT_BUFF_SIZE);
                  size_t bytes_sent =  peer_sock_send(bs, peer, MSG_DONTWAIT);
               } else if (event.events & EPOLLOUT) {

               }
            }
         }
      }
   }
   return NULL;
}

int sock_core_init() {
   int max_peers = MAX_CLIENTS_CAPABLE / CORE_COUNT;
   int extra = MAX_CLIENTS_CAPABLE % CORE_COUNT;

   sock_workers = malloc(sizeof(struct worker *) * CORE_COUNT);
   if (!sock_workers) {
      DEBUG_INFO("no workers\n");
      goto free_this_trash;
   }

   struct tcp_port_conf port_conf = {
      .domain = AF_INET,
      .type = SOCK_STREAM,
      .port = htons(51234),
      .addr.s_addr = htonl(INADDR_ANY)
   };

   for (int work_index = 0; work_index < CORE_COUNT; work_index++) {
      sock_workers[work_index] = malloc(sizeof(struct worker));
      if (!sock_workers[work_index]) {
         DEBUG_ERR("no malloc worker\n");
         goto free_everything;
      }
     
      struct worker *work = sock_workers[work_index];
    
      work->epoll_fd = epoll_create1(0);
      if (work->epoll_fd < 0) {
         DEBUG_ERR("flows weren`t inited\n");
         goto free_everything;
      }

      int capable = max_peers + (work_index < extra ? 1 : 0);

      struct server_sock *bs = server_sock_create(&port_conf, work, work_index, capable);
      if (!bs) {
         DEBUG_INFO("where my sock!!\n");
         goto free_everything;
      }

      work->bs = bs;
      work->core_id = work_index;
      work->peer_count = 0;
      work->event.events = EPOLLIN | EPOLLET;

      int thread_ret = pthread_create(&work->core.flow, NULL, bs_worker_thread, work);
      if (thread_ret != 0) {
         DEBUG_ERR("failed to create worker thread %d: %s\n", work_index, strerror(thread_ret));
         goto free_everything;
      }
      set_worker_affinity(work->core.flow, work->core_id);
   }
   return 0;
free_everything:
   sock_core_exit();
free_this_trash:
   return -1;
}

void sock_core_exit() {
   for (int work_index = 0; work_index < CORE_COUNT; work_index++) {
      struct worker *work = sock_workers[work_index];
      if (!work) {
         DEBUG_ERR("work wasn`t found in sock_workers\n"); 
         return;
      }
      worker_release(work);
   }
}

int sock_syst_init(void) {
   int ret = -1;
   CORE_COUNT = (int)sysconf(_SC_NPROCESSORS_ONLN);

   ret = sock_core_init();
   if (ret < 0) {
      DEBUG_FUNC("no sock cores inited\n");
      goto free_this_trash;
   }

   return 0;

free_this_trash:
   return ret;
}

void sock_syst_exit(struct server_sock *bs) {
   sock_core_exit();
}
