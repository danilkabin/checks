#define _GNU_SOURCE

#include "net.h"
#include "epoll.h"
#include "socket.h"
#include "device.h"
#include "pool.h"
#include "utils.h"

#include <arpa/inet.h>
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

/*void onion_server_sock_release(struct onion_server_sock *server_sock) {
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

struct onion_worker_head *big_smoke;

struct onion_worker *onion_get_worker_by_epoll(onion_epoll_t *target) {
   for (long index = 0; index < big_smoke->capable; index++) {
      struct onion_worker *worker = (struct onion_worker*)onion_block_get(big_smoke->workers, index); 
      if (worker->epoll == target) {
         return worker;
      }
   }
   return NULL;
}

void *onion_dev_handler(struct onion_thread_my_args *args) {
   int ret;
   onion_epoll_static_t *epoll_static = args->ep_st;
   onion_epoll_t *epoll = args->ep;
   onion_epoll_tag_t *tag = args->tag;
   onion_handler_ret_t tag_ret = args->tag_ret;

   struct onion_worker *worker = onion_get_worker_by_epoll(epoll);
   if (!worker) {
      DEBUG_ERR("Worker finding failed.\n");
      return NULL;
   }

   if (!worker->epoll || !worker->server_sock) {
      DEBUG_ERR("Worker is dolbaeb!\n");
      return NULL;
   }

   onion_server_net *net = worker->server_sock;
   ret = onion_accept_net(net);
   if (ret < 0) {
      //DEBUG_FUNC("No client!\n");
      return NULL;      
   } 

   DEBUG_FUNC("CLIENTTT!\n");
   DEBUG_FUNC("args: %ld worker data: %d\n", epoll_static->capable, worker->epoll->conn_max);
   return NULL;
}

int onion_dev_worker_init(struct onion_worker_head *onion_workers, struct onion_tcp_port_conf port_conf, int peers_capable, int queue_capable) {
   int ret;

   if (peers_capable < 1) {
      DEBUG_ERR("Peers capable is less than 1.\n");
      return -1;
   }

   if (queue_capable < 1) {
      DEBUG_ERR("Queue capable is less than 1.\n");
      return -1;
   }

   onion_server_net_conf conf = {
      .peers_capable = peers_capable,
      .queue_capable = queue_capable,
      .port_conf = port_conf
   };

   struct onion_worker *worker = onion_block_alloc(onion_workers->workers, NULL);
   if (!worker) {
      DEBUG_FUNC("Worker initialization failed.\n");
      goto unsuccessfull;
   }

   onion_epoll_t *epoll = onion_epoll1_init(onion_workers->epoll_static, onion_dev_handler, conf.peers_capable);
   if (!epoll) {
      DEBUG_FUNC("Epoll initialization failed.\n");
      goto free_worker;
   }

   onion_server_net *net_server = onion_server_net_init(onion_workers->net_static, &conf);
   if (!net_server) {
      DEBUG_FUNC("Server socket initialization failed.\n");
      goto free_worker;
   } 

   worker->epoll = epoll;
   worker->server_sock = net_server;
   onion_workers->count = onion_workers->count + 1;

   DEBUG_FUNC("Worked allocated! Peer max: %d\n", peers_capable);
   return 0;
free_worker:
   onion_dev_worker_exit(onion_workers, worker);
unsuccessfull:
   return -1;
}

void onion_dev_worker_exit(struct onion_worker_head *head, struct onion_worker *worker) {

}

int onion_device_init(struct onion_worker_head **head, uint16_t port, long core_count, int peers_capable, int queue_capable) {
   int ret;

   if (big_smoke) {
      DEBUG_ERR("Head already initialized!!!\n");
      return -1;
   }

   if (core_count < 1) {
      DEBUG_ERR("Core count is less than 1. Please initialize onion_config.\n");
      return -1;
   }

   if (peers_capable < 1) {
      DEBUG_ERR("Peers capable is less than 1.\n");
      return -1;
   }

   if (onion_net_port_check(port) <= 0) {
      DEBUG_ERR("Port is invalid or already in use.\n");
      return -1;
   }

   struct onion_worker_head *onion_workers = malloc(sizeof(struct onion_worker_head));
   if (!onion_workers) {
      DEBUG_ERR("Onion_workers initialization failed.\n");
      return -1;
   }

   ret = onion_block_init(&onion_workers->workers, sizeof(struct onion_worker) * core_count, sizeof(struct onion_worker));
   if (ret < 0) {
      DEBUG_ERR("Onion_workers pool initialization failed.\n");
      goto unsuccessfull;
   }

   onion_workers->count = 0;
   onion_workers->capable = core_count;

   ret = onion_epoll_static_init(&onion_workers->epoll_static, onion_workers->capable);
   if (ret < 0) {
      DEBUG_ERR("onion_epoll_static_t initialization failed.\n");
      goto unsuccessfull;
   }

   ret = onion_net_static_init(&onion_workers->net_static, onion_workers->capable);
   if (ret < 0) {
      DEBUG_ERR("onion_net_static_t initialization failed.\n");
      goto unsuccessfull;
   } 

   DEBUG_FUNC("onion_workers size : %zu, epoll %zu, netstat: %zu\n", sizeof(*onion_workers), sizeof(*onion_workers->epoll_static), sizeof(*onion_workers->net_static));
   struct onion_tcp_port_conf port_conf = {
      .domain = AF_INET,
      .type = SOCK_STREAM,
      .port = htons(port),
      .addr.s_addr = htonl(INADDR_ANY)
   };

   int peers_per_core = peers_capable / onion_workers->capable;
   int peers_remain = peers_capable % onion_workers->capable;

   for (long index = 0; index < onion_workers->capable ; index++) {
      int peers_for_this_core = peers_per_core + (index < peers_remain ? 1 : 0);
      int success = onion_dev_worker_init(onion_workers, port_conf, peers_for_this_core, queue_capable);
      if (success < 0) {
         DEBUG_FUNC("Fail!!!!!1\n");
         goto unsuccessfull;
      }
   }

   big_smoke = onion_workers;
   *head = onion_workers;
   return 0;
unsuccessfull:
   onion_device_exit(onion_workers);
   return -1;
}

void onion_device_exit(struct onion_worker_head *head) {

   for (int index = 0; index < head->capable; index++) {
      struct onion_worker *worker = onion_block_get(head->workers, index);
      if (!worker) {
         continue;
      }
      onion_epoll_t *epoll = worker->epoll;
   }

   free(head);
   big_smoke = NULL;
}
