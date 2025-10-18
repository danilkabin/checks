#ifndef OPIUM_EPOLL_INCLUDE_H 
#define OPIUM_EPOLL_INCLUDE_H

#include "core/opium_core.h"

#define OPIUM_EPOLLIN      0x001
#define OPIUM_EPOLLOUT     0x004
#define OPIUM_EPOLLERR     0x008
#define OPIUM_EPOLLHUP     0x010
#define OPIUM_EPOLLONESHOT 0x40000000
#define OPIUM_EPOLLET      0x80000000

#define OPIUM_EPOLL_CTL_ADD 1
#define OPIUM_EPOLL_CTL_DEL 2
#define OPIUM_EPOLL_CTL_MOD 3

typedef struct opium_epoll_s opium_epoll_t;

struct opium_epoll_s {
   opium_fd_t fd;
   opium_fd_t event_fd;
   opium_fd_t timer_fd;

   struct epoll_event event;
};

#endif /* OPIUM_EPOLL_INCLUDE_H */

