#include "core/opium_core.h"
#include <sys/epoll.h>

void *opium_event_cb(void *data) {

   return NULL;
}

   opium_int_t
opium_event_init(opium_event_t *event, opium_arena_t *arena, opium_log_t *log)
{
   opium_err_t err;

   event->fd = epoll_create1(0);
   if (event->fd < 0) {
      opium_log_err(log, "epoll_create1() failed\n");
      return OPIUM_RET_ERR;
   }


   err = opium_thread_init(&event->thread, opium_event_cb, event, log);
   if (err != OPIUM_RET_OK) {
      opium_log_err(log, "opium_thread_init() failed\n");
      return OPIUM_RET_ERR;
   }

   return OPIUM_RET_OK;
}

   void
opium_event_exit(opium_event_t *event)
{
}
