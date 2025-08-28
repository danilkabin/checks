#ifndef UIDQ_SOCKET_INCLUDE_H
#define UIDQ_SOCKET_INCLUDE_H

#include <stddef.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

typedef int uidq_socket_t;

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

} uidq_sock_conf_t;

int uidq_sock_conf_isvalid(uidq_sock_conf_t *conf);

int uidq_sock_init(uidq_sock_conf_t *conf);
void uidq_sock_exit(uidq_socket_t fd);

int uidq_sock_port_check(int domain, int type, int protocol, uint16_t port);

int uidq_sock_set_reuseaddr(uidq_socket_t fd, int on);
int uidq_sock_set_reuseport(uidq_socket_t fd, int on);
int uidq_sock_set_sndbuf(uidq_socket_t fd, int size);
int uidq_sock_set_rcvbuf(uidq_socket_t fd, int size);
int uidq_sock_set_keepalive(uidq_socket_t fd, int on, int idle, int intvl, int cnt);
int uidq_sock_set_nodelay(uidq_socket_t fd, int on);
int uidq_sock_set_fastopen(uidq_socket_t fd, int on);
int uidq_sock_set_nonblocking(uidq_socket_t fd, int on);
int uidq_sock_set_cloexec(uidq_socket_t fd, int on);

#endif /* UIDQ_SOCKET_INCLUDE_H */
