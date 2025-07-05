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

int onion_worker_stack_push(onion_worker_stack *stack, onion_server_net_conf *conf, void *ptr, long per_size) {
   long max_size = conf->peers_capable;
   long queue_capable = conf->queue_capable;
   struct onion_tcp_port_conf port_conf = conf->port_conf;

   int peers_per_core = per_size / max_size;
   int peers_remain = per_size % max_size;

   for (long index = 0; index < max_size; index++) {
      int peers_for_this_core = peers_per_core + (index < peers_remain ? 1 : 0);
      int success = onion_dev_worker_init(ptr, port_conf, peers_for_this_core, queue_capable);
      if (success < 0) {
         DEBUG_FUNC("SUCCESS FAIL!!!\n");
         return -1;
      }
   }

   return 0;
}

int onion_static_worker_stack_init(onion_worker_stack *stack, onion_server_net_conf conf, onion_thread_func_t func, void *ptr) {
   int ret;

   long capable = conf.peers_capable;

   stack->epoll_static = onion_epoll_static_init(capable);
   if (!stack->epoll_static) {
      DEBUG_ERR("onion_epoll_static_t initialization failed.\n");
      goto unsuccessfull;
   }

   ret = onion_net_static_init(&stack->net_static, capable);
   if (ret < 0) {
      DEBUG_ERR("onion_net_static_t initialization failed.\n");
      goto please_free;
   }

   ret = pthread_create(&stack->flow, NULL, func, ptr);
   if (ret < 0) {
      DEBUG_ERR("pthread_create faield.\n");
      goto please_free;
   }
   
   stack->conf = conf;

   return 0;
please_free:
   onion_static_worker_stack_exit(stack);
unsuccessfull:
   return -1;
}

void onion_static_worker_stack_exit(onion_worker_stack *stack) {
   if (stack->flow) {
      pthread_join(stack->flow, NULL);
      stack->flow = 0;
   }
   if (stack->net_static) {
      onion_net_static_exit(stack->net_static);
      stack->net_static = NULL;
   }
   if (stack->epoll_static) {
      onion_epoll_static_exit(stack->epoll_static);
      stack->epoll_static = NULL;
   }
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

static int onion_device_check_args(uint16_t port, long core_count, int peers_capable) {
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
   return 0;
}

struct onion_worker_head *onion_device_init(uint16_t port, long core_count, int peers_capable, int queue_capable) {
   int ret;

   if (onion_device_check_args(port, core_count, peers_capable) == -1) {
      return NULL;
   }

   struct onion_tcp_port_conf port_conf = {
      .domain = AF_INET,
      .type = SOCK_STREAM,
      .port = htons(port),
      .addr.s_addr = htonl(INADDR_ANY)
   };

   struct onion_worker_head *head = malloc(sizeof(struct onion_worker_head));
   if (!head) {
      DEBUG_ERR("head initialization failed.\n");
      return NULL;
   }
   memset(head, 0, sizeof(*head));

   head->count = 0;
   head->capable = core_count > 1 ? core_count - 1 : core_count;

   size_t workers_max_size = sizeof(struct onion_worker) * core_count;
   size_t workers_per_size = sizeof(struct onion_worker);

   ret = onion_block_init(&head->workers, workers_max_size, workers_per_size);
   if (ret < 0) {
      DEBUG_ERR("head pool initialization failed.\n");
      goto unsuccessfull;
   }

   if (onion_static_worker_stack_init(&head->stack, , onion_device_head_flow, head) == -1) {
      DEBUG_ERR("onion_epoll_static_t initialization failed.\n");
      goto unsuccessfull;
   }

   if (onion_worker_stack_push(&head->stack, conf, void *ptr, long per_size))

      DEBUG_FUNC("head size : %zu, epoll %zu, netstat: %zu\n", sizeof(*head));

   return head;
unsuccessfull:
   onion_device_exit(head);
   return NULL;
}

void onion_device_exit(struct onion_worker_head *head) {
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
