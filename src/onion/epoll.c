#define _GNU_SOURCE
#include "epoll.h"
#include "utils.h"
#include "pool.h"
#include "sup.h"

#include <fcntl.h>
#include <bits/time.h>
#include <bits/types/struct_itimerspec.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

int onion_set_worker_core(pthread_t thread, int core_id) {
   cpu_set_t *set_t = CPU_ALLOC(core_id + 1);
   size_t setsize = CPU_ALLOC_SIZE(core_id + 1);
   CPU_ZERO_S(setsize, set_t);
   CPU_SET_S(core_id, setsize, set_t);
   int ret = pthread_setaffinity_np(thread, setsize, set_t);
   if (ret != 0) {
      DEBUG_ERR("Failed to set thread affinity to core %d\n", core_id);
      CPU_FREE(set_t);
      return ret;
   }
   CPU_FREE(set_t);
   return 0;
}

int onion_fd_is_valid(int fd) {
   if (fd < 0) {
      return -1;
   }

   if (fcntl(fd, F_GETFD) == -1 && errno == EBADF) {
      DEBUG_ERR("Fd descriptor is invalid\n");
      return -1;
   }
   return 0;
}

void onion_epoll_tag_set(onion_epoll_tag_t *tag, int fd, onion_handler_ret_t type, void *data) {
   tag->fd = fd;
   tag->type = type;
   tag->user_data = data;
}

int onion_epoll_event_add(onion_epoll_t *ep, int fd, onion_handler_ret_t type, void *data) {
   onion_epoll_tag_t *tag = (onion_epoll_tag_t*)onion_block_alloc(ep->tags, -1);
   if (!tag) {
      DEBUG_ERR("Epoll tag initialization failed!\n");
      return -1;
   }
   onion_epoll_tag_set(tag, fd, type, data);
   struct epoll_event timer_event = {.events = EPOLLIN, .data.ptr = tag};
   int ret = epoll_ctl(ep->fd, EPOLL_CTL_ADD, fd, &timer_event);
   if (ret < 0) {
      DEBUG_ERR("Epoll ctl add timer failed!\n");
      onion_epoll_tag_set(tag, -1, ONION_EPOLL_HANDLER_UNKNOWN, NULL);
      return -1;
   }
   return 0;
}

time_t onion_tick() {
   struct timespec time;
   clock_gettime(CLOCK_MONOTONIC_RAW, &time);
   return time.tv_sec;
}

int onion_integrate_timer(onion_epoll_t *ep) {
   int ret;
   ep->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
   if (onion_fd_is_valid(ep->timerfd) == -1) {
      DEBUG_FUNC("Epoll timer failed!\n");
      return -1;
   }

   struct itimerspec timer_spec = {
      .it_interval = {ONION_TIMER_INTERVAL, 0},
      .it_value = {ONION_TIMER_INTERVAL, 0}
   };

   ret = timerfd_settime(ep->timerfd, 0, &timer_spec, NULL);
   if (ret < 0) {
      DEBUG_ERR("Epoll timerfd settime failed!\n");
      return -1;
   }

   ret = onion_epoll_event_add(ep, ep->timerfd, ONION_EPOLL_HANDLER_TIMERFD, NULL);
   if (ret < 0) {
      DEBUG_ERR("Epoll tag initialization failed!\n");
      return -1;
   }

   return 0;
}

void onion_liquidate_timer(onion_epoll_t *ep) {
   if (onion_fd_is_valid(ep->timerfd) == 0) {
      if (onion_fd_is_valid(ep->fd) == 0) {
         epoll_ctl(ep->fd, EPOLL_CTL_DEL, ep->timerfd, NULL);
      }
      close(ep->timerfd);
      ep->timerfd = -1;
   }
}

int onion_handle_timer(onion_epoll_t *ep) {
   struct onion_block *pool = ep->slots;
   size_t offset = 0;
   int start_pos = -1;
   while ((start_pos = onion_ffb(ep->bitmask, offset, 1)) != -1) {
      offset = start_pos + 1;
      onion_epoll_slot_t *slot = (onion_epoll_slot_t*)onion_block_get(pool, start_pos);
      if (slot->start_pos != start_pos) {
         continue;
      }

      //DEBUG_FUNC("from core: %d hello: %ld\n", ep->core, onion_tick() - slot->time_alive);

      time_t sleepy = onion_tick() - slot->time_alive;
      if (sleepy >= slot->time_limit) {
         DEBUG_FUNC("Anonymous is offline man!\n");
         onion_epoll_slot_del(ep, slot);
      }
   }
   return -1;
}

void *onion_epoll_hander(void *arg) {
   int ret;
   onion_epoll_t *ep = (onion_epoll_t*)arg;
   if (!ep) {
      DEBUG_ERR("I want arg!\n");
      goto unsuccessfull;
   }

   ret = onion_set_worker_core(pthread_self(), ep->core);
   if (ret < 0) {
      DEBUG_ERR("Failed to set thread affinity inside handler, core = %d\n", ep->core);
      goto free_epoll;
   } else {
      DEBUG_FUNC("Thread affinity set to core: %d\n", ep->core);
   }

   struct epoll_event events[10];
   while (ep && ep->active) {
      int bindables = epoll_wait(ep->fd, events, 64, -1);   
      if (bindables < 0) {
         continue;
      }

      for (int index = 0; index < bindables; index++) {
         struct epoll_event event = events[index];
         uint64_t expirations;
         if (event.events & EPOLLIN) {
            if (event.data.fd) {
               if (event.data.fd == ep->timerfd) {
                  read(ep->timerfd, &expirations, sizeof(expirations));
                  ret = onion_handle_timer(ep);
               }
            }

         }
      }

      if (ep->handler) {
         ep->handler(ep, -1);
      }
   }

   return NULL;
free_epoll:
   onion_epoll1_exit(ep);
unsuccessfull:
   return NULL;
}

int onion_epoll_slot_add(onion_epoll_t *ep, int fd, void *data, int (*func) (void*), void (*shutdown) (void*), void *shutdown_data) {
   int ret;

   if (ep->conn_count + 1 >= ep->conn_max) {
      DEBUG_ERR("Epoll slot add limit!\n");
      return -1;
   }

   int start_pos = onion_bitmask_add(ep->bitmask, 1);
   if (start_pos < 0) {
      DEBUG_ERR("Epoll no bitmask add!\n");
      return -1;
   }

   onion_epoll_slot_t *ep_slot = onion_block_alloc(ep->slots, start_pos); 
   if (!ep_slot) {
      DEBUG_ERR("Epoll ep_slot initialization failed!\n");
      goto unsuccessfull;
   }

   ep_slot->fd = fd;

   ret = onion_epoll_event_add(ep, ep_slot->fd, ONION_EPOLL_HANDLER_PUPPYFD, NULL);
   if (ret < 0) {
      DEBUG_FUNC("Epoll ctl add failed!\n");
      goto please_free;
   }

   ep_slot->data = data;
   ep_slot->func = func;
   ep_slot->shutdown = shutdown;
   ep_slot->shutdown_data = shutdown_data;

   ep_slot->time_alive = onion_tick();
   ep_slot->time_limit = ONION_ANONYMOUS_TIME_ALIVE;

   ep_slot->start_pos = start_pos;
   ep->conn_count = ep->conn_count + 1;
   DEBUG_FUNC("new onion_epoll_slot_t inited: %d\n", ep_slot->fd);
   return 0;
please_free:
   onion_epoll_slot_del(ep, ep_slot);
unsuccessfull:
   return -1;
}

void onion_epoll_slot_del(onion_epoll_t *ep, onion_epoll_slot_t *ep_slot) {
   if (onion_fd_is_valid(ep_slot->fd) == 0) {
      if (onion_fd_is_valid(ep->fd) == 0) {
         epoll_ctl(ep->fd, EPOLL_CTL_DEL, ep_slot->fd, NULL);
      }
      close(ep_slot->fd);
      ep_slot->fd = -1;
   }

   if (ep_slot->start_pos >= 0) {
      onion_bitmask_del(ep->bitmask, ep_slot->start_pos, 1);
      ep->conn_count = ep->conn_count -1;
   }

   onion_block_free(ep->slots, ep_slot);
   DEBUG_FUNC("new onion_epoll_slot_t exit\n");
}

onion_epoll_t *onion_epoll1_init(onion_handler_t handler, size_t conn_max) {
   int ret;

   if (conn_max < 1 || conn_max >= INT_MAX) {
      DEBUG_ERR("Connection max must be provided!\n");
      return NULL;
   }

   if (!handler) {
      DEBUG_ERR("Handler must be provided!\n");
      return NULL;
   }

   if (onion_epoll_static.current_slots + 1 > onion_epoll_static.max_slots) {
      DEBUG_ERR("onion_epoll_t limit!\n");
      return NULL;
   }

   int core = onion_epoll_static.current_slots;
   size_t bitmask_size = (conn_max + 63) / 64; 

   onion_epoll_t *ep = onion_slab_malloc(onion_epoll_static.slots, NULL, sizeof(onion_epoll_t));
   if (!ep) {
      DEBUG_ERR("onion_epoll_t initialization failed!\n");
      goto unsuccessfull;
   }

   ep->active = false;

   ep->fd = epoll_create1(EPOLL_CLOEXEC);
   if (onion_fd_is_valid(ep->fd) == -1) {
      DEBUG_ERR("Epoll initialization failed!\n");
      goto please_free;
   }

   ep->eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
   if (onion_fd_is_valid(ep->eventfd) == -1) {
      DEBUG_ERR("Epoll eventfd initialization failed!\n");
      goto please_free;
   }

   ep->conn_count = 0;
   ep->conn_max = conn_max;
   ep->handler = handler;

   ret = onion_block_init(&ep->slots, ep->conn_max * sizeof(onion_epoll_slot_t), sizeof(onion_epoll_slot_t)); 
   if (ret < 0) {
      DEBUG_ERR("Epoll slots initialization failed!\n");
      goto please_free;
   }

   ret = onion_block_init(&ep->tags, ONION_EPOLL_MAX_EVENTS * sizeof(onion_epoll_tag_t), sizeof(onion_epoll_tag_t));
   if (ret < 0) {
      DEBUG_ERR("Tags initialization failed!\n");
      goto please_free;
   }

   ret = onion_epoll_event_add(ep, ep->eventfd, ONION_EPOLL_HANDLER_EVENTFD, NULL);
   if (ret < 0) {
      DEBUG_ERR("Epoll event fd ctl add failed!\n");
      goto please_free;
   }

   ret = onion_bitmask_init(&ep->bitmask, bitmask_size, sizeof(uint64_t));
   if (ret < 0) {
      DEBUG_ERR("Epoll bitmask initialization failed!\n");
      goto please_free;
   }

   ret = pthread_create(&ep->flow, NULL, onion_epoll_hander, ep); 
   if (ret < 0) {
      DEBUG_ERR("Epoll pthread_create initialization failed!\n");
      goto please_free;
   }

   ep->event.events = EPOLLIN | EPOLLOUT | EPOLLET;
   ep->core = core;

   ret = onion_integrate_timer(ep);
   if (ret < 0) {
      DEBUG_ERR("No timer!\n");
      goto please_free;
   }

   onion_epoll_static.current_slots = onion_epoll_static.current_slots + 1;
   ep->active = true;
   DEBUG_FUNC("new epoll initied : %d core: %d size %zu!\n", ep->fd, ep->core, sizeof(*ep));
   return ep;
please_free:
   onion_epoll1_exit(ep);
unsuccessfull:
   return NULL;
}

void onion_epoll1_exit(onion_epoll_t *ep) {
   if (ep->tags) {
      onion_block_exit(ep->tags);
      ep->tags = NULL;
   }

   if (ep->slots) {
      int index;
      onion_epoll_slot_t *slots = (onion_epoll_slot_t*)ep->slots->data;
      while ((index = onion_find_bit(ep->bitmask, 1)) != -1) {
         int fd = slots[index].fd;
         if (onion_fd_is_valid(fd) == 0) {
            onion_epoll_slot_del(ep, &slots[index]);
         }
         onion_clear_bit(ep->bitmask, index);
      }
      onion_block_exit(ep->slots);
      ep->slots = NULL;
   }

   onion_liquidate_timer(ep);

   if (onion_fd_is_valid(ep->eventfd) == 0) {
      if (onion_fd_is_valid(ep->fd)) {
         epoll_ctl(ep->fd, EPOLL_CTL_DEL, ep->eventfd, NULL);
      }
      close(ep->eventfd);
      ep->eventfd = -1;
   }

   if (onion_fd_is_valid(ep->fd) == 0) {
      close(ep->fd);
      ep->fd = -1;
   }

   onion_slab_free(onion_epoll_static.slots, ep);

   if (ep->active) {
      onion_epoll_static.current_slots = onion_epoll_static.current_slots - 1;
   }

   ep->active = false;
   if (ep->flow) {
      pthread_join(ep->flow, NULL);
   }
   DEBUG_FUNC("epoll exit\n");
}

int onion_epoll_static_init(size_t core_count) {
   if (core_count < 1) {
      DEBUG_FUNC("Core count must be provided");
      return -1;
   }

   onion_epoll_static.current_slots = 0;
   onion_epoll_static.max_slots = core_count;
   onion_epoll_static.slots = onion_slab_init(sizeof(onion_epoll_t) * onion_epoll_static.max_slots);
   DEBUG_FUNC("dsfds  %zu\n", sizeof(onion_epoll_t) * onion_epoll_static.max_slots);
   if (!onion_epoll_static.slots) {
      DEBUG_ERR("onion_epoll_static slots initialization failed!\n");
      return -1;
   }

   DEBUG_FUNC("init.\n");
   return 0;
}

void onion_epoll_static_exit() {
   if (onion_epoll_static.slots) {
      onion_epoll_t *child_slots = (onion_epoll_t*)onion_epoll_static.slots->pool;
      for (int index = 0; index < onion_epoll_static.max_slots; index++) {
         bool isActive = child_slots[index].active;
         if (isActive) {
            onion_epoll1_exit(&child_slots[index]);
         }
      }
      onion_slab_exit(onion_epoll_static.slots);
      onion_epoll_static.slots = NULL;
   }

   onion_epoll_static.slots = 0;
   onion_epoll_static.max_slots = 0;
   DEBUG_FUNC("exit.\n");
}
