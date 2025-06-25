#ifndef ONION_NET_H
#define ONION_NET_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>

struct onion_tcp_port_conf {
   int32_t domain;
   int32_t type;
   int32_t protocol;
   uint16_t port;
   struct in_addr addr;
};

struct onion_net_sock {
   int fd;
   int type;
   struct sockaddr_in sock_addr;

   uint32_t packets_sent;
   uint32_t packets_received;
};

int onion_net_sock_accept(struct onion_net_sock *onion_server_sock, struct onion_net_sock *client_sock);

int onion_net_sock_init(struct onion_net_sock *sock_struct, struct onion_tcp_port_conf *port_conf, size_t queue_capable);
void onion_net_sock_exit(struct onion_net_sock *sock_struct);

#endif
