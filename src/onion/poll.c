#include "worker.h"
#define _GNU_SOURCE

#include "net.h"
#include "parser.h"
#include "slab.h"
#include "listhead.h"
#include "lock.h"
#include "poll.h"
#include "pool.h"
#include "utils.h"

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

int onion_dev_core_count = 6;
struct onion_worker **server_sock_onion_workers;

uint8_t *onion_get_peers_bitmap(struct onion_server_sock *server_sock) {
   return server_sock->peerIDs;
}

void onion_set_onion_worker_affinity(pthread_t thread, int core_id) {
   cpu_set_t set_t;
   CPU_ZERO(&set_t);
   CPU_SET(core_id, &set_t);
   int ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &set_t);
   if (ret != 0) {
      DEBUG_ERR("NO CORE!\n");
   }
}

void onion_worker_release(struct onion_worker *work) {
   if (work->server_sock) {
      onion_server_sock_release(work->server_sock);
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
   work->server_sock = NULL;
   free(work);
}

struct onion_server_sock *onion_server_sock_create(struct onion_tcp_port_conf *port_conf, struct onion_worker *work, int work_index, int peer_capable) {
   int ret;

   if (!port_conf->port || !port_conf->domain || !port_conf->type) {
      DEBUG_FUNC("Invalid port configuration provided.\n");
      goto free_this_trash;
   }

   struct onion_server_sock *server_sock = malloc(sizeof(struct onion_server_sock));
   if (!server_sock) {
      DEBUG_ERR("Failed to allocate memory for server socket.\n");
      goto free_this_trash;
   }

   server_sock->initialized = false;
   server_sock->released = false;
   server_sock->onion_worker = work;
   server_sock->onion_worker_index = work_index;
   server_sock->peer_current = 0;
   server_sock->peer_capable = peer_capable;
   server_sock->peer_queue_capable = 30;

   ret = onion_block_init(&server_sock->peer_pool, ONION_MAX_CLIENTS_CAPABLE * sizeof(struct onion_peer_sock), sizeof(struct onion_peer_sock));
   if (ret < 0) {
      DEBUG_FUNC("Client memory pool initialization failed.\n");
      goto free_everything;
   }

   server_sock->request_allocator = onion_slab_memory_init(100000);
   if (!server_sock->request_allocator) {
      DEBUG_FUNC("request_allocator initialization failed.\n");
      goto free_everything;
   }

   server_sock->request_msg_allocator = onion_slab_memory_init(100000);
   if (!server_sock->request_msg_allocator) {
      DEBUG_FUNC("request_msg_allocator initialization failed.\n");
      goto free_everything;
   }

   server_sock->peerIDs_size = (server_sock->peer_capable + 63) / 64;
   server_sock->peerIDs = malloc(server_sock->peerIDs_size);
   if (!server_sock->peerIDs) {
      DEBUG_ERR("Failed to allocate memory for peer ID bitmap.\n");
      goto free_everything;
   }
   memset(server_sock->peerIDs, 0, server_sock->peerIDs_size);

   ret = onion_net_sock_init(&server_sock->sock, port_conf, ONION_PEER_MAX_QUEUE_CAPABLE);
   if (ret < 0) {
      DEBUG_FUNC("Socket initialization failed.\n");
      goto free_everything;
   }

   work->event.events = EPOLLIN | EPOLLOUT | EPOLLET;
   work->event.data.ptr = server_sock;

   ret = epoll_ctl(server_sock->onion_worker->epoll_fd, EPOLL_CTL_ADD, server_sock->sock.fd, &work->event);
   if (ret < 0) {
      DEBUG_ERR("Failed to add socket to epoll.\n");
      goto free_everything;
   }

   INIT_LIST_HEAD(&server_sock->peers);
   server_sock->initialized = true;

   DEBUG_FUNC("Server socket created successfully. Descriptor: %d\n", server_sock->sock.fd);
   return server_sock;

   free_everything:
   onion_server_sock_release(server_sock);
   free_this_trash:
   return NULL;
}

void onion_server_sock_release(struct onion_server_sock *server_sock) {
   int ret;

   if (server_sock->released) {
      return;
   }

   server_sock->released = true;
   server_sock->initialized = false;

   ret = epoll_ctl(server_sock->onion_worker->epoll_fd, EPOLL_CTL_DEL, server_sock->sock.fd, &server_sock->onion_worker->event);
   if (ret < 0 && errno != ENOENT) {
      DEBUG_ERR("Failed to remove fd from epoll: %s\n", strerror(errno));
   }

   if (server_sock->peerIDs) {
      free(server_sock->peerIDs);
      server_sock->peerIDs = NULL;
   }

   if (server_sock->peer_pool) {
      struct onion_peer_sock *pos, *temp;
      list_for_each_entry_safe(pos, temp, &server_sock->peers, list) {
         onion_peer_sock_release(server_sock, pos);
      }
      free(server_sock->peer_pool);
      server_sock->peer_pool = NULL;
   }

   onion_net_sock_exit(&server_sock->sock);
   free(server_sock);
}

struct onion_peer_sock *onion_peer_sock_create(struct onion_server_sock *server_sock, int fd) {
   int ret;

   struct onion_peer_sock *peer = (struct onion_peer_sock *)onion_block_alloc(server_sock->peer_pool, fd);
   if (!peer) {
      DEBUG_ERR("failed to allocate peer socket from peer pool\n");
      goto free_this_trash;
   }

   peer->onion_worker = server_sock->onion_worker;
   peer->proto_type = ONION_PEER_PROTO_TYPE_HTTP;

   onion_http_parser_t *parser = &peer->parser;

   if (peer->proto_type == ONION_PEER_PROTO_TYPE_HTTP) {
      ret = onion_http_parser_init(parser, server_sock->request_allocator, server_sock->request_msg_allocator);
      if (ret < 0) {
         goto free_everything;
      }
   }

   peer->sock.fd = fd;
   peer->released = false;
   peer->initialized = true;

   list_add_tail(&peer->list, &server_sock->peers);

   return peer;

   free_everything:
   onion_peer_sock_release(server_sock, peer);
   free_this_trash:
   return NULL;
}

void onion_peer_sock_release(struct onion_server_sock *server_sock, struct onion_peer_sock *peer) {
   if (peer->released) {
      return;
   }

   list_del(&peer->list);
   peer->released = true;

   onion_http_parser_exit(&peer->parser);

   if (peer->sock.fd > 0) {
      struct onion_worker *work = peer->onion_worker;
      if (work) {
         epoll_ctl(work->epoll_fd, EPOLL_CTL_DEL, peer->sock.fd, NULL);
         atomic_fetch_sub(&work->peer_count, 1);
      }
      onion_net_sock_exit(&peer->sock);
   }

   onion_block_free(server_sock->peer_pool, peer);
   DEBUG_FUNC("Peer fd %d released on core %d\n", peer->sock.fd, peer->onion_worker ? peer->onion_worker->core_id : -1);
}

int onion_server_sock_accept(struct onion_server_sock *server_sock) {
   int ret = -1;
   if (!server_sock) {
      DEBUG_FUNC("no server socket provided\n");
      goto free_this_trash;
   }
   DEBUG_FUNC("hello1\n");
   struct onion_net_sock onion_peer_sock;
   ret = onion_net_sock_accept(&server_sock->sock, &onion_peer_sock);
   if (ret < 0) {
      DEBUG_FUNC("Peer accepting failed.\n");
      goto free_this_trash;
   }
   DEBUG_FUNC("hello122\n");
   struct onion_peer_sock *peer = onion_peer_sock_create(server_sock, onion_peer_sock.fd);
   if (!peer) {
      DEBUG_ERR("peer initialization failed for fd %d\n", onion_peer_sock.fd);
      goto free_sock;
   }

   peer->sock = onion_peer_sock;

   struct epoll_event peer_event;
   peer_event.events = EPOLLIN | EPOLLOUT | EPOLLET;
   peer_event.data.ptr = peer;

   ret = epoll_ctl(server_sock->onion_worker->epoll_fd, EPOLL_CTL_ADD, peer->sock.fd, &peer_event);
   if (ret == -1) {
      DEBUG_ERR("failed to add peer fd %d to epoll: %s\n", peer->sock.fd, strerror(errno));
      goto free_everything;
   }

   DEBUG_FUNC("new peer fd %d added to epoll\n", peer->sock.fd);

   return peer->sock.fd;
   free_everything:
   onion_peer_sock_release(server_sock, peer);
   return ret;
   free_sock:
   close(onion_peer_sock.fd);
   free_this_trash:
   return ret;
}

int onion_sock_core_init() {
   int max_peers = ONION_MAX_CLIENTS_CAPABLE / onion_dev_core_count;
   int extra = ONION_MAX_CLIENTS_CAPABLE % onion_dev_core_count;

   server_sock_onion_workers = malloc(sizeof(struct onion_worker *) * onion_dev_core_count);
   if (!server_sock_onion_workers) {
      DEBUG_INFO("no onion_workers\n");
      goto free_this_trash;
   }

   struct onion_tcp_port_conf port_conf = {
      .domain = AF_INET,
      .type = SOCK_STREAM,
      .port = htons(51235),
      .addr.s_addr = htonl(INADDR_ANY)
   };

   for (int work_index = 0; work_index < onion_dev_core_count; work_index++) {
      server_sock_onion_workers[work_index] = malloc(sizeof(struct onion_worker));
      if (!server_sock_onion_workers[work_index]) {
         DEBUG_ERR("no malloc onion_worker\n");
         goto free_everything;
      }

      struct onion_worker *work = server_sock_onion_workers[work_index];

      work->epoll_fd = epoll_create1(0);
      if (work->epoll_fd < 0) {
         DEBUG_ERR("flows werent inited\n");
         goto free_everything;
      }

      int capable = max_peers + (work_index < extra ? 1 : 0);

      struct onion_server_sock *server_sock = onion_server_sock_create(&port_conf, work, work_index, capable);
      if (!server_sock) {
         DEBUG_INFO("where my sock!!\n");
         goto free_everything;
      }

      work->server_sock = server_sock;
      work->core_id = work_index;
      work->peer_count = 0;
      work->event.events = EPOLLIN | EPOLLET;

      int thread_ret = pthread_create(&work->core.flow, NULL, onion_worker_thread, work);
      if (thread_ret != 0) {
         DEBUG_ERR("failed to create onion_worker thread %d: %s\n", work_index, strerror(thread_ret));
         goto free_everything;
      }
      onion_set_onion_worker_affinity(work->core.flow, work->core_id);
   }
   return 0;
   free_everything:
   onion_sock_core_exit();
   free_this_trash:
   return -1;
}

void onion_sock_core_exit() {
   for (int work_index = 0; work_index < onion_dev_core_count; work_index++) {
      struct onion_worker *work = server_sock_onion_workers[work_index];
      if (!work) {
         DEBUG_ERR("work wasnt found in server_sock_onion_workers\n");
         return;
      }
      onion_worker_release(work);
   }
}

int onion_sock_syst_init(void) {
   int ret = -1;

   ret = onion_sock_core_init();
   if (ret < 0) {
      DEBUG_FUNC("no sock cores inited\n");
      goto free_this_trash;
   }

   return 0;

   free_this_trash:
   return ret;
}

void onion_sock_syst_exit(struct onion_server_sock *server_sock) {
   onion_sock_core_exit();
}

