#ifndef BINSOCKET_H
#define BINSOCKET_H

#include "http.h"
#include "listhead.h"
#include "lock.h"
#include "pool.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include "slab.h"
#include "unistd.h"

#define sockType int
#define idType uint32_t

#define BS_CLIENT_BUFF_COUNT_CAPACITY 16 
#define BS_CLIENT_BUFF_CAPACITY 8192 
#define BS_EPOLL_MAX_EVENTS 20
#define MAX_CLIENTS_CAPABLE 170

#define AUTO_WORKER_COUNT true

#define MESSAGE_HEADER_SIZE sizeof(BINSOCKET_HEADER)
#define CHECK_SOCKET_SYSTEM_STATUS(ret) do { if (SOCKET_SYSTEM_STATUS != SOCKET_STATUS_OPEN) return (ret); } while(0)

extern struct worker **sock_workers;

typedef enum {
   SOCKET_STATUS_OPEN,
   SOCKET_STATUS_CLOSE
} sock_syst_status;

typedef enum {
   PEER_PROTO_TYPE_HTTP,
   PEER_PROTO_TYPE_WEBSOCK
} peer_proto_type;

typedef enum {
   BS_PROTODEBUG_OK = 0,
   BS_PROTODEBUG_MESSAGELEN = -1,
   BS_PROTODEBUG_USERID = -2,
   BS_PROTODEBUG_CHATID = -3,
   BS_PROTODEBUG_TYPE = -4
} BINSOCKET_DEBUG;

typedef enum {
   WAITING_FOR_BODY,
   WAITING_FOR_HANDLER
} MESSAGE_STATE;

typedef struct {
   sockType fd;
   int8_t state;
   uint32_t packets_sent;
   uint32_t packets_received;
} sock_t;

struct worker {
   int epoll_fd;
   struct epoll_event event, events[BS_EPOLL_MAX_EVENTS];
   thread_m core;
   struct server_sock *bs;
   int core_id;
   atomic_int peer_count;
};

struct tcp_port_conf {
   int32_t domain;
   int32_t type;
   int32_t protocol;
   uint16_t port;
   struct in_addr addr;
};

struct peer_sock {
   sock_t sock;
   http_parser parser;
   peer_proto_type proto_type;
   struct list_head list;
   struct worker *worker;
   bool initialized;
   bool released;
};

struct server_sock {
   sock_t sock;
   uint16_t peer_queue_capable;
   uint8_t *peerIDs;
   size_t peerIDs_size;

   struct memoryPool *peer_pool;
   struct list_head peers;

   struct worker *worker;
   int worker_index;

   int peer_current;
   int peer_capable;

   bool initialized;
   bool released;
};

typedef void (*accept_callback_sk)(sockType peer_fd, struct peer_sock *);

sock_syst_status get_socket_system_status(void);
uint8_t *bs_get_peers_bitmap(struct server_sock *);
struct peer_sock *get_bs_peer_by_index(int);
struct peer_sock *peer_sock_create(struct server_sock *, sockType);
void peer_sock_release(struct server_sock *, struct peer_sock *);

struct server_sock *server_sock_create(struct tcp_port_conf *, struct worker *, int, int);
void server_sock_release(struct server_sock *);
int server_sock_accept(struct server_sock *);

void *bs_worker_thread(void *);
int sock_core_init();
void sock_core_exit();

int sock_syst_init(void);
void sock_syst_exit(struct server_sock *);

#define _CREATE_EPOLL(work, fd, event, flags) do { \
   fd = epoll_create1(flags); \
   if (fd < 0) { \
      DEBUG_ERR("no epoll!\n"); \
      work; \
   } \
} while(0)

#define _DELETE_EPOLL(fd) do { \
   if (fd) { \
      close(fd); \
   } \
} while(0)

#endif
