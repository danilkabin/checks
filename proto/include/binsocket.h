#ifndef BIN_SOCKET_H
#define BIN_SOCKET_H

#include "lock.h"
#include "account.h"
#include "listhead.h"
#include "pool.h"

#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define sockType int
#define idType uint32_t

#define BS_ANONYMOUS_BUFF_SIZE 64

extern struct list_head bs_anonymous_list;
extern thread_m anonymous_core;

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

struct binSocket {
   sockType fd;
   int8_t state;

   uint64_t packets_sent;
   uint64_t packets_received;

   uint32_t peer_connections;
   uint32_t peer_capable;
   uint16_t peer_queue_capable;

   struct tcp_port_conf port_conf;
   struct sockaddr_in sock_addr;
   socklen_t addrlen;

   struct list_head clients;
};

struct binSocket_client {
   sockType sock;
   char buff[BS_ANONYMOUS_BUFF_SIZE];
   struct list_head list;
};

typedef void(*accept_callback_sk)(sockType client_fd, struct binSocket_client *anonymous);

int bs_header_assemble(BINSOCKET_MESSAGE *message);
int bs_header_disassemble(BINSOCKET_MESSAGE *message, char *buffer);

sock_syst_status get_socket_system_status();
uint8_t *bs_get_clients_bitmap();
struct binSocket_client *get_bs_client_by_index(int index);

//int bs_recv_message(struct binSocket *sock);

struct binSocket_client *binSocket_client_create(sockType fd);
void binSocket_client_release(struct binSocket_client *client);
int binSocket_accept(struct binSocket *sock, accept_callback_sk work);

struct binSocket *binSocket_create(struct tcp_port_conf *port_conf);
int binSocket_init(struct binSocket *sock);
void binSocket_release(struct binSocket *sock);

int sock_syst_init(void);
void sock_syst_exit(void);

#define MESSAGE_HEADER_SIZE sizeof(BINSOCKET_HEADER)
#define MAX_CLIENTS_CAPABLE 17
#define CHECK_SOCKET_SYSTEM_STATUS(ret) do { if (SOCKET_SYSTEM_STATUS != SOCKET_STATUS_OPEN) return (ret); } while(0)

#endif 
