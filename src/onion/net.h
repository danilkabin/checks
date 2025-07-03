#ifndef ONION_NET_H
#define ONION_NET_H

#include "parser.h"
#include "slab.h"
#include "socket.h"
#include "listhead.h"

#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>

typedef struct {
   int peers_capable;
   uint16_t queue_capable;
   struct onion_tcp_port_conf port_conf;
} onion_server_net_conf;

typedef struct {
   struct onion_net_sock *sock;
   struct onion_server_net *server_sock;

   bool initialized;
} onion_peer_net;

typedef struct {
   struct onion_net_sock *sock;

   struct onion_block *peer_barracks;
   int peer_current;
   int peer_capable;

   struct onion_slab *request_allocator;
   struct onion_slab *request_msg_allocator;

   bool initialized;
} onion_server_net;

typedef struct {
   struct onion_block *nets;
   long capable;
   long count;
} onion_net_static_t;

onion_net_static_t *onion_get_static_by_net(onion_server_net *net);

int onion_accept_net(onion_server_net *net_server);

onion_peer_net *onion_peer_net_init(onion_server_net *net_server, struct onion_net_sock *sock);
void onion_peer_net_exit(onion_server_net *net_server, onion_peer_net *ptr);

onion_server_net *onion_server_net_init(onion_net_static_t *net_static, onion_server_net_conf *conf);
void onion_server_net_exit(onion_net_static_t *net_static, onion_server_net *net_server);

int onion_net_static_init(onion_net_static_t **ptr, long capable);
void onion_net_static_exit(onion_net_static_t *net_static);

#endif
