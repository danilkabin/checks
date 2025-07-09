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

int onion_conf_triad_init(onion_server_conf_triad_t *triad) {
   triad->core = malloc(sizeof(onion_core_conf_t));
   triad->epoll = malloc(sizeof(onion_epoll_conf_t));
   triad->net = malloc(sizeof(onion_net_conf_t));
   if (!triad->core || !triad->epoll || !triad->net) {
      onion_conf_triad_exit(triad);
      return -1;
   }
   return 0;
}

void onion_conf_triad_exit(onion_server_conf_triad_t *triad) {
   if (triad->core) {
      free(triad->core);
      triad->core = NULL;
   }
   if (triad->epoll) {
      free(triad->epoll);
      triad->epoll = NULL;
   }
   if (triad->net) {
      free(triad->net);
      triad->net = NULL;
   }
}

int onion_core_conf_init(onion_core_conf_t *core_conf) {
   int core_count;
   int core_sched;
   if (core_conf->count < 0) {
      core_count = sysconf(_SC_NPROCESSORS_CONF);
      if (core_count < 1) {
         DEBUG_FUNC("onion_config_init: no CPU cores found (core_count = %d)\n", core_count);
         return -1;
      }
      core_conf->count = core_count;
   }

   if (core_conf->sched < 0) {
      core_sched = ONION_DEFAULT_CORE_SCHED;
      if (onion_cpu_set_core(pthread_self(), core_sched) == -1) {
         DEBUG_FUNC("failed to set CPU affinity.\n");
         return -1;
      }

      if (core_sched >= core_count) {
         DEBUG_FUNC("sched_core (%d) >= core_count (%d)\n", core_sched, core_count);
         return -1;
      }
      core_conf->sched = core_sched;
   }

   return 0;
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

   onion_epoll_slot_t *slot = onion_epoll_add_slot(epoll, peer->sock->fd, peer, NULL, NULL, NULL);
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
  /* while (1) {
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
   }*/
   return NULL;
}

void *onion_dev_handler(struct onion_thread_my_args *args) {
 /*  int ret;
   onion_epoll_static_t *epoll_static = args->epoll_static;
   onion_epoll_t *epoll = args->epoll;

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
*/
   return NULL;
}

int onion_static_worker_stack_init(struct onion_worker_head *head, onion_thread_func_t func, void *ptr) {
   int ret;
   onion_worker_stack *stack = &head->stack;
   ONION_UNPACK_TRIAD_FULL(head->triad_conf);

   stack->epoll_static = onion_epoll_static_init(epoll_conf, core_conf->count);
   if (!stack->epoll_static) {
      DEBUG_ERR("onion_epoll_static_t initialization failed.\n");
      goto unsuccessfull;
   }

   stack->net_static = onion_net_static_init(net_conf, core_conf->count);
   if (!stack->net_static) {
      DEBUG_ERR("onion_net_static_t initialization failed.\n");
      goto please_free;
   }

   ret = pthread_create(&stack->flow, NULL, func, ptr);
   if (ret < 0) {
      DEBUG_ERR("pthread_create faield.\n");
      goto please_free;
   }

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

int onion_worker_stack_push(struct onion_worker_head *head) {
   onion_worker_stack *stack = &head->stack;
   ONION_UNPACK_TRIAD_FULL(head->triad_conf);

   long max_peers = net_conf->max_peers;
   long max_queue = net_conf->max_queue;

   int peers_per_core = max_peers / head->capable;
   int peers_remain = max_peers % head->capable;

   for (long index = 0; index < max_peers; index++) {
      int peers_for_this_core = peers_per_core + (index < peers_remain ? 1 : 0);
      int success = onion_dev_worker_init(head, peers_for_this_core);
      if (success < 0) {
         DEBUG_FUNC("SUCCESS FAIL!!!\n");
         return -1;
      }
   }

   return 0;
}

int onion_dev_worker_init(struct onion_worker_head *head, int max_peers) {
   int ret;
   
   onion_worker_stack *stack = &head->stack;
   ONION_UNPACK_TRIAD_FULL(head->triad_conf);
   
   struct onion_worker *worker = onion_block_alloc(head->workers, NULL);
   if (!worker) {
      DEBUG_FUNC("Worker initialization failed.\n");
      goto unsuccessfull;
   }

   worker->epoll = onion_slave_epoll1_init(stack->epoll_static, onion_dev_handler, core_conf->sched, max_peers);
   if (!worker->epoll) {
      DEBUG_FUNC("Epoll initialization failed.\n");
      goto free_worker;
   }

   worker->server_sock = onion_server_net_init(stack->net_static);
   if (!worker->server_sock) {
      DEBUG_FUNC("Server socket initialization failed.\n");
      goto free_worker;
   } 

   head->count = head->count + 1;

   DEBUG_FUNC("Worker allocated! Peer max: %d\n", max_peers);
   return 0;
free_worker:
   onion_dev_worker_exit(head, worker);
unsuccessfull:
   return -1;
}

void onion_dev_worker_exit(struct onion_worker_head *head, struct onion_worker *worker) {
   onion_epoll_t *epoll = worker->epoll;
   if (epoll && epoll->initialized) {
      onion_epoll_static_t *epoll_static = onion_epoll_priv(epoll);
      onion_slave_epoll1_exit(epoll_static, epoll); 
   }

   onion_server_net *net = worker->server_sock;
   if (net && net->initialized) {
      onion_net_static_t *net_static = onion_net_priv(net);
      onion_server_net_exit(net_static, net);
   }

   onion_block_free(head->workers, worker);
}

struct onion_worker_head *onion_device_init(onion_server_conf_triad_t *triad_conf) {
   int ret;
   ONION_UNPACK_TRIAD(triad_conf);
   
   struct onion_worker_head *head = malloc(sizeof(struct onion_worker_head));
   if (!head) {
      DEBUG_ERR("head initialization failed.\n");
      return NULL;
   }
   memset(head, 0, sizeof(*head));

   onion_worker_stack *stack = &head->stack;

   head->triad_conf = triad_conf;
   head->count = 0;
   head->capable = core_conf->count > 1 ? core_conf->count - 1 : core_conf->count;

   size_t worker_size = sizeof(struct onion_worker);
   size_t total_size  = worker_size * core_conf->count;

   ret = onion_block_init(&head->workers, total_size, worker_size);
   if (ret < 0) {
      DEBUG_ERR("head pool initialization failed.\n");
      goto unsuccessfull;
   }

   if (onion_static_worker_stack_init(head, onion_device_head_flow, head) == -1) {
      DEBUG_ERR("onion worker stack initialization failed.\n");
      goto unsuccessfull;
   }

   if (onion_worker_stack_push(head) == -1) {
      DEBUG_ERR("onion worker stack push failed.\n");
      goto unsuccessfull;
   }

   //DEBUG_FUNC("head size : %zu, epoll %zu, netstat: %zu\n", sizeof(*head));

   return head;
unsuccessfull:
   onion_device_exit(head);
   return NULL;
}

void onion_device_exit(struct onion_worker_head *head) {
   for (int index = 0; index < head->capable; index++) {
      struct onion_worker *worker = onion_block_get(head->workers, index);
      if (!worker) {
         continue;
      }
      onion_dev_worker_exit(head, worker);
   }

   onion_static_worker_stack_exit(&head->stack);

   free(head);
}
