#ifndef ONION_SOCKET_H
#define ONION_SOCKET_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>

struct onion_tcp_port_conf {
   int domain;
   int type;
   int protocol;
   uint16_t port;
   struct in_addr addr;
};

struct onion_net_sock {
   int fd;
   int type;
   int queue_capable;
   struct sockaddr_in sock_addr;

   uint32_t packets_sent;
   uint32_t packets_received;
};

int onion_tcp_port_conf_check(struct onion_tcp_port_conf *conf);

int onion_net_port_check(uint16_t port);
int onion_net_sock_accept(struct onion_net_sock *onion_server_sock, struct onion_net_sock *client_sock);

struct onion_net_sock *onion_net_sock_init(struct onion_tcp_port_conf *port_conf, size_t queue_capable);
void onion_net_sock_exit(struct onion_net_sock *sock_struct);

#endif
