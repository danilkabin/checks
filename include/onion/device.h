#ifndef ONION_DEVICE_H
#define ONION_DEVICE_H

#include "onion/epoll.h"
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

typedef void *(*onion_thread_func_t)(void*);

typedef struct {
   onion_epoll_static_t *epoll_static;
   onion_net_static_t *net_static;
   pthread_t flow;
   onion_server_net_conf conf;
} onion_worker_stack;

struct onion_worker_head {
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

int onion_static_worker_stack_init(onion_worker_stack *stack, onion_server_net_conf conf, onion_thread_func_t func, void *ptr);
void onion_static_worker_stack_exit(onion_worker_stack *stack);

int onion_accept_net(onion_server_net *net_server, onion_epoll_t *epoll);

int onion_dev_worker_init(struct onion_worker_head *onion_workers, struct onion_tcp_port_conf port_conf, int peers_capable, int queue_capable);
void onion_dev_worker_exit(struct onion_worker_head *onion_workers, struct onion_worker *worker);

struct onion_worker_head *onion_device_init(uint16_t port, long core_count, int peers_capable, int queue_capable);
void onion_device_exit(struct onion_worker_head *head);

#endif
