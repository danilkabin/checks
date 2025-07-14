#ifndef ONION_DEVICE_H
#define ONION_DEVICE_H

#include "epoll.h"
#include "pool.h"
#include "socket.h"
#include "net.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define ONION_DEFAULT_CORE_SCHED 0

typedef struct {
   int count;
   int sched;
   int worker_count;
} onion_core_conf_t;

typedef struct {
   onion_core_conf_t *core;
   onion_epoll_conf_t *epoll;
   onion_net_conf_t *net;
} onion_server_conf_triad_t;

#define ONION_UNPACK_TRIAD(triad) \
   onion_core_conf_t *core_conf = (triad)->core; \
   onion_epoll_conf_t *epoll_conf = (triad)->epoll; \
   onion_net_conf_t *net_conf = (triad)->net;

#define ONION_UNPACK_TRIAD_FULL(triad) \
   onion_server_conf_triad_t *triad_conf = (triad); \
   ONION_UNPACK_TRIAD(triad);

#define ONION_WORK_UNPACK(worker) \
   onion_server_net *net = worker->server_sock; \
   onion_epoll_t *epoll = worker->epoll; 

typedef void *(*onion_thread_func_t)(void*);

typedef struct {
   int conn;
   int index;
   int real_index;
} onion_work_info_t;

typedef struct {
   onion_epoll_data_t *epoll_data;
   onion_net_sock *sock_data;
   pthread_t flow;
   bool should_stop;
} onion_hermit_stack_t;

typedef struct {
   onion_epoll_static_t *epoll_static;
   onion_net_static_t *net_static;
   struct onion_block *workers;
   struct onion_block *workers_info;
} onion_worker_stack_t;

struct onion_worker_head_t {
   onion_server_conf_triad_t *triad_conf;
   onion_worker_stack_t *worker_stack;
   onion_hermit_stack_t *hermit_stack;

   long count;
   long capable;

   bool initialized;
};

struct onion_worker_t {
   onion_server_net_t *server_sock;
   onion_epoll_t *epoll;
   bool initialized;
};

typedef void (*onion_accept_callback_sk)(int peer_fd, onion_server_slot_net_t *);

int onion_conf_triad_init(onion_server_conf_triad_t *triad);
void onion_conf_triad_exit(onion_server_conf_triad_t *triad);
int onion_core_conf_init(onion_core_conf_t *core_conf);

int onion_hermit_stack_init(struct onion_worker_head_t *head);
void onion_hermit_stack_exit(onion_hermit_stack_t *hermit_stack);

int onion_worker_slot_init(struct onion_worker_head_t *head, int max_peers);
void onion_worker_slot_exit(struct onion_block *workers, struct onion_worker_t *worker);

int onion_workers_init(struct onion_worker_head_t *head);
void onion_workers_exit(struct onion_block *workers);

int onion_worker_stack_init(struct onion_worker_head_t *head);
void onion_worker_stack_exit(onion_worker_stack_t *worker_stack);

struct onion_worker_head_t *onion_device_init(onion_server_conf_triad_t *triad_conf);
void onion_device_exit(struct onion_worker_head_t *head);

#endif
