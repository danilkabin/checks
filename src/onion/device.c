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

void *onion_dev_handler(struct onion_thread_my_args *args) {
   return NULL;
}

void *onion_hermit_stack_flow(void *arg) {
    struct onion_worker_head_t *head = (struct onion_worker_head_t*)arg;
    if (!head) {
        DEBUG_ERR("Received null pointer for worker head structure.\n");
        goto unsuccessfull;
    }

    ONION_UNPACK_TRIAD_FULL(head->triad_conf);
    onion_worker_stack_t *worker_stack = head->worker_stack;
    onion_hermit_stack_t *hermit_stack = head->hermit_stack;

    if (!worker_stack || !hermit_stack) {
        DEBUG_ERR("Worker stack or hermit stack is not properly initialized.\n");
        goto unsuccessfull;
    }

    while (!hermit_stack->should_stop) {
       DEBUG_FUNC("Hello!\n");
       usleep(100000);
    }

unsuccessfull:
    return NULL;
}

int onion_hermit_stack_init(struct onion_worker_head_t *head) {
   int ret;
   ONION_UNPACK_TRIAD_FULL(head->triad_conf);

   onion_hermit_stack_t *hermit_stack = calloc(1, sizeof(onion_hermit_stack_t));
   if (!hermit_stack) {
      DEBUG_ERR("Failed to allocate memory for hermit stack.\n"); 
      goto unsuccessfull;
   }
   head->hermit_stack = hermit_stack;

   onion_net_conf_t *accept_conf = net_conf;
   
   int domain = AF_INET;
   int type = SOCK_STREAM;

   const char *ip_address = accept_conf->ip_address;
   uint16_t port = htons(accept_conf->port);
   size_t queue_capable = accept_conf->max_queue;

   struct onion_tcp_port_conf port_conf = {
      .domain = domain,
      .type = type,
      .port = port,
   };

   if (inet_pton(domain, ip_address, &port_conf.addr) <= 0) {
      DEBUG_ERR("Ip address bind failed.\n");
      goto unsuccessfull;
   }

   hermit_stack->epoll_data = onion_epoll1_init(EPOLL_CLOEXEC, EPOLLIN, 10);
   if (!hermit_stack->epoll_data) {
      DEBUG_ERR("Failed to initialize epoll data.\n");
      goto free_stack; 
   }

   hermit_stack->sock_data = onion_net_sock_init(&port_conf, queue_capable);
   if (!hermit_stack->sock_data) {
      DEBUG_ERR("Failted to initialize sock data.\n");
      goto free_stack;
   }

   hermit_stack->should_stop = false;
   ret = pthread_create(&hermit_stack->flow, NULL, onion_hermit_stack_flow, head);
   if (ret != 0) {
      DEBUG_ERR("Could not launch worker thread (pthread_create returned %d)\n", ret);
      goto free_stack;
   }

   return 0;
free_stack:
   onion_hermit_stack_exit(hermit_stack);
unsuccessfull:
   return -1;
}

void onion_hermit_stack_exit(onion_hermit_stack_t *hermit_stack) {
   hermit_stack->should_stop = true;
   if (hermit_stack->flow) {
      pthread_join(hermit_stack->flow, NULL);
      hermit_stack->flow = 0;
   }

   if (hermit_stack->sock_data) {
      onion_net_sock_exit(hermit_stack->sock_data);
      hermit_stack->sock_data = NULL;
   } 

   if (hermit_stack->epoll_data) {
      onion_epoll1_exit(hermit_stack->epoll_data);
      hermit_stack->epoll_data = NULL;
   }
}

int onion_workers_info_init(struct onion_worker_head_t *head) {
   ONION_UNPACK_TRIAD_FULL(head->triad_conf);

   onion_worker_stack_t *worker_stack = head->worker_stack;
   if (!worker_stack) {
      DEBUG_ERR("Worker stack is missing or uninitialized. Please check initialization logic.\n");
      goto unsuccessfull;
   }

   int count = head->capable;
   if (count < 1) {
      DEBUG_ERR("Invalid number of capable workers. Expected at least one.\n");
      goto unsuccessfull;
   }

   size_t size = sizeof(onion_work_info_t);
   size_t capable = size * count;

   struct onion_block *work_info = onion_block_init(capable, size);
   if (!work_info) {
      DEBUG_ERR("Failed to allocate memory for worker info block.\n");
      goto unsuccessfull;
   }

   for (int index = 0; index < count; index++) {
      onion_work_info_t *info = onion_block_get(work_info, index);
      info->conn = 0;
      info->index = index;
      info->real_index = index;
   }

   return 1;
unsuccessfull:
   return -1;
}

void onion_workers_info_exit(struct onion_block *work_info) {
   onion_block_exit(work_info);
}

void onion_work_info_add(onion_work_info_t *work_info) {
   // Stub
}

void onion_work_info_del(onion_work_info_t *work_info) {
   // Stub
}

int onion_worker_slot_init(struct onion_worker_head_t *head, int max_peers) {
   ONION_UNPACK_TRIAD_FULL(head->triad_conf);
   
   onion_worker_stack_t *worker_stack = head->worker_stack;
   if (!worker_stack) {
      DEBUG_ERR("Worker stack not found in head structure.\n");
      goto unsuccessfull;
   }

   struct onion_block *workers = worker_stack->workers;
   if (!workers) {
      DEBUG_ERR("Workers block not allocated in worker stack.\n");
      goto unsuccessfull;
   }

   struct onion_worker_t *child = onion_block_alloc(workers, NULL);
   if (!child) {
      DEBUG_ERR("Failed to allocate memory for new worker.\n");
      goto unsuccessfull;
   }
   
   child->epoll = onion_slave_epoll1_init(worker_stack->epoll_static, onion_dev_handler, core_conf->sched, max_peers);
   if (!child->epoll) {
      DEBUG_ERR("Failed to initialize epoll for worker.\n");
      goto free_slot;
   }

   child->server_sock = onion_server_net_init(worker_stack->net_static);
   if (!child->server_sock) {
      DEBUG_ERR("Failed to initialize server socket for worker.\n");
      goto free_slot;
   }

   head->count += 1;

   DEBUG_FUNC("Worker allocated successfully. Peer max: %d\n", max_peers);
   return 0;
free_slot:
   onion_worker_slot_exit(workers, child);
unsuccessfull:
   return -1;
}

void onion_worker_slot_exit(struct onion_block *workers, struct onion_worker_t *worker) {
   if (!worker) return;

   if (worker->epoll && worker->epoll->initialized) {
      onion_slave_epoll1_exit(onion_epoll_priv(worker->epoll), worker->epoll);
   }

   if (worker->server_sock && worker->server_sock->initialized) {
      onion_server_net_exit(onion_net_priv(worker->server_sock), worker->server_sock);
   }

   onion_block_free(workers, worker);
}

int onion_workers_init(struct onion_worker_head_t *head) {
   ONION_UNPACK_TRIAD_FULL(head->triad_conf);

   onion_worker_stack_t *worker_stack = head->worker_stack;
   if (!worker_stack) {
      DEBUG_ERR("Worker stack not initialized in head.\n");
      goto unsuccessfull;
   }
   int capable = core_conf->count;
   int max_peers = net_conf->max_peers;

   int peers_per_core = max_peers / capable;
   int peers_remain = max_peers % capable;
DEBUG_FUNC("capable %d, max peers: %d, peer_per_core: %d, peers_remain: %d\n", capable, max_peers, peers_per_core, peers_remain);
   size_t size = sizeof(struct onion_worker_t);
   size_t total = size * head->capable;

   struct onion_block *workers = onion_block_init(total, size);
   if (!workers) {
      DEBUG_ERR("Failed to initialize worker block pool.\n");
      goto unsuccessfull;
   }
   worker_stack->workers = workers;

   for (int index = 0; index < capable; index++) {
      int peers_for_core = peers_per_core + (index < peers_remain ? 1 : 0);
      int ret = onion_worker_slot_init(head, peers_for_core);
      if (ret < 0) {
         DEBUG_ERR("Failed to initialize worker slot for core %d.\n", index);
         onion_workers_exit(workers);
         goto unsuccessfull; 
      }
   }

   return 1;
unsuccessfull:
   return -1;
}

void onion_workers_exit(struct onion_block *workers) {
   if (!workers) return;

   for (size_t index = 0; index < workers->block_max; index++) {
      struct onion_worker_t *worker = onion_block_get(workers, index);
      if (worker) {
         onion_worker_slot_exit(workers, worker);
      }
   }

   onion_block_exit(workers);
}

int onion_worker_stack_init(struct onion_worker_head_t *head) {
   int ret;
   ONION_UNPACK_TRIAD_FULL(head->triad_conf);
   
   onion_worker_stack_t *worker_stack = calloc(1, sizeof(onion_worker_stack_t));
   if (!worker_stack) {
      DEBUG_ERR("Failed to allocate memory for worker stack.\n"); 
      goto unsuccessfull;
   }
   head->worker_stack = worker_stack;

   worker_stack->epoll_static = onion_epoll_static_init(epoll_conf, head->capable);
   if (!worker_stack->epoll_static) {
      DEBUG_ERR("Failed to initialize static epoll block.\n");
      goto free_stack;
   }

   worker_stack->net_static = onion_net_static_init(net_conf, head->capable);
   if (!worker_stack->net_static) {
      DEBUG_ERR("Failed to initialize static network block.\n");
      goto free_stack;
   }

   ret = onion_workers_init(head);
   if (ret < 0) {
      DEBUG_ERR("Workers initialization returned an error.\n");
      goto free_stack;
   }

   ret = onion_workers_info_init(head);
   if (ret < 0) {
      DEBUG_ERR("Failed to initialize workers info block.\n");
      goto free_stack;
   }

   return 1;
free_stack:
   onion_worker_stack_exit(head->worker_stack);
unsuccessfull:
   return -1;
}

void onion_worker_stack_exit(onion_worker_stack_t *worker_stack) {
   if (worker_stack->workers_info) {
      onion_workers_info_exit(worker_stack->workers_info);
      worker_stack->workers_info = NULL;
   }

   if (worker_stack->workers) {
      onion_workers_exit(worker_stack->workers);
      worker_stack->workers = NULL;
   }

   if (worker_stack->net_static) {
      onion_net_static_exit(worker_stack->net_static);
      worker_stack->net_static = NULL;
   }

   if (worker_stack->epoll_static) {
      onion_epoll_static_exit(worker_stack->epoll_static);
      worker_stack->epoll_static = NULL;
   }
}

struct onion_worker_head_t *onion_device_init(onion_server_conf_triad_t *triad_conf) {
   int ret; 
   ONION_UNPACK_TRIAD(triad_conf);
   
   struct onion_worker_head_t *head = calloc(1, sizeof(struct onion_worker_head_t));
   if (!head) {
      DEBUG_ERR("Failed to allocate memory for onion_worker_head structure.\n");
      return NULL;
   }

   long capable = core_conf->count > 1 ? core_conf->count - 1 : core_conf->count;

   head->initialized = false;
   head->triad_conf = triad_conf;
   head->worker_stack = NULL;
   head->hermit_stack = NULL;
   head->count = 0;
   head->capable = capable; 

   ret = onion_worker_stack_init(head);
   if (ret < 0) {
      DEBUG_ERR("Worker stack initialization failed.\n");
      goto free_head;
   }

   ret = onion_hermit_stack_init(head);
   if (ret < 0) {
      DEBUG_ERR("Hermit stack initialization failed.\n");
      goto free_head;
   } 

   head->initialized = true;
   DEBUG_FUNC("Successfully initialized onion_worker_head. Size: %zu bytes.\n", sizeof(*head));
   return head;
free_head:
   onion_device_exit(head);
unsuccessfull:
   return NULL;
}

void onion_device_exit(struct onion_worker_head_t *head) {
   if (!head) return;
   head->initialized = false;

   if (head->worker_stack) {
      onion_worker_stack_exit(head->worker_stack);
      head->worker_stack = NULL;
   }

   if (head->hermit_stack) {
      onion_hermit_stack_exit(head->hermit_stack);
      head->hermit_stack = NULL;
   }

   free(head);
}
