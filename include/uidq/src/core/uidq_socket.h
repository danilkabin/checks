#ifndef UIDQ_SOCKET_H 
#define UIDQ_SOCKET_H

#include "core/uidq_log.h"
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
   bool initialized;
   uint32_t count;
   uint32_t max_count;
   uidq_slab_t *sockets;

   uidq_log_t *log;
} uidq_sock_ctl_t;

// Validation functions
bool uidq_sock_ctl_isvalid(uidq_sock_ctl_t *ctl);
int uidq_sock_conf_isvalid(uidq_sock_conf_t *conf);

// Socket creation and management
int uidq_sock_port_check(int domain, int type, int protocol, uint16_t port);
int uidq_sock_set_flags(int fd, int flags, int fcntl_flags);
int uidq_sock_must_have(uidq_sock_conf_t *conf, struct sockaddr *addr, socklen_t len);
int uidq_sock_create_init(uidq_sock_conf_t *conf);
int uidq_sock_create_init6(uidq_sock_conf_t *conf);
int uidq_sock_create_unix(uidq_sock_conf_t *conf);
int uidq_sock_init(uidq_sock_conf_t *conf);
void uidq_sock_exit(int fd);

// Controller management
uidq_sock_t *uidq_sock_ctl_add(uidq_sock_ctl_t *ctl, uidq_sock_conf_t *conf);
void uidq_sock_ctl_del(uidq_sock_ctl_t *ctl, uidq_sock_t *sock);
uidq_sock_ctl_t *uidq_sock_ctl_init(uidq_log_t *log, uint32_t max_count);
void uidq_sock_ctl_exit(uidq_sock_ctl_t *ctl);

#endif /* UIDQ_SOCKET_H */
