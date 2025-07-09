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

typedef void *(*onion_thread_func_t)(void*);

typedef struct {
   onion_epoll_static_t *epoll_static;
   onion_net_static_t *net_static;
   pthread_t flow;
} onion_worker_stack;

struct onion_worker_head {
   onion_server_conf_triad_t *triad_conf;
   struct onion_block *workers;
   onion_worker_stack stack;
   long count;
   long capable;
};

struct onion_worker {
   onion_server_net *server_sock;
   onion_epoll_t *epoll;
};

typedef void (*onion_accept_callback_sk)(int peer_fd, onion_peer_net *);

int onion_conf_triad_init(onion_server_conf_triad_t *triad);
void onion_conf_triad_exit(onion_server_conf_triad_t *triad);
int onion_core_conf_init(onion_core_conf_t *core_conf);

int onion_static_worker_stack_init(struct onion_worker_head *head, onion_thread_func_t func, void *ptr);
void onion_static_worker_stack_exit(onion_worker_stack *stack);

int onion_accept_net(onion_server_net *net_server, onion_epoll_t *epoll);

int onion_dev_worker_init(struct onion_worker_head *head, int max_peers);
void onion_dev_worker_exit(struct onion_worker_head *onion_workers, struct onion_worker *worker);

struct onion_worker_head *onion_device_init(onion_server_conf_triad_t *triad_conf);
void onion_device_exit(struct onion_worker_head *head);

#endif
