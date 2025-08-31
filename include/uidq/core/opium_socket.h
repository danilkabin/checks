#ifndef OPIUM_SOCKET_INCLUDE_H
#define OPIUM_SOCKET_INCLUDE_H

#include <stddef.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

typedef int opium_socket_t;

typedef struct {
   int domain;
   int type;
   int protocol;
   int flags;
   int fcntl_flags;
   uint16_t port;

   size_t queue_capable;

   union {
      struct sockaddr_in sock_addr;
      struct sockaddr_in6 sock_addr6;
      struct sockaddr_un sock_un;
   };

} opium_sock_conf_t;

int opium_sock_conf_isvalid(opium_sock_conf_t *conf);

int opium_sock_init(opium_sock_conf_t *conf);
void opium_sock_exit(opium_socket_t fd);

int opium_sock_port_check(int domain, int type, int protocol, uint16_t port);

int opium_sock_set_reuseaddr(opium_socket_t fd, int on);
int opium_sock_set_reuseport(opium_socket_t fd, int on);
int opium_sock_set_sndbuf(opium_socket_t fd, int size);
int opium_sock_set_rcvbuf(opium_socket_t fd, int size);
int opium_sock_set_keepalive(opium_socket_t fd, int on, int idle, int intvl, int cnt);
int opium_sock_set_nodelay(opium_socket_t fd, int on);
int opium_sock_set_fastopen(opium_socket_t fd, int on);
int opium_sock_set_nonblocking(opium_socket_t fd, int on);
int opium_sock_set_cloexec(opium_socket_t fd, int on);

#endif /* OPIUM_SOCKET_INCLUDE_H */
