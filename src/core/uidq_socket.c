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

#include "uidq_socket.h"
#include "uidq_log.h"
#include "uidq_utils.h"

#define UIDQ_SOCK_CONF_UNPACK(conf)       \
   int domain = (conf)->domain;           \
   int type = (conf)->type;               \
   int protocol = (conf)->protocol;       \
   int flags = (conf)->flags;             \
   int fcntl_flags = (conf)->fcntl_flags; \
   uint16_t port = (conf)->port;          \

bool uidq_sock_ctl_isvalid(uidq_sock_ctl_t *ctl) {
   if (!ctl || !ctl->initialized) {
      return false;
   }
   return true;
}

int uidq_sock_conf_isvalid(uidq_sock_conf_t *conf) {
   if (!conf) {
      return -1;
   }
   if (conf->domain == 0) return -1;
   if (conf->type == 0) return -1;
   if (conf->port == 0) return -1;
   return 1;
}

int uidq_socket_port_check(int domain, int type, int protocol, uint16_t port) {
   int sock_fd = socket(domain, type, protocol);
   if (sock_fd < 0) {
      fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
      return -1;
   }

   int ret = -1;

   if (domain == AF_INET) {
      struct sockaddr_in sock_addr;
      memset(&sock_addr, 0, sizeof(sock_addr));
      sock_addr.sin_family = AF_INET;
      sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      sock_addr.sin_port = htons(port);
      ret = bind(sock_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));

   } else if (domain == AF_INET6) {
      struct sockaddr_in6 sock_addr;
      memset(&sock_addr, 0, sizeof(sock_addr));
      sock_addr.sin6_family = AF_INET6;
      sock_addr.sin6_addr = in6addr_any;
      sock_addr.sin6_port = htons(port);
      ret = bind(sock_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));

   } else {
      DEBUG_FUNC("Unsupported domain.\n");
   }

   close(sock_fd);

   return (ret == 0);
}

int uidq_sock_set_flags(int fd, int flags, int fcntl_flags) {
   int opt = 1;

   if (flags & SO_REUSEADDR) {
      if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
         DEBUG_ERR("Failed to set socket options[SO_REUSEADDR]\n");
         return -1;
      }
   }

   if (flags & SO_REUSEPORT) {
      if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
         DEBUG_ERR("Failed to set socket options[SO_REUSEPORT]\n");
         return -1;
      }
   }

   int fcntl_result = fcntl(fd, F_GETFL, 0);
   if (fcntl_result == -1) {
      DEBUG_ERR("Failed to get peer socket flags\n");
      return -1;
   }

   fcntl_result = fcntl(fd, F_SETFL, fcntl_result | fcntl_flags);
   if (fcntl_result == -1) {
      DEBUG_ERR("Failed to set peer socket flags\n");
      return -1;
   }

   return 1;
}

int uidq_sock_must_have(uidq_sock_conf_t *conf, struct sockaddr *addr, socklen_t len) {
   UIDQ_SOCK_CONF_UNPACK(conf);

   size_t queue_capable = conf->queue_capable <= 0 ? 1 : conf->queue_capable;

   int fd = socket(domain, type, protocol);
   if (fd < 0) {
      DEBUG_ERR("Failed to create socket.\n");
      return -1;
   }

   if (domain == AF_INET || domain == AF_INET6) {
      if (uidq_sock_set_flags(fd, flags, fcntl_flags) < 0) {
         DEBUG_ERR("Failed to set flags.\n");
         goto fail;
      }
   }

   if (domain == AF_UNIX) {
      struct sockaddr_un *un_addr = (struct sockaddr_un*)addr;
      unlink(un_addr->sun_path);
   }

   if (bind(fd, addr, len) < 0) {
      DEBUG_ERR("Failed to bind socket.\n");
      goto fail;
   }

   if (type == SOCK_STREAM) {
      if (listen(fd, queue_capable) < 0) {
         DEBUG_ERR("Failed to listen socket.\n");
         goto fail;
      }
   }

   return fd;
fail:
   close(fd);
   return -1;
}

int uidq_sock_create_init(uidq_sock_conf_t *conf) {
   UIDQ_SOCK_CONF_UNPACK(conf);

   struct sockaddr_in sock_addr;
   socklen_t addr_len = sizeof(sock_addr);
   memset(&sock_addr, 0, addr_len);

   sock_addr.sin_family = domain;
   sock_addr.sin_addr = conf->sock_addr.sin_addr;
   sock_addr.sin_port = htons(port);

   int fd = uidq_sock_must_have(conf, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
   if (fd < 0) {
      DEBUG_ERR("Failed to initialize socket.\n");
      return -1;
   }

   return fd;
}

int uidq_sock_create_init6(uidq_sock_conf_t *conf) {
   UIDQ_SOCK_CONF_UNPACK(conf);

   struct sockaddr_in6 sock_addr6;
   socklen_t addr_len = sizeof(sock_addr6);
   memset(&sock_addr6, 0, addr_len);

   sock_addr6.sin6_family = domain;
   sock_addr6.sin6_addr = conf->sock_addr6.sin6_addr;
   sock_addr6.sin6_port = htons(port);

   int fd = uidq_sock_must_have(conf, (struct sockaddr*)&sock_addr6, sizeof(sock_addr6));
   if (fd < 0) {
      DEBUG_ERR("Failed to initialize socket.\n");
      return -1;
   }

   return fd;
}

int uidq_sock_create_unix(uidq_sock_conf_t *conf) {
    UIDQ_SOCK_CONF_UNPACK(conf);

    const char *path = conf->sock_un.sun_path;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));

    addr.sun_family = domain;

    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    socklen_t addr_len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

    int fd = uidq_sock_must_have(conf, (struct sockaddr*)&addr, addr_len);
    if (fd < 0) {
        DEBUG_ERR("Failed to initialize UNIX socket.\n");
        return -1;
    }

    return fd;
}

int uidq_sock_init(uidq_sock_conf_t *conf) {
   int fd = -1;
   if (conf->domain == AF_INET) {
      return uidq_sock_create_init(conf);
   } else if (conf->domain == AF_INET6) {
      return uidq_sock_create_init6(conf);
   } else if (conf->domain == AF_UNIX) {
      return uidq_sock_create_unix(conf);
   } else {
      DEBUG_FUNC("Unsupported config domain.\n");
      return -1;
   }
}

void uidq_sock_exit(int fd) {
   close(fd);
}

uidq_sock_t *uidq_sock_ctl_add(uidq_sock_ctl_t *ctl, uidq_sock_conf_t *conf) {
   if (!uidq_sock_ctl_isvalid(ctl)) {
      DEBUG_ERR("Controller is invalid.\n");
      goto fail;
   }

   uidq_slab_t *sockets = ctl->sockets;

   int fd = uidq_sock_init(conf);
   if (fd < 0) {
      DEBUG_ERR("Failed to allocate socket.\n");
      goto fail;
   }

   size_t sock_size = sizeof(uidq_sock_t);
   int pos = uidq_slab_alloc(sockets, NULL, sock_size);
   if (pos < 0) {
      DEBUG_ERR("Failed to allocate proc into slab.\n");
      goto close_sock;
   }

   int domain = conf->domain; 
   int type = conf->type;
   size_t queue_capable = conf->queue_capable;

   uidq_sock_t *sock = uidq_slab_get(sockets, pos);

   sock->fd = fd;
   sock->domain = domain;
   sock->type = type;
   sock->queue_capable = queue_capable;
   sock->packets_sent = 0;
   sock->packets_received = 0;

   if (domain == AF_INET) {      
      sock->sock_addr = conf->sock_addr;
   } else if (domain == AF_INET6) {      
      sock->sock_addr6 = conf->sock_addr6;
   } else if (domain == AF_UNIX) {
      sock->sock_un = conf->sock_un;
   }

   ctl->count = ctl->count + 1;

   return sock;

close_sock:
   uidq_sock_exit(fd);
fail:
   return NULL;
}

void uidq_sock_ctl_del(uidq_sock_ctl_t *ctl, uidq_sock_t *sock) {

} 

uidq_sock_ctl_t *uidq_sock_ctl_init(uint32_t max_count) {
   if (max_count == 0) {
      DEBUG_ERR("Invalid data.\n");
      goto fail;
   }

   size_t ctl_size = sizeof(uidq_sock_ctl_t);
   uidq_sock_ctl_t *ctl = malloc(ctl_size);
   if (!ctl) {
      DEBUG_ERR("Failed to allocate controller.\n");
      goto fail;
   }

   ctl->sockets = uidq_slab_create_and_init(max_count, sizeof(uidq_sock_t)); 
   if (!ctl->sockets) {\
      DEBUG_ERR("Failed to allocate ctl sockets.\n");
      goto ctl_exit;
   }

   ctl->count = 0;
   ctl->max_count = max_count;
   ctl->initialized = true;

   return ctl;
ctl_exit:
   uidq_sock_ctl_exit(ctl);
fail:
   return NULL;
}

void uidq_sock_ctl_exit(uidq_sock_ctl_t *ctl) {
   if (!ctl) {
      return;
   }

   if (ctl->sockets) {
      uidq_slab_exit(ctl->sockets);
      ctl->sockets = NULL;
   }

   ctl->count = 0;
   ctl->max_count = 0;
   ctl->initialized = false;

   free(ctl);
}
