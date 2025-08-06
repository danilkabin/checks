#ifndef UIDQ_SOCKET_H 
#define UIDQ_SOCKET_H

#include "uidq_slab.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>

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

typedef struct {
   int fd;
   int domain;
   int type;
   int queue_capable;
 
   uint32_t packets_sent;
   uint32_t packets_received;

   union {
      struct sockaddr_in sock_addr;
      struct sockaddr_in6 sock_addr6;
      struct sockaddr_un sock_un;
   };

} uidq_sock_t;

typedef struct {
   uint32_t count;
   uint32_t max_count;
   uidq_slab_t *sockets;

   bool initialized;
} uidq_sock_ctl_t;

int uidq_socket_port_check(int domain, int type, int protocol, uint16_t port);

int uidq_sock_conf_isvalid(uidq_sock_conf_t *conf);

int uidq_socket_port_check(int domain, int type, int protocol, uint16_t port);
int uidq_sock_set_flags(int fd, int flags, int fcntl_flags);

int uidq_sock_init(uidq_sock_conf_t *conf);
void uidq_sock_exit(int fd);

uidq_sock_ctl_t *uidq_sock_ctl_init(uint32_t max_count);
void uidq_sock_ctl_exit(uidq_sock_ctl_t *ctl);


#endif
