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
   if (triad->core) {free(triad->core); triad->core = NULL;}
   if (triad->epoll) {free(triad->epoll); triad->epoll = NULL;}
   if (triad->net) {free(triad->net); triad->net = NULL;}
}

int onion_worker_create(struct onion_worker_t *worker, struct onion_net_sock *temp_sock) {
   int ret;

   ONION_WORK_UNPACK(worker);

   onion_peer_net *peer = onion_peer_net_init(net, temp_sock);
   if (!peer) {
      DEBUG_ERR("Peer init failed!\n");
      goto unsuccessfull;
   }

   onion_epoll_slot_t *slot = onion_epoll_add_slot(epoll, peer->sock->fd, peer, NULL, NULL, NULL);
   if (!slot) {
      DEBUG_ERR("Slot initialization failed.\n");
      goto free_peer;
   }

 /*  for (size_t i = 0; i < epoll->slots->bitmask->conv_size; ++i) {
      printf("Frame %zu: ", i);
      for (int b = 63; b >= 0; --b) {
         putchar((epoll->slots->bitmask->mask[i] & (1ULL << b)) ? '1' : '0');
      }
      printf("\n");
   }*/
   DEBUG_FUNC("Peer inited!: %d current peers: %d\n", epoll->core, epoll->conn_count);

   return 0;
free_peer:
   //onion_peer_net_exit(net, peer);
unsuccessfull:
   return -1;
}

void onion_dev_peer_accept(struct onion_worker_head_t *head) {
   int ret;
   int index = 0;

   onion_worker_stack_t *stack = &head->stack;
   onion_worker_pair_t *pair = &head->worker_pair;

   struct onion_worker_t *worker = onion_block_get(pair->workers, index); 
   if (!worker) {
      return;
   }

   onion_work_info_t *info = onion_block_get(pair->workers_info, index); 
   if (!info) {
      return;
   }

   ONION_WORK_UNPACK(worker);
   if (!net || !epoll) {
      return;
   }

   struct onion_net_sock temp_sock;
   ret = onion_net_sock_accept(net->sock, &temp_sock);
   if (ret < 0) {
      //DEBUG_ERR("Peer finding failed.\n");
      return;
   }

   ret = onion_worker_create(worker, &temp_sock);
   if (ret < 0) {
      DEBUG_ERR("No!\n");
      return;
   }

   DEBUG_FUNC("New client!!!!\n");
}

void *onion_device_head_flow(void *arg) {
   struct onion_worker_head_t *head = (struct onion_worker_head_t*)arg;

   if (!head) {
      DEBUG_ERR("Head argument is null!\n");
      return NULL;
   }

   while (!atomic_load(&head->stack.should_stop)) {
      usleep(10900);
      int ret;

      onion_dev_peer_accept(head);
   }
   return NULL;
}

void *onion_dev_handler(struct onion_thread_my_args *args) {
   return NULL;
}

struct onion_block *onion_work_info_init(int count) {
   if (count < 1) {
      return NULL;
   }

   size_t size = sizeof(onion_work_info_t);
   size_t capable = size * count;
   struct onion_block *work_info = onion_block_init(capable, size);
   if (!work_info) {
      DEBUG_ERR("Onion worker info initialization failed.\n");
      return NULL;
   }

   for (int index = 0; index < count; index++) {
      onion_work_info_t *info = onion_block_get(work_info, index); 
      info->conn = 0;
      info->index = index;
      info->real_index = index;
   }

   return work_info;
}

void onion_work_info_exit(struct onion_block *work_info) {
   onion_block_exit(work_info);
}

void onion_work_info_add(onion_work_info_t *work_info) {

}

void onion_work_info_del(onion_work_info_t *work_info) {

}

int onion_hermit_pair_init(onion_hermit_pair_t *pair) {
   int ret;

   return 0;
}

int onion_static_worker_stack_init(struct onion_worker_head_t *head, onion_thread_func_t func, void *ptr) {
   onion_worker_stack_t *stack = &head->stack;
   ONION_UNPACK_TRIAD_FULL(head->triad_conf);

   atomic_store(&stack->should_stop, false);

   int ret = onion_epoll1_init(stack->epoll_data, EPOLL_CLOEXEC, EPOLLIN, 10);
   if (ret < 0) {
      DEBUG_ERR("Failed to initialize epoll data.\n");
      return -1;
   }

   stack->epoll_static = onion_epoll_static_init(epoll_conf, core_conf->count);
   if (!stack->epoll_static) {
      DEBUG_ERR("Failed to initialize onion_epoll_static_t.\n");
      return -1;
   }

   stack->net_static = onion_net_static_init(net_conf, core_conf->count);
   if (!stack->net_static) {
      DEBUG_ERR("Failed to initialize onion_net_static_t.\n");
      onion_epoll_static_exit(stack->epoll_static);
      stack->epoll_static = NULL;
      return -1;
   }

   ret = pthread_create(&stack->flow, NULL, func, ptr);
   if (ret != 0) {
      DEBUG_ERR("pthread_create failed with code %d.\n", ret);
      onion_net_static_exit(stack->net_static);
      stack->net_static = NULL;
      onion_epoll_static_exit(stack->epoll_static);
      stack->epoll_static = NULL;
      return -1;
   }

   struct epoll_event event;
   event.events = EPOLLIN;
   event.data.fd = stack->

   return 0;
}

void onion_static_worker_stack_exit(onion_worker_stack_t *stack) {
   atomic_store(&stack->should_stop, true);

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

   if (stack->epoll_data) {
      onion_epoll1_exit(stack->epoll_data);
      stack->epoll_data = NULL;
   }
}

int onion_worker_pair_init(struct onion_worker_head_t *head, size_t total, size_t size) {
   int count = total / size;
   onion_worker_pair_t *pair = &head->worker_pair;

   pair->workers = onion_worker_init(head, total, size);
   if (!pair->workers) {
      DEBUG_ERR("Workers initialization failed.\n");
      return -1;
   }

   pair->workers_info = onion_work_info_init(count);
   if (!pair->workers_info) {
      DEBUG_ERR("Workers info initialization failed.\n");
      onion_worker_pair_exit(pair, head->count);
      return -1;
   }

   return 0;
}

void onion_worker_pair_exit(onion_worker_pair_t *pair, int count) {
   if (pair->workers_info) {
      onion_work_info_exit(pair->workers_info);
      pair->workers_info = NULL;
   }
   if (pair->workers) {
      onion_worker_exit(pair->workers, count);
      pair->workers = NULL;
   }
}

int onion_dev_worker_init(struct onion_worker_head_t *head, int max_peers) {
   onion_worker_stack_t *stack = &head->stack;
   onion_worker_pair_t *pair = &head->worker_pair;
   ONION_UNPACK_TRIAD_FULL(head->triad_conf);

   struct onion_worker_t *worker = onion_block_alloc(pair->workers, NULL);
   if (!worker) {
      DEBUG_ERR("Worker allocation failed.\n");
      return -1;
   }

   worker->epoll = onion_slave_epoll1_init(stack->epoll_static, onion_dev_handler, core_conf->sched, max_peers);
   if (!worker->epoll) {
      DEBUG_ERR("Epoll initialization failed.\n");
      onion_block_free(pair->workers, worker);
      return -1;
   }

   worker->server_sock = onion_server_net_init(stack->net_static);
   if (!worker->server_sock) {
      DEBUG_ERR("Server socket initialization failed.\n");
      onion_slave_epoll1_exit(onion_epoll_priv(worker->epoll), worker->epoll);
      onion_block_free(pair->workers, worker);
      return -1;
   }

   head->count = head->count + 1;

   DEBUG_FUNC("Worker allocated! Peer max: %d\n", max_peers);
   return 0;
}

void onion_dev_worker_exit(struct onion_block *workers, struct onion_worker_t *worker) {
   if (!worker) return;

   if (worker->epoll && worker->epoll->initialized) {
      onion_slave_epoll1_exit(onion_epoll_priv(worker->epoll), worker->epoll);
   }

   if (worker->server_sock && worker->server_sock->initialized) {
      onion_server_net_exit(onion_net_priv(worker->server_sock), worker->server_sock);
   }

   onion_block_free(workers, worker);
}

struct onion_block *onion_worker_init(struct onion_worker_head_t *head, size_t total, size_t size) {
   onion_worker_pair_t *pair = &head->worker_pair;

   pair->workers = onion_block_init(total, size);
   if (!pair->workers) {
      DEBUG_ERR("Workers pool initialization failed.\n");
      return NULL;
   }

   ONION_UNPACK_TRIAD_FULL(head->triad_conf);
   onion_worker_stack_t *stack = &head->stack;

   long max_peers = net_conf->max_peers;
   int peers_per_core = max_peers / head->capable;
   int peers_remain = max_peers % head->capable;

   for (int index = 0; index < head->capable; index++) {
      int peers_for_core = peers_per_core + (index < peers_remain ? 1 : 0);
      if (onion_dev_worker_init(head, peers_for_core) < 0) {
         DEBUG_ERR("Failed to initialize worker at core %d\n", index);
         onion_worker_exit(pair->workers, head->capable);
         return NULL;
      }
   }

   return pair->workers;
}

void onion_worker_exit(struct onion_block *workers, int count) {
   if (!workers) return;

   for (int i = 0; i < count; i++) {
      struct onion_worker_t *worker = onion_block_get(workers, i);
      if (worker) {
         onion_dev_worker_exit(workers, worker);
      }
   }

   onion_block_exit(workers);
}

struct onion_worker_head_t *onion_device_init(onion_server_conf_triad_t *triad_conf) {
   int ret; 
   ONION_UNPACK_TRIAD(triad_conf);

   struct onion_worker_head_t *head = malloc(sizeof(*head));
   if (!head) {
      DEBUG_ERR("Failed to allocate onion_worker_head_t.\n");
      return NULL;
   }
   memset(head, 0, sizeof(*head));

   head->triad_conf = triad_conf;
   head->count = 0;
   head->capable = core_conf->count > 1 ? core_conf->count - 1 : core_conf->count;

   ret = onion_static_worker_stack_init(head, onion_device_head_flow, head); 
   if (ret < 0) {
      DEBUG_ERR("Failed to initialize static worker stack.\n");
      free(head);
      return NULL;
   }

   size_t size = sizeof(struct onion_worker_t);
   size_t total = size * core_conf->count;
   ret = onion_worker_pair_init(head, total, size);
   if (ret < 0) {
      DEBUG_ERR("Failed to initialize worker pair.\n");
      onion_static_worker_stack_exit(&head->stack);
      free(head);
      return NULL;
   }

   DEBUG_FUNC("Initialized onion_worker_head_t of size %zu bytes\n", sizeof(*head));
   return head;
}

void onion_device_exit(struct onion_worker_head_t *head) {
   if (!head) return;

   onion_worker_pair_exit(&head->worker_pair, head->capable);
   onion_static_worker_stack_exit(&head->stack);
   free(head);
}
