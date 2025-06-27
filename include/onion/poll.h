#ifndef ONION_POLL_H
#define ONION_POLL_H

#include "onion.h"
#include "listhead.h"
#include "lock.h"
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

#define ONION_AUTO_onion_worker_COUNT true

#define ONION_MESSAGE_HEADER_SIZE sizeof(onionET_HEADER)
#define ONION_CHECK_SOCKET_SYSTEM_STATUS(ret) do { if (ONION_SOCKET_SYSTEM_STATUS != ONION_SOCKET_STATUS_OPEN) return (ret); } while(0)

extern struct onion_worker **onion_sock_onion_workers;

typedef enum {
   ONION_SOCKET_STATUS_OPEN,
   ONION_SOCKET_STATUS_CLOSE
} onion_sock_syst_status;

typedef enum {
   ONION_PEER_PROTO_TYPE_HTTP,
   ONION_PEER_PROTO_TYPE_WEBSOCK
} onion_peer_proto_type;

typedef enum {
   ONION_BS_PROTODEBUG_OK = 0,
   ONION_BS_PROTODEBUG_MESSAGELEN = -1,
   ONION_BS_PROTODEBUG_USERID = -2,
   ONION_BS_PROTODEBUG_CHATID = -3,
   ONION_BS_PROTODEBUG_TYPE = -4
} onionET_DEBUG;

typedef enum {
   ONION_WAITING_FOR_BODY,
   ONION_WAITING_FOR_HANDLER
} onion_MESSAGE_STATE;

struct onion_worker {
   int epoll_fd;
   struct epoll_event event, events[ONION_BS_EPOLL_MAX_EVENTS];
   thread_m core;
   struct onion_server_sock *server_sock;
   int core_id;
   atomic_int peer_count;
};

struct onion_peer_sock {
   struct onion_net_sock sock;
   onion_http_parser_t parser;
   onion_peer_proto_type proto_type;
   struct list_head list;
   struct onion_worker *onion_worker;
   bool initialized;
   bool released;
};

struct onion_server_sock {
   struct onion_net_sock sock;
   uint16_t peer_queue_capable;
   uint8_t *peerIDs;
   size_t peerIDs_size;

   struct onion_block *peer_pool;
   struct list_head peers;

   struct onion_worker *onion_worker;
   int onion_worker_index;

   int peer_current;
   int peer_capable;

   struct onion_slab *request_allocator;
   struct onion_slab *request_msg_allocator;

   bool initialized;
   bool released;
};

typedef void (*onion_accept_callback_sk)(int peer_fd, struct onion_peer_sock *);

onion_sock_syst_status onion_get_socket_system_status(void);
uint8_t *onion_bs_get_peers_bitmap(struct onion_server_sock *);
struct onion_peer_sock *onion_get_bs_peer_by_index(int);
struct onion_peer_sock *onion_peer_sock_create(struct onion_server_sock *, int);
void onion_peer_sock_release(struct onion_server_sock *, struct onion_peer_sock *);

struct onion_server_sock *onion_server_sock_create(struct onion_tcp_port_conf *, struct onion_worker *, int, int);
void onion_server_sock_release(struct onion_server_sock *);
int onion_server_sock_accept(struct onion_server_sock *);

void *onion_bs_onion_worker_thread(void *);
int onion_sock_core_init();
void onion_sock_core_exit();

int onion_sock_syst_init(void);
void onion_sock_syst_exit(struct onion_server_sock *);

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
