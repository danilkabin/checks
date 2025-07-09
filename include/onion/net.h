#ifndef ONION_NET_H
#define ONION_NET_H

#include "parser.h"
#include "socket.h"

#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>

#define ONION_IP_ADDRESS_SIZE 64

#define ONION_DEFAULT_IP_ADDRESS "127.0.0.1"
#define ONION_DEFAULT_PORT 8080
#define ONION_DEFAULT_MAX_PEERS 16
#define ONION_DEFAULT_MAX_QUEUE 16

typedef struct {
    char ip_address[ONION_IP_ADDRESS_SIZE];
    int port;
    int max_peers;
    int max_queue;
} onion_net_conf_t;

typedef struct {
   struct onion_net_sock *sock;
   struct onion_server_net *server_sock;

   bool initialized;
} onion_peer_net;

typedef struct {
   onion_net_conf_t *conf;
   struct onion_block *nets;
   long capable;
   long count;
} onion_net_static_t;

typedef struct {
   onion_net_static_t *parent;
   struct onion_net_sock *sock;

   struct onion_block *peer_barracks;
   int peer_current;
   int peer_capable;

   bool initialized;
} onion_server_net;

int onion_net_conf_init(onion_net_conf_t *net_conf);

onion_net_static_t *onion_net_priv(onion_server_net *net);
onion_server_net *onion_get_weak_net(onion_net_static_t *net_static);

onion_peer_net *onion_peer_net_init(onion_server_net *net_server, struct onion_net_sock *sock);

void onion_peer_net_exit(onion_server_net *net_server, onion_peer_net *ptr);

onion_server_net *onion_server_net_init(onion_net_static_t *net_static);
void onion_server_net_exit(onion_net_static_t *net_static, onion_server_net *net_server);

onion_net_static_t *onion_net_static_init(onion_net_conf_t *net_conf, long capable);
void onion_net_static_exit(onion_net_static_t *net_static);

#endif
