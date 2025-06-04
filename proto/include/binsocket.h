#ifndef BINSOCKET_H
#define BINSOCKET_H

#include "lock.h"
#include "listhead.h"
#include "pool.h"

#include <netinet/in.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define sockType int
#define idType uint32_t

#define BS_CLIENT_BUFF_SIZE 1024
#define BS_EPOLL_MAX_EVENTS 20

typedef enum {
   SOCKET_STATUS_OPEN,
   SOCKET_STATUS_CLOSE
} sock_syst_status;

typedef struct {
   uint32_t magic_number;
   uint8_t type;
   uint32_t message_len;
   idType sender_id;
   idType recipient_id;
   idType chat_id;
} BINSOCKET_HEADER;

typedef struct {
   BINSOCKET_HEADER *header;
   char *buff;
} BINSOCKET_MESSAGE;

typedef enum {
   BS_MESSAGETYPE_MESSAGE = 0x01,
   BS_MESSAGETYPE_JOIN    = 0x02,
   BS_MESSAGETYPE_LEAVE   = 0x03
} BINSOCKET_MESSAGE_TYPE;

typedef enum {
   BS_PROTODEBUG_OK = 0,
   BS_PROTODEBUG_MESSAGELEN = -1,
   BS_PROTODEBUG_USERID = -2,
   BS_PROTODEBUG_CHATID = -3,
   BS_PROTODEBUG_TYPE = -4
} BINSOCKET_DEBUG;

typedef enum {
   SOCKET_OPEN = 0,
   SOCKET_SHUTDOWN = -1,
   SOCKET_CLOSE = -2,
} BINSOCKET_TYPE;

typedef enum {
   WAITING_FOR_BODY,
   WAITING_FOR_HANDLER
} MESSAGE_STATE;

struct tcp_port_conf {
   int32_t domain;
   int32_t type;
   int32_t protocol;

   uint16_t port;
   struct in_addr addr;
};

typedef struct {
   sockType fd;
   int8_t state;

   uint32_t packets_sent;
   uint32_t packets_received;
} sock_t;

struct server_sock {
   sock_t sock;
   uint32_t peer_connections;
   uint32_t peer_capable;
   uint16_t peer_queue_capable;

   uint8_t *clientIDs;
   size_t clientIDs_size;

   struct tcp_port_conf port_conf;
   struct sockaddr_in sock_addr;
   socklen_t addrlen;

   int epoll_fd;
   struct epoll_event event, events[BS_EPOLL_MAX_EVENTS]; 

   struct list_head clients;
   thread_m client_core;
   atomic_bool released;
};

struct client_sock {
   sock_t sock;
   char buff[BS_CLIENT_BUFF_SIZE];
   size_t buff_len;

   struct epoll_event communicate_epoll;

   struct list_head list;
   atomic_bool released;
};

typedef void(*accept_callback_sk)(sockType client_fd, struct client_sock *);

int bs_header_assemble(BINSOCKET_MESSAGE *);
int bs_header_disassemble(BINSOCKET_MESSAGE *, char *);

sock_syst_status get_socket_system_status();
uint8_t *bs_get_clients_bitmap(struct server_sock *bs);
struct client_sock *get_bs_client_by_index(int );

size_t bs_recv_message(struct client_sock *, size_t, int);

struct client_sock *client_sock_create(struct server_sock *, sockType );
void client_sock_release(struct server_sock *, struct client_sock *);
int binSocket_accept(struct server_sock *sock, accept_callback_sk );

struct server_sock *binSocket_create(struct tcp_port_conf *);
int server_sock_init(struct server_sock *);
void server_sock_release(struct server_sock *);

int sock_syst_init(void);
void sock_syst_exit(struct server_sock *);

#define MESSAGE_HEADER_SIZE sizeof(BINSOCKET_HEADER)
#define MAX_CLIENTS_CAPABLE 17
#define CHECK_SOCKET_SYSTEM_STATUS(ret) do { if (SOCKET_SYSTEM_STATUS != SOCKET_STATUS_OPEN) return (ret); } while(0)

#endif 
