#define _GNU_SOURCE

#include "epoll.h"
#include "onion.h"
#include "net.h"
#include "parser.h"
#include "slab.h"
#include "listhead.h"
#include "lock.h"
#include "device.h"
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

struct onion_worker_pool *onion_workers = NULL;

/*struct onion_server_sock *onion_server_sock_create(struct onion_tcp_port_conf *port_conf, struct onion_worker *work, int work_index, int peer_capable) {
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
}*/

void *handler(void *arg, onion_epoll_tag_t *tag, onion_handler_ret_t handler_ret) {
return NULL;
} 

int onion_dev_worker_init(struct onion_worker_pool *onion_workers, int peers_capable) {
   int ret;
   struct onion_worker *worker = onion_block_alloc(onion_workers->workers, -1);
   if (!worker) {
      DEBUG_FUNC("Worker initialization failed.\n");
      return -1;
   }
   
   onion_epoll_t *epoll = onion_epoll1_init(onion_workers->epoll_static, handler, peers_capable);
   if (!epoll) {
      DEBUG_FUNC("Epoll initialization failed.\n");
      return -1;
   }

   onion_workers->count = onion_workers->count + 1;

   DEBUG_FUNC("Worked allocated! Peer max: %d\n", peers_capable);
   return 0;
}

void onion_dev_worker_exit() {

}

int onion_device_init(uint16_t port, long core_count, int peers_capable) {
   int ret;
   if (core_count < 1) {
      DEBUG_ERR("Core count is less than 1. Please initialize onion_config.\n");
      return -1;
   }

   if (peers_capable < 1) {
      DEBUG_ERR("Peers capable is less than 1.\n");
      return -1;
   }

   if (onion_workers) {
      DEBUG_ERR("The onion_workers has already been initialized.\n");
      return -1;
   }

   if (onion_net_port_check(port) <= 0) {
      DEBUG_ERR("Port is invalid or already in use.\n");
      return -1;
   }

   onion_workers = malloc(sizeof(struct onion_worker_pool));
   if (!onion_workers) {
      DEBUG_ERR("Onion_workers initialization failed.\n");
      return -1;
   }

   ret = onion_block_init(&onion_workers->workers, sizeof(struct onion_worker) * core_count, sizeof(struct onion_worker));
   if (ret < 0) {
      DEBUG_ERR("Onion_workers initialization failed.\n");
      goto unsuccessfull;
   }

   onion_workers->count = 0;
   onion_workers->capable = core_count;
   onion_workers->peers_capable = peers_capable;

   ret = onion_epoll_static_init(&onion_workers->epoll_static, onion_workers->capable);
   if (ret < 0) {
      DEBUG_ERR("onion_epoll_static_t initialization failed.\n");
      goto unsuccessfull;
   } 

   struct onion_tcp_port_conf port_conf = {
      .domain = AF_INET,
      .type = SOCK_STREAM,
      .port = htons(port),
      .addr.s_addr = htonl(INADDR_ANY)
   };

   int peers_per_core = onion_workers->peers_capable / onion_workers->capable;
   int peers_remain = onion_workers->peers_capable % onion_workers->capable;

   for (long index = 0; index < onion_workers->capable ; index++) {
      int peers_for_this_core = peers_per_core + (index < peers_remain ? 1 : 0);
      int success = onion_dev_worker_init(onion_workers, peers_for_this_core);
   }

   return 0;
unsuccessfull:
   onion_device_exit();
   return -1;
}

void onion_device_exit() {

}
