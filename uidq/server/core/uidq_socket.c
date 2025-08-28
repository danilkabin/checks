#include <asm-generic/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netinet/tcp.h>

#include "uidq_socket.h"
#include "core/uidq_core.h"
#include "core/uidq_slab.h"
#include "uidq_log.h"

#define UIDQ_SOCK_CONF_UNPACK(conf)       \
   int domain = (conf)->domain;           \
   int type = (conf)->type;               \
   int protocol = (conf)->protocol;       \
   int flags = (conf)->flags;             \
   int fcntl_flags = (conf)->fcntl_flags; \
   uint16_t port = (conf)->port;

static int uidq_sock_create_init(uidq_sock_conf_t *conf);
static int uidq_sock_create_init6(uidq_sock_conf_t *conf);
static int uidq_sock_create_unix(uidq_sock_conf_t *conf);

static int uidq_set_sock_opt(uidq_socket_t fd, int level, int optname, int val);
static int uidq_set_fcntl(uidq_socket_t fd, int level, int flag, int enable);

   int 
uidq_sock_conf_isvalid(uidq_sock_conf_t *conf) 
{
   if (!conf || conf->domain == 0 || conf->type == 0 || conf->port == 0) {
      uidq_err(NULL, "Invalid socket configuration\n");
      return UIDQ_RET_ERR;
   }
   return UIDQ_RET_OK;
}

   int 
uidq_sock_init(uidq_sock_conf_t *conf) 
{
   if (!uidq_sock_conf_isvalid(conf)) {
      uidq_err(NULL, "Invalid socket configuration\n");
      return UIDQ_RET_ERR;
   }
   if (conf->domain == AF_INET) {
      return uidq_sock_create_init(conf);
   } else if (conf->domain == AF_INET6) {
      return uidq_sock_create_init6(conf);
   } else if (conf->domain == AF_UNIX) {
      return uidq_sock_create_unix(conf);
   }
   uidq_err(NULL, "Unsupported domain\n");
   return UIDQ_RET_ERR;
}

   void
uidq_sock_exit(uidq_socket_t fd) 
{
   if (fd >= 0) {
      close(fd);
      uidq_debug(NULL, "Socket %d closed\n", fd);
   }
}

   int
uidq_sock_port_check(int domain, int type, int protocol, uint16_t port)
{
   uidq_socket_t sock_fd = socket(domain, type, protocol);
   if (sock_fd < 0) {
      uidq_err(NULL, "Failed to create socket: %s\n", strerror(errno));
      return UIDQ_RET_ERR;
   }

   int ret = -1;
   if (domain == AF_INET) {
      struct sockaddr_in sock_addr = {0};
      sock_addr.sin_family = AF_INET;
      sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      sock_addr.sin_port = htons(port);
      ret = bind(sock_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
   } else if (domain == AF_INET6) {
      struct sockaddr_in6 sock_addr = {0};
      sock_addr.sin6_family = AF_INET6;
      sock_addr.sin6_addr = in6addr_any;
      sock_addr.sin6_port = htons(port);
      ret = bind(sock_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
   } else {
      uidq_err(NULL, "Unsupported domain\n");
   }

   close(sock_fd);
   if (ret < 0) {
      uidq_err(NULL, "Port %u bind failed: %s\n", port, strerror(errno));
   }

   return ret;
}

   int
uidq_sock_set_reuseaddr(uidq_socket_t fd, int on) 
{
   return uidq_set_sock_opt(fd, SOL_SOCKET, SO_REUSEADDR, on);
}

   int
uidq_sock_set_reuseport(uidq_socket_t fd, int on)
{
   return uidq_set_sock_opt(fd, SOL_SOCKET, SO_REUSEPORT, on);
}

   int
uidq_sock_set_sndbuf(uidq_socket_t fd, int size)
{
   return uidq_set_sock_opt(fd, SOL_SOCKET, SO_SNDBUF, size);
}

   int
uidq_sock_set_rcvbuf(uidq_socket_t fd, int size)
{
   return uidq_set_sock_opt(fd, SOL_SOCKET, SO_RCVBUF, size);
}

   int
uidq_sock_set_keepalive(uidq_socket_t fd, int on, int idle, int intvl, int cnt)
{
   if (uidq_set_sock_opt(fd, SOL_SOCKET, SO_KEEPALIVE, on) != UIDQ_RET_OK)
      return UIDQ_RET_ERR;

#ifdef TCP_KEEPIDLE
   if (uidq_set_sock_opt(fd, IPPROTO_TCP, TCP_KEEPIDLE, idle) != UIDQ_RET_OK)
      return UIDQ_RET_ERR;
#endif
#ifdef TCP_KEEPINTVL
   if (uidq_set_sock_opt(fd, IPPROTO_TCP, TCP_KEEPINTVL, intvl) != UIDQ_RET_OK)
      return UIDQ_RET_ERR;
#endif
#ifdef TCP_KEEPCNT 
   if (uidq_set_sock_opt(fd, IPPROTO_TCP, TCP_KEEPCNT, cnt) != UIDQ_RET_OK)
      return UIDQ_RET_ERR;
#endif

   return UIDQ_RET_OK;
}

   int
uidq_sock_set_nodelay(uidq_socket_t fd, int on)
{
   return uidq_set_sock_opt(fd, IPPROTO_TCP, TCP_NODELAY, on);
}

   int
uidq_sock_set_fastopen(uidq_socket_t fd, int on)
{
#ifdef TCP_FASTOPEN
   return uidq_set_sock_opt(fd, IPPROTO_TCP, TCP_FASTOPEN, on);
#else
   return UIDQ_RET_ERR;
#endif
}

   int
uidq_sock_set_nonblocking(uidq_socket_t fd, int on)
{
   return uidq_set_fcntl(fd, F_GETFD, O_NONBLOCK, on);
}

   int
uidq_sock_set_cloexec(uidq_socket_t fd, int on)
{
   return uidq_set_fcntl(fd, F_GETFL, O_CLOEXEC, on);
}

   int
uidq_sock_get_err(uidq_socket_t fd)
{
   int err = 0;
   socklen_t len = sizeof(err);
   if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
      return errno;
   return err;
}

   int
uidq_sock_get_local(uidq_socket_t fd, struct sockaddr *addr, socklen_t *len)
{
   return getsockname(fd, addr, len) < 0 ? UIDQ_RET_ERR : UIDQ_RET_OK;
}

   int
uidq_sock_get_peer(uidq_socket_t fd, struct sockaddr *addr, socklen_t *len)
{
   return getpeername(fd, addr, len) < 0 ? UIDQ_RET_ERR : UIDQ_RET_OK;
}

   int
uidq_sock_shutdown(uidq_socket_t fd, int how)
{
   return shutdown(fd, how) < 0 ? UIDQ_RET_ERR : UIDQ_RET_OK;
}

   int 
uidq_sock_must_have(uidq_sock_conf_t *conf, struct sockaddr *addr, socklen_t len) 
{
   UIDQ_SOCK_CONF_UNPACK(conf);

   // Ensure minimum queue size for small connections
   size_t queue_capable = conf->queue_capable <= 0 ? 1 : conf->queue_capable;

   uidq_socket_t fd = socket(domain, type, protocol);
   if (fd < 0) {
      uidq_err(NULL, "Failed to create socket: %s\n", strerror(errno));
      goto fail;
   }

   if (domain == AF_UNIX) {
      struct sockaddr_un *un_addr = (struct sockaddr_un*)addr;
      if (strlen(un_addr->sun_path) > 0) {
         unlink(un_addr->sun_path); // Clean up existing socket file
      }
   }

   if (bind(fd, addr, len) < 0) {
      uidq_err(NULL, "Failed to bind socket: %s\n", strerror(errno));
      goto fail;
   }

   if (type == SOCK_STREAM) {
      if (listen(fd, queue_capable) < 0) {
         uidq_err(NULL, "Failed to listen on socket: %s\n", strerror(errno));
         goto fail;
      }
   }

   return fd;
fail:
   if (fd >= 0) close(fd);
   return UIDQ_RET_ERR;
}

   static int
uidq_set_sock_opt(uidq_socket_t fd, int level, int optname, int val)
{
   if (setsockopt(fd, level, optname, &val, sizeof(val)) < 0) {
      return UIDQ_RET_ERR;
   }
   return UIDQ_RET_OK;
}

   static int 
uidq_set_fcntl(uidq_socket_t fd, int level, int flag, int enable)
{
   int flags;
   flags = fcntl(fd, level, 0);
   if (flags < 0) return UIDQ_RET_ERR;

   if (enable) {
      flags |= flag;
   } else {
      flags &= ~flag;
   }

   if (fcntl(fd, level == F_GETFD ? F_SETFD : F_SETFL, flags) < 0) {
      return UIDQ_RET_ERR;
   }

   return UIDQ_RET_OK;
}

   static int
uidq_sock_create_init(uidq_sock_conf_t *conf)
{
   UIDQ_SOCK_CONF_UNPACK(conf);

   struct sockaddr_in sock_addr = {0};
   socklen_t addr_len = sizeof(sock_addr);
   sock_addr.sin_family = domain;
   sock_addr.sin_addr = conf->sock_addr.sin_addr;
   sock_addr.sin_port = htons(port);

   uidq_socket_t fd = uidq_sock_must_have(conf, (struct sockaddr*)&sock_addr, addr_len);
   if (fd < 0) {
      uidq_err(NULL, "Failed to initialize IPv4 socket\n");
      return UIDQ_RET_ERR;
   }
   return fd;
}

   static int
uidq_sock_create_init6(uidq_sock_conf_t *conf)
{
   UIDQ_SOCK_CONF_UNPACK(conf);

   struct sockaddr_in6 sock_addr6 = {0};
   socklen_t addr_len = sizeof(sock_addr6);
   sock_addr6.sin6_family = domain;
   sock_addr6.sin6_addr = conf->sock_addr6.sin6_addr;
   sock_addr6.sin6_port = htons(port);

   uidq_socket_t fd = uidq_sock_must_have(conf, (struct sockaddr*)&sock_addr6, addr_len);
   if (fd < 0) {
      uidq_err(NULL, "Failed to initialize IPv6 socket\n");
      return UIDQ_RET_ERR;
   }
   return fd;
}

   static int
uidq_sock_create_unix(uidq_sock_conf_t *conf) 
{
   UIDQ_SOCK_CONF_UNPACK(conf);

   const char *path = conf->sock_un.sun_path;
   if (!path || strlen(path) == 0) {
      uidq_err(NULL, "Invalid UNIX socket path\n");
      return UIDQ_RET_ERR;
   }

   struct sockaddr_un addr = {0};
   addr.sun_family = domain;
   strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

   socklen_t addr_len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

   uidq_socket_t fd = uidq_sock_must_have(conf, (struct sockaddr*)&addr, addr_len);
   if (fd < 0) {
      uidq_err(NULL, "Failed to initialize UNIX socket\n");
      return UIDQ_RET_ERR;
   }
   return fd;
}
