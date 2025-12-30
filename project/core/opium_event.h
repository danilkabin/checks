#ifndef OPIUM_EVENT_CONNECTION_H 
#define OPIUM_EVENT_CONNECTION_H

#include "core/opium_core.h"

#define OPIUM_EVENT_READ  EPOLLIN
#define OPIUM_EVENT_WRITE EPOLLOUT 
#define OPIUM_EVENT_ERR   EPOLLER | EPOLLHUP 

struct opium_event_s {
   opium_fd_t             fd;

   struct epoll_event     events;
   size_t                 max_events;

   opium_thread_t         thread;
   
   opium_arena_t         *arena;
   opium_log_t           *log;
};

opium_s32_t  opium_event_init(opium_event_t *event, opium_arena_t *arena, size_t mevents, opium_log_t *log);
void opium_event_exit(opium_event_t *event);

#endif /* OPIUM_EVENT_CONNECTION_H */
