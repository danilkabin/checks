#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "opium_socket.h"
#include "core/opium_core.h"
#include "core/opium_slab.h"
#include "opium_log.h"

#define OPIUM_SOCK_CONF_UNPACK(conf)       \
   int domain       = (conf)->domain;      \
   int type         = (conf)->type;        \
   int protocol     = (conf)->protocol;    \
   int flags        = (conf)->flags;       \
   int fcntl_flags  = (conf)->fcntl_flags; \
   uint16_t port    = (conf)->port;

/* Static Helpers */
static int opium_sock_must_have(opium_sock_conf_t *conf, struct sockaddr *addr, socklen_t len);

static int opium_sock_create_init(opium_sock_conf_t *conf);
static int opium_sock_create_init6(opium_sock_conf_t *conf);
static int opium_sock_create_unix(opium_sock_conf_t *conf);

   static int
opium_set_sock_opt(opium_socket_t fd, int level, int optname, int val)
{
   if (setsockopt(fd, level, optname, &val, sizeof(val)) < 0)
      return OPIUM_RET_ERR;
   return OPIUM_RET_OK;
}

   static int
opium_set_fcntl(opium_socket_t fd, int level, int flag, int enable)
{
   int flags = fcntl(fd, level, 0);
   if (flags < 0) 
      return OPIUM_RET_ERR;

   if (enable) {
      flags |= flag;
   } else {
      flags &= ~flag;
   }

   if (fcntl(fd, level == F_GETFD ? F_SETFD : F_SETFL, flags) < 0)
      return OPIUM_RET_ERR;

   return OPIUM_RET_OK;
}

/* Validation */
   int
opium_sock_is_valid(opium_socket_t fd)
{
   return fd >= 0 ? OPIUM_RET_OK : OPIUM_RET_ERR;
}

   int
opium_sock_conf_isvalid(opium_sock_conf_t *conf)
{
   if (!conf || conf->domain == 0 || conf->type == 0 || conf->port == 0) {
      opium_err(NULL, "Invalid socket configuration\n");
      return OPIUM_RET_ERR;
   }
   return OPIUM_RET_OK;
}

/* Socket Creation */
   int
opium_sock_init(opium_sock_conf_t *conf)
{
   if (!opium_sock_conf_isvalid(conf)) {
      opium_err(NULL, "Invalid socket configuration\n");
      return OPIUM_RET_ERR;
   }

   if (conf->domain == AF_INET)
      return opium_sock_create_init(conf);
   else if (conf->domain == AF_INET6)
      return opium_sock_create_init6(conf);
   else if (conf->domain == AF_UNIX)
      return opium_sock_create_unix(conf);

   opium_err(NULL, "Unsupported domain\n");
   return OPIUM_RET_ERR;
}

   static int
opium_sock_create_init(opium_sock_conf_t *conf)
{
   OPIUM_SOCK_CONF_UNPACK(conf);

   struct sockaddr_in sock_addr = {0};
   sock_addr.sin_family      = domain;
   sock_addr.sin_addr        = conf->sock_addr.sin_addr;
   sock_addr.sin_port        = htons(port);

   socklen_t addr_len = sizeof(sock_addr);

   opium_socket_t fd = opium_sock_must_have(conf, (struct sockaddr*)&sock_addr, addr_len);
   if (fd < 0) {
      opium_err(NULL, "Failed to initialize IPv4 socket\n");
      return OPIUM_RET_ERR;
   }
   return fd;
}

   static int
opium_sock_create_init6(opium_sock_conf_t *conf)
{
   OPIUM_SOCK_CONF_UNPACK(conf);

   struct sockaddr_in6 sock_addr6 = {0};
   sock_addr6.sin6_family = domain;
   sock_addr6.sin6_addr   = conf->sock_addr6.sin6_addr;
   sock_addr6.sin6_port   = htons(port);

   socklen_t addr_len = sizeof(sock_addr6);

   opium_socket_t fd = opium_sock_must_have(conf, (struct sockaddr*)&sock_addr6, addr_len);
   if (fd < 0) {
      opium_err(NULL, "Failed to initialize IPv6 socket\n");
      return OPIUM_RET_ERR;
   }
   return fd;
}

   static int
opium_sock_create_unix(opium_sock_conf_t *conf)
{
   OPIUM_SOCK_CONF_UNPACK(conf);

   const char *path = conf->sock_un.sun_path;
   if (!path || strlen(path) == 0) {
      opium_err(NULL, "Invalid UNIX socket path\n");
      return OPIUM_RET_ERR;
   }

   struct sockaddr_un addr = {0};
   addr.sun_family = domain;
   strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

   socklen_t addr_len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

   opium_socket_t fd = opium_sock_must_have(conf, (struct sockaddr*)&addr, addr_len);
   if (fd < 0) {
      opium_err(NULL, "Failed to initialize UNIX socket\n");
      return OPIUM_RET_ERR;
   }
   return fd;
}

/* Socket Lifecycle */
   void
opium_sock_exit(opium_socket_t fd)
{
   if (fd >= 0) {
      close(fd);
      opium_debug(NULL, "Socket %d closed\n", fd);
   }
}

   opium_socket_t
opium_socket_dup(opium_socket_t fd)
{
   if (!opium_sock_is_valid(fd)) {
      return OPIUM_RET_ERR;
   }

   int new = dup(fd);
   if (!opium_sock_is_valid(new)) {
      return OPIUM_RET_ERR;
   }

   return new;
}

   int
opium_sock_port_check(int domain, int type, int protocol, uint16_t port)
{
   opium_socket_t sock_fd = socket(domain, type, protocol);
   if (sock_fd < 0) {
      opium_err(NULL, "Failed to create socket: %s\n", strerror(errno));
      return OPIUM_RET_ERR;
   }

   int ret = -1;
   if (domain == AF_INET) {
      struct sockaddr_in sock_addr = {0};
      sock_addr.sin_family      = AF_INET;
      sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      sock_addr.sin_port        = htons(port);
      ret = bind(sock_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
   } else if (domain == AF_INET6) {
      struct sockaddr_in6 sock_addr = {0};
      sock_addr.sin6_family = AF_INET6;
      sock_addr.sin6_addr   = in6addr_any;
      sock_addr.sin6_port   = htons(port);
      ret = bind(sock_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
   } else {
      opium_err(NULL, "Unsupported domain\n");
   }

   close(sock_fd);
   if (ret < 0)
      opium_err(NULL, "Port %u bind failed: %s\n", port, strerror(errno));

   return ret;
}

   int
opium_sock_must_have(opium_sock_conf_t *conf, struct sockaddr *addr, socklen_t len)
{
   OPIUM_SOCK_CONF_UNPACK(conf);

   size_t queue_capable = conf->queue_capable <= 0 ? 1 : conf->queue_capable;

   opium_socket_t fd = socket(domain, type, protocol);
   if (fd < 0) {
      opium_err(NULL, "Failed to create socket: %s\n", strerror(errno));
      goto fail;
   }

   if (domain == AF_UNIX) {
      struct sockaddr_un *un_addr = (struct sockaddr_un*)addr;
      if (strlen(un_addr->sun_path) > 0)
         unlink(un_addr->sun_path); // cleanup existing socket file
   }

   if (bind(fd, addr, len) < 0) {
      opium_err(NULL, "Failed to bind socket: %s\n", strerror(errno));
      goto fail;
   }

   if (type == SOCK_STREAM) {
      if (listen(fd, queue_capable) < 0) {
         opium_err(NULL, "Failed to listen on socket: %s\n", strerror(errno));
         goto fail;
      }
   }

   return fd;

fail:
   if (fd >= 0) close(fd);
   return OPIUM_RET_ERR;
}

/* Socket Options */
   int
opium_sock_set_reuseaddr(opium_socket_t fd, int on)
{
   return opium_set_sock_opt(fd, SOL_SOCKET, SO_REUSEADDR, on);
}

   int
opium_sock_set_reuseport(opium_socket_t fd, int on)
{
   return opium_set_sock_opt(fd, SOL_SOCKET, SO_REUSEPORT, on);
}

   int
opium_sock_set_sndbuf(opium_socket_t fd, int size)
{
   return opium_set_sock_opt(fd, SOL_SOCKET, SO_SNDBUF, size);
}

   int
opium_sock_set_rcvbuf(opium_socket_t fd, int size)
{
   return opium_set_sock_opt(fd, SOL_SOCKET, SO_RCVBUF, size);
}

   int
opium_sock_set_keepalive(opium_socket_t fd, int on, int idle, int intvl, int cnt)
{
   if (opium_set_sock_opt(fd, SOL_SOCKET, SO_KEEPALIVE, on) != OPIUM_RET_OK)
      return OPIUM_RET_ERR;

#ifdef TCP_KEEPIDLE
   if (opium_set_sock_opt(fd, IPPROTO_TCP, TCP_KEEPIDLE, idle) != OPIUM_RET_OK)
      return OPIUM_RET_ERR;
#endif
#ifdef TCP_KEEPINTVL
   if (opium_set_sock_opt(fd, IPPROTO_TCP, TCP_KEEPINTVL, intvl) != OPIUM_RET_OK)
      return OPIUM_RET_ERR;
#endif
#ifdef TCP_KEEPCNT
   if (opium_set_sock_opt(fd, IPPROTO_TCP, TCP_KEEPCNT, cnt) != OPIUM_RET_OK)
      return OPIUM_RET_ERR;
#endif

   return OPIUM_RET_OK;
}

   int
opium_sock_set_nodelay(opium_socket_t fd, int on)
{
   return opium_set_sock_opt(fd, IPPROTO_TCP, TCP_NODELAY, on);
}

   int
opium_sock_set_fastopen(opium_socket_t fd, int on)
{
#ifdef TCP_FASTOPEN
   return opium_set_sock_opt(fd, IPPROTO_TCP, TCP_FASTOPEN, on);
#else
   return OPIUM_RET_ERR;
#endif
}

   int
opium_sock_set_nonblocking(opium_socket_t fd, int on)
{
   return opium_set_fcntl(fd, F_GETFD, O_NONBLOCK, on);
}

   int
opium_sock_set_cloexec(opium_socket_t fd, int on)
{
   return opium_set_fcntl(fd, F_GETFL, O_CLOEXEC, on);
}

/* Socket Info/Control */
   int
opium_sock_get_err(opium_socket_t fd)
{
   int err = 0;
   socklen_t len = sizeof(err);
   if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
      return errno;
   return err;
}

   int
opium_sock_get_local(opium_socket_t fd, struct sockaddr *addr, socklen_t *len)
{
   return getsockname(fd, addr, len) < 0 ? OPIUM_RET_ERR : OPIUM_RET_OK;
}

   int
opium_sock_get_peer(opium_socket_t fd, struct sockaddr *addr, socklen_t *len)
{
   return getpeername(fd, addr, len) < 0 ? OPIUM_RET_ERR : OPIUM_RET_OK;
}

   int
opium_sock_shutdown(opium_socket_t fd, int how)
{
   return shutdown(fd, how) < 0 ? OPIUM_RET_ERR : OPIUM_RET_OK;
}
