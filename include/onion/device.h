#ifndef ONION_DEVICE_H
#define ONION_DEVICE_H

#include "onion.h"
#include "listhead.h"
#include "lock.h"
#include "onion/epoll.h"
#include "parser.h"
#include "pool.h"
#include "net.h"
#include "slab.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define ONION_BS_CLIENT_BUFF_COUNT_CAPACITY 16
#define ONION_BS_CLIENT_BUFF_CAPACITY 8192
#define ONION_BS_EPOLL_MAX_EVENTS 20
#define ONION_PEER_MAX_QUEUE_CAPABLE 16
#define ONION_MAX_CLIENTS_CAPABLE 170

struct onion_worker_pool {
   struct onion_block *workers;
   onion_epoll_static_t *epoll_static;
   long count;
   long capable;
   int peers_capable;
};

struct onion_worker {
   struct onion_server_sock *server_sock;
   onion_epoll_t *epoll;
};

extern struct onion_worker_pool *onion_workers;

struct onion_peer_sock {
   struct onion_net_sock sock;
   struct onion_server_sock *server_sock;

   onion_http_parser_t parser;
   struct list_head list;

   bool initialized;
   bool released;
};

struct onion_server_sock {
   struct onion_net_sock sock;
   struct onion_worker *onion_worker;

   struct onion_block *peer_pool;
   struct list_head peers;

   uint8_t *peerIDs;
   size_t peerIDs_size;

   int peer_current;
   int peer_capable;

   struct onion_slab *request_allocator;
   struct onion_slab *request_msg_allocator;

   bool initialized;
   bool released;
};

typedef void (*onion_accept_callback_sk)(int peer_fd, struct onion_peer_sock *);

uint8_t *onion_bs_get_peers_bitmap(struct onion_server_sock *);
struct onion_peer_sock *onion_get_bs_peer_by_index(int);
struct onion_peer_sock *onion_peer_sock_create(struct onion_server_sock *, int);
void onion_peer_sock_release(struct onion_server_sock *, struct onion_peer_sock *);

struct onion_server_sock *onion_server_sock_create(struct onion_tcp_port_conf *, struct onion_worker *, int, int);
void onion_server_sock_release(struct onion_server_sock *);
int onion_server_sock_accept(struct onion_server_sock *);

void *onion_bs_onion_worker_thread(void *);

int onion_device_init(uint16_t port, long core_count, int peers_capable);
void onion_device_exit();

#define ONION_CREATE_EPOLL(work, fd, event, flags) do { \
   fd = epoll_create1(flags); \
   if (fd < 0) { \
      DEBUG_ERR("no epoll!\n"); \
      work; \
   } \
} while(0)

#define ONION_DELETE_EPOLL(fd) do { \
   if (fd) { \
      close(fd); \
   } \
} while(0)

#endif
