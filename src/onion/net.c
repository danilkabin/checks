#include "net.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

void onion_net_sock_zero(struct onion_net_sock *sock_struct) {
   sock_struct->fd = -1;
   sock_struct->type = -1;
   sock_struct->packets_sent = 0;
   sock_struct->packets_received = 0;
}

int onion_net_sock_tcp_create(struct onion_net_sock *sock_struct, struct onion_tcp_port_conf *port_conf, size_t queue_capable) {
   struct sockaddr_in sock_addr;
   socklen_t addr_len = sizeof(sock_addr);
   memset(&sock_addr, 0, addr_len);

   sock_addr.sin_family = port_conf->domain;
   sock_addr.sin_addr.s_addr = port_conf->addr.s_addr;
   sock_addr.sin_port = port_conf->port;

   int sock_fd = socket(port_conf->domain, port_conf->type, port_conf->protocol);
   if (sock_fd < 0) {
      fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
      goto unsuccessfull;
   }

   int opt = 1;

   if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      fprintf(stderr, "Failed to set socket options[SO_REUSEADDR]: %s\n", strerror(errno));
   }

   if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
      fprintf(stderr, "Failed to set socket options[SO_REUSEPORT]: %s\n", strerror(errno));
      goto close_sock;
   }

   if (bind(sock_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) {
      fprintf(stderr, "Failed to bind socket: %s\n", strerror(errno));
      goto close_sock;
   }

   if (listen(sock_fd, queue_capable) < 0) {
      fprintf(stderr, "Failed to listen socket: %s\n", strerror(errno));
      goto close_sock;
   }

   onion_net_sock_zero(sock_struct);
   sock_struct->fd = sock_fd;
   sock_struct->type = port_conf->type;
   sock_struct->sock_addr = sock_addr;

   return 0;
   close_sock:
   close(sock_fd);
   unsuccessfull:
   return -1;
}

int onion_net_sock_tcp_accept(struct onion_net_sock *onion_server_sock, struct onion_net_sock *client_sock) {
   struct sockaddr_in client_addr;
   socklen_t client_len = sizeof(client_addr);

   int client_fd = accept(onion_server_sock->fd, (struct sockaddr *)&client_addr, &client_len);
   if (client_fd < 0) {
      fprintf(stderr, "Failed to accept connection: %s\n", strerror(errno));
      return -1;
   }

   int fcntl_result = fcntl(client_fd, F_GETFL, 0);
   if (fcntl_result == -1) {
      fprintf(stderr, "Failed to get peer socket flags%s\n", strerror(errno));
      return -1;
   }

   fcntl_result = fcntl(client_fd, F_SETFL, fcntl_result | O_NONBLOCK);
   if (fcntl_result == -1) {
      fprintf(stderr, "Failed to set peer socket flags%s\n", strerror(errno));
      return -1;
   }

   onion_net_sock_zero(client_sock);
   client_sock->fd = client_fd;
   client_sock->type = SOCK_STREAM;
   client_sock->sock_addr = client_addr;

   return 0;
}

void onion_net_sock_tcp_close(struct onion_net_sock *sock_struct) {
   if (sock_struct->fd >= 0) {
      close(sock_struct->fd);
      onion_net_sock_zero(sock_struct);
   }
}

int onion_net_sock_accept(struct onion_net_sock *onion_server_sock, struct onion_net_sock *client_sock) {
   if (onion_server_sock->type == SOCK_STREAM) {
      return onion_net_sock_tcp_accept(onion_server_sock, client_sock);
   }
   return -1;
}

int onion_net_sock_init(struct onion_net_sock *sock_struct, struct onion_tcp_port_conf *port_conf, size_t queue_capable) {
   if (port_conf->type == SOCK_STREAM) {
      return onion_net_sock_tcp_create(sock_struct, port_conf, queue_capable);
   }
   return -1;
}

void onion_net_sock_exit(struct onion_net_sock *sock_struct) {
   if (sock_struct->type == SOCK_STREAM) {
      onion_net_sock_tcp_close(sock_struct);
   }
}
