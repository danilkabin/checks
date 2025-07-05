#define _GNU_SOURCE

#include "onion.h"
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

struct onion_worker_head *big_smoke;

struct onion_worker_head *get_worker_head_by_worker(struct onion_worker *worker) {
   return big_smoke;
}

struct onion_worker *onion_get_worker_by_epoll(onion_epoll_t *target) {
   for (long index = 0; index < big_smoke->capable; index++) {
      struct onion_worker *worker = (struct onion_worker*)onion_block_get(big_smoke->workers, index); 
      if (worker->epoll == target) {
         return worker;
      }
   }
   return NULL;
}

int onion_accept_net(onion_server_net *net_server, onion_epoll_t *epoll) {
   int ret;

   struct onion_net_sock temp_sock;
   ret = onion_net_sock_accept(net_server->sock, &temp_sock);
   if (ret < 0) {
      DEBUG_ERR("Peer finding failed.\n");
      goto unsuccessfull;
   }

   onion_peer_net *peer = onion_peer_net_init(net_server, &temp_sock);
   if (!peer) {
      DEBUG_ERR("Peer init failed!\n");
      goto unsuccessfull;
   }

   onion_epoll_slot_t *slot = onion_epoll_slot_add(epoll, peer->sock->fd, peer, NULL, NULL, NULL);
   if (!slot) {
      DEBUG_ERR("Slot initialization failed.\n");
      goto free_peer;
   }

   DEBUG_FUNC("Peer inited!: %d current peers: %d\n", epoll->core, epoll->conn_count);

   return 0;
free_peer:
   onion_peer_net_exit(net_server, peer);
unsuccessfull:
   return -1;
}

void *onion_device_head_flow(void *arg) {
   struct onion_worker_head *head = (struct onion_worker_head*)arg;
   while (1) {
      usleep(10000);
      int ret;
      
      onion_server_net *net = onion_get_weak_net(head->net_static);
      if (!net) {
         continue;
      }
      
      onion_epoll_t *epoll = net->epoll;
      
      ret = onion_accept_net(net, epoll);
      if (ret < 0) {
         DEBUG_FUNC("No client!\n");
         continue;
      }  

      DEBUG_FUNC("New client\n");
   }
   return NULL;
}

void *onion_dev_handler(struct onion_thread_my_args *args) {
   int ret;
   onion_epoll_static_t *epoll_static = args->ep_st;
   onion_epoll_t *epoll = args->ep;

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
   onion_net_static_t *net_static = onion_get_static_by_net(net);

   long core = sched_getcpu();
   //DEBUG_FUNC("CORE: %ld\n", core);
   for (int index = 0; index < net_static->capable; index++) {
      onion_server_net *netka = onion_block_get(net_static->nets, index);
   }

   return NULL;
}

int onion_dev_worker_init(struct onion_worker_head *head, struct onion_tcp_port_conf port_conf, int peers_capable, int queue_capable) {
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

   struct onion_worker *worker = onion_block_alloc(head->workers, NULL);
   if (!worker) {
      DEBUG_FUNC("Worker initialization failed.\n");
      goto unsuccessfull;
   }

   onion_epoll_t *epoll = onion_slave_epoll1_init(head->epoll_static, onion_dev_handler, onion_config.sched_core, conf.peers_capable);
   if (!epoll) {
      DEBUG_FUNC("Epoll initialization failed.\n");
      goto free_worker;
   }

   onion_server_net *net_server = onion_server_net_init(head->net_static, epoll, &conf);
   if (!net_server) {
      DEBUG_FUNC("Server socket initialization failed.\n");
      goto free_worker;
   } 

   worker->epoll = epoll;
   worker->server_sock = net_server;
   head->count = head->count + 1;

   DEBUG_FUNC("Worker allocated! Peer max: %d\n", peers_capable);
   return 0;
free_worker:
   onion_dev_worker_exit(head, worker);
unsuccessfull:
   return -1;
}

void onion_dev_worker_exit(struct onion_worker_head *head, struct onion_worker *worker) {
   onion_epoll_t *epoll = worker->epoll;
   if (epoll && epoll->initialized) {
      onion_epoll_static_t *epoll_static = onion_get_static_by_epoll(epoll);
      onion_slave_epoll1_exit(epoll_static, epoll); 
   }

   onion_server_net *net = worker->server_sock;
   if (net && net->initialized) {
      onion_net_static_t *net_static = onion_get_static_by_net(net);
      onion_server_net_exit(net_static, net);
   }

   onion_block_free(head->workers, worker);
}

int onion_device_init(struct onion_worker_head **ptr, uint16_t port, long core_count, int peers_capable, int queue_capable) {
   int ret;

   if (big_smoke) {
      DEBUG_ERR("Big smoke already existing!\n");
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

   struct onion_worker_head *head = malloc(sizeof(struct onion_worker_head));
   if (!head) {
      DEBUG_ERR("head initialization failed.\n");
      return -1;
   }

   ret = onion_block_init(&head->workers, sizeof(struct onion_worker) * core_count, sizeof(struct onion_worker));
   if (ret < 0) {
      DEBUG_ERR("head pool initialization failed.\n");
      goto unsuccessfull;
   }

   head->count = 0;
   head->capable = core_count > 1 ? core_count - 1 : core_count;

   ret = onion_epoll_static_init(&head->epoll_static, head->capable);
   if (ret < 0) {
      DEBUG_ERR("onion_epoll_static_t initialization failed.\n");
      goto unsuccessfull;
   }

   ret = onion_net_static_init(&head->net_static, head->capable);
   if (ret < 0) {
      DEBUG_ERR("onion_net_static_t initialization failed.\n");
      goto unsuccessfull;
   }

   ret = pthread_create(&head->flow, NULL, onion_device_head_flow, head);
   if (ret < 0) {
      DEBUG_ERR("pthread_create faield.\n");
      goto unsuccessfull;
   }

   DEBUG_FUNC("head size : %zu, epoll %zu, netstat: %zu\n", sizeof(*head), sizeof(*head->epoll_static), sizeof(*head->net_static));
   struct onion_tcp_port_conf port_conf = {
      .domain = AF_INET,
      .type = SOCK_STREAM,
      .port = htons(port),
      .addr.s_addr = htonl(INADDR_ANY)
   };

   int peers_per_core = peers_capable / head->capable;
   int peers_remain = peers_capable % head->capable;

   for (long index = 0; index < head->capable; index++) {
      int peers_for_this_core = peers_per_core + (index < peers_remain ? 1 : 0);
      int success = onion_dev_worker_init(head, port_conf, peers_for_this_core, queue_capable);
      if (success < 0) {
         DEBUG_FUNC("SUCCESS FAIL!!!\n");
         goto unsuccessfull;
      }
   }

   big_smoke = head;
   *ptr = head;
   return 0;
unsuccessfull:
   onion_device_exit(head);
   return -1;
}

void onion_device_exit(struct onion_worker_head *head) {
   big_smoke = NULL;

   if (head->flow) {
      pthread_join(head->flow, NULL);
      head->flow = 0;
   }

   for (int index = 0; index < head->capable; index++) {
      struct onion_worker *worker = onion_block_get(head->workers, index);
      if (!worker) {
         continue;
      }
      onion_dev_worker_exit(head, worker);
   }

   if (head->net_static) {
      onion_net_static_exit(head->net_static);
      head->net_static = NULL;
   }

   if (head->epoll_static) {
      onion_epoll_static_exit(head->epoll_static);
      head->epoll_static = NULL;
   }

   free(head);
}
